#!/usr/bin/env python3
"""Driver for the v4 ground-carbon campaign (production x transport, NORM, analog).

Subcommands (all take --root, the campaign run directory):

  plan    --soils DIR --compositions FILE [--profile full|smoke] [--ids ...]
          Write ROOT/plan.json (every job, its command, dependencies and cost).
  run     --jobs N [--max-attempts 2] [--only-kinds ...]
          Execute the plan with a global pool of N concurrent gc4 processes.
          Resumable: finished jobs have a .done marker and are skipped.
          `touch ROOT/STOP` makes it finish the running jobs and exit.
  status  Counts by kind and state, core-hours used and estimated remaining.
  reduce  Sum each finished run's chunks into ROOT/reduced/<run>.npz (+ .json).
  bundle  Pack ROOT/reduced, the plan, every manifest and a log digest into
          ROOT/v4_results_<date>.tar.gz for the analysis.

Only the standard library is needed except for `reduce` (numpy).
See README_V4.md for what each job family computes and the output units.
"""

import argparse
import datetime as dt
import hashlib
import json
import os
import signal
import subprocess
import tarfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE.parent  # ground-carbon/
DEFAULT_GC4 = SRC / "build-v4" / "gc4"

ISOTOPES = ["K40", "U238", "Th232"]
CRY_DATE = "3-14-2026"

# ----------------------------------------------------------------------------
# Profiles: sizes of every job family. Costs (CPU seconds) are Mac M3 Pro
# benchmarks from 2026-09-24 and are only used for planning / ETA.
# ----------------------------------------------------------------------------
PROFILES = {
    "full": {
        "cry_exposure_s": 1800.0,     # simulated sky time per composition
        "cry_chunk_s": 90.0,          # per production job
        "replays": 4,                 # continuum noise is replay-dominated (PREP_NOTES 5)
        "keep_chunks_central": 4,     # production records kept for re-analysis
        "norm_samples": 32_000_000,   # per isotope per composition
        "norm_chunk_samples": 8_000_000,
        "library_chains": 2_000_000,
        "analog": [
            # id, soil, cry box (m), world half width (m), exposure (s), chunk (s)
            ("analog_box5", "chromosol", 5.0, 10.0, 43200.0, 180.0),
            ("analog_box10", "chromosol", 10.0, 15.0, 10800.0, 45.0),
        ],
        "validation_chunks": 4,
        "uniform_replays": 20,
        "k8_replays": 8,
    },
    "smoke": {
        "cry_exposure_s": 4.0,
        "cry_chunk_s": 2.0,
        "replays": 2,
        "keep_chunks_central": 1,
        "norm_samples": 200_000,
        "norm_chunk_samples": 100_000,
        "library_chains": 20_000,
        "analog": [("analog_box5", "chromosol", 5.0, 10.0, 2.0, 1.0)],
        "validation_chunks": 1,
        "uniform_replays": 2,
        "k8_replays": 2,
    },
}

COST = {  # CPU seconds, for estimates only
    "produce_per_exposure_s": 14.0,
    "records_per_exposure_s": 30000.0,
    "transport_events_per_s": 11000.0,
    "uniform_events_per_s": 5500.0,
    "norm_samples_per_s": 45000.0,
    "library_chains_per_s": 4500.0,
    "analog_per_exposure_s_box5": 16.0,
}

# D1: variants of the chromosol central run that change run options, not soil.
PHYSICS_VARIANTS = [
    ("phys_qgsp", "chromosol", ["--physics", "QGSP_BIC_HP"], "hadronic physics QGSP_BIC_HP"),
    ("phys_ts", "chromosol", ["--thermal-scattering", "1"], "thermal scattering law for H in water"),
    ("phys_capdefault", "chromosol", ["--capture-model", "default"],
     "capture model left at the physics list default (G4NeutronRadCaptureHP on 11.4)"),
    ("geo_w2p5", "chromosol", ["--world-half-width-m", "2.5"], "5 x 5 m soil (v3 lateral size)"),
    ("geo_w40", "chromosol", ["--world-half-width-m", "40"], "80 x 80 m soil"),
    ("geo_air0p5", "chromosol", ["--air-height-m", "0.5"], "0.5 m of air (v3)"),
    ("geo_v3like", "chromosol_depth50",
     ["--world-half-width-m", "2.5", "--air-height-m", "0.5"],
     "v3-like geometry: 5 x 5 x 0.5 m soil, 0.5 m air"),
]
BOX10_VARIANT = ("geo_box10", "chromosol", 10.0, "CRY source box 10 m")


# ----------------------------------------------------------------------------
# helpers
# ----------------------------------------------------------------------------
def seed_for(*parts):
    h = hashlib.sha256("/".join(str(p) for p in parts).encode()).hexdigest()
    return int(h[:8], 16) % 2_000_000_000 + 1


def now():
    return dt.datetime.now().isoformat(timespec="seconds")


def load_json(path):
    with open(path) as f:
        return json.load(f)


def write_json(path, obj):
    tmp = Path(str(path) + ".tmp")
    with open(tmp, "w") as f:
        json.dump(obj, f, indent=1)
    os.replace(tmp, path)


def physics_args(extra):
    """The physics options of a production command, which its transport reuses."""
    out = []
    for opt in ("--physics", "--thermal-scattering", "--capture-model"):
        if opt in extra:
            out += [opt, extra[extra.index(opt) + 1]]
    return out


def cry_setup_text(latitude, box_m):
    return (
        "returnNeutrons 1\nreturnProtons 1\nreturnGammas 1\nreturnElectrons 1\n"
        "returnMuons 1\nreturnPions 1\n"
        f"date {CRY_DATE}\nlatitude {latitude:.2f}\naltitude 0\nsubboxLength {box_m:g}\n"
    )


# ----------------------------------------------------------------------------
# plan
# ----------------------------------------------------------------------------
def cmd_plan(args):
    root = Path(args.root).resolve()
    root.mkdir(parents=True, exist_ok=True)
    soils = Path(args.soils).resolve()
    comps = load_json(args.compositions)
    prof = PROFILES[args.profile]
    gc4 = str(Path(args.gc4).resolve())

    comp_list = comps["compositions"]
    if args.ids:
        comp_list = [c for c in comp_list if c["id"] in args.ids]
    by_id = {c["id"]: c for c in comps["compositions"]}
    ref_lat = comps["reference_latitude"]

    (root / "cry").mkdir(exist_ok=True)

    def setup_file(lat, box):
        p = root / "cry" / f"setupCry_lat{lat:+.2f}_box{box:g}.file"
        p.write_text(cry_setup_text(lat, box))
        return str(p)

    jobs = []
    runs = []

    def add_job(job):
        job.setdefault("deps", [])
        jobs.append(job)
        return job["id"]

    # NORM libraries (soil only sets the beta bremsstrahlung medium)
    lib_soil = str(soils / "chromosol.soil")
    lib_ids = {}
    for iso in ISOTOPES:
        prefix = root / "libs" / iso
        jid = f"lib/{iso}"
        lib_ids[iso] = jid
        add_job({
            "id": jid, "kind": "library", "priority": 0,
            "cmd": [gc4, "norm-library", "--soil", lib_soil, "--isotope", iso,
                    "--events", str(prof["library_chains"]), "--seed", str(seed_for(jid)),
                    "--out", str(prefix.parent), "--tag", iso, "--print-every", "0",
                    "--world-half-width-m", "2", "--air-height-m", "1"],
            "prefix": str(prefix), "expect": [".nlib.bin", ".json"],
            "est_cpu_s": prof["library_chains"] / COST["library_chains_per_s"],
        })

    def add_cry_run(run_id, comp_id, soil_path, lat, box, extra, family, change, priority,
                    keep_chunks, norm=True):
        n_chunks = max(1, round(prof["cry_exposure_s"] / prof["cry_chunk_s"]))
        setup = setup_file(lat, box)
        rdir = root / "runs" / run_id
        chunk_jobs = []
        for c in range(n_chunks):
            base = rdir / "cry" / f"c{c:03d}"
            pj = add_job({
                "id": f"{run_id}/produce/{c:03d}", "kind": "produce", "priority": priority,
                "cmd": [gc4, "produce", "--soil", soil_path, "--cry-setup", setup,
                        "--exposure-s", str(prof["cry_chunk_s"]),
                        "--seed", str(seed_for(run_id, "produce", c)),
                        "--out", str(base.parent), "--tag", base.name + "p",
                        "--print-every", "0"] + extra,
                "prefix": str(base) + "p",
                "expect": [".prod.bin", ".prodhist.bin", ".proddepth.bin", ".flux.bin", ".json"],
                "est_cpu_s": prof["cry_chunk_s"] * COST["produce_per_exposure_s"] * (box / 5.0) ** 2,
            })
            tj = add_job({
                "id": f"{run_id}/transport/{c:03d}", "kind": "transport", "priority": priority,
                "deps": [pj],
                "cmd": [gc4, "transport", "--soil", soil_path,
                        "--input", str(base) + "p.prod.bin",
                        "--replays", str(prof["replays"]),
                        "--seed", str(seed_for(run_id, "transport", c)),
                        "--out", str(base.parent), "--tag", base.name + "t",
                        "--print-every", "0"] + physics_args(extra),
                "prefix": str(base) + "t", "expect": [".thist.bin", ".tdepth.bin", ".json"],
                "delete_after": ([] if c < keep_chunks else [str(base) + "p.prod.bin"]),
                "est_cpu_s": prof["cry_chunk_s"] * COST["records_per_exposure_s"]
                * prof["replays"] / COST["transport_events_per_s"] * (box / 5.0) ** 2,
            })
            chunk_jobs.append((pj, tj))
        norm_jobs = []
        if norm:
            n_norm_chunks = max(1, round(prof["norm_samples"] / prof["norm_chunk_samples"]))
            for iso in ISOTOPES:
                for c in range(n_norm_chunks):
                    base = rdir / "norm" / f"{iso}_c{c:02d}"
                    norm_jobs.append(add_job({
                        "id": f"{run_id}/norm/{iso}/{c:02d}", "kind": "norm",
                        "priority": priority, "deps": [lib_ids[iso]],
                        "cmd": [gc4, "transport", "--soil", soil_path,
                                "--input", str(root / "libs" / iso) + ".nlib.bin",
                                "--norm-samples", str(prof["norm_chunk_samples"]),
                                "--seed", str(seed_for(run_id, "norm", iso, c)),
                                "--out", str(base.parent), "--tag", base.name,
                                "--print-every", "0"],
                        "prefix": str(base), "expect": [".thist.bin", ".tdepth.bin", ".json"],
                        "est_cpu_s": prof["norm_chunk_samples"] / COST["norm_samples_per_s"],
                    }))
        runs.append({"id": run_id, "composition": comp_id, "soil": soil_path, "family": family,
                     "change": change, "latitude": lat, "cry_box_m": box, "extra_args": extra,
                     "cry_chunks": [t for _, t in chunk_jobs],
                     "produce_chunks": [p for p, _ in chunk_jobs],
                     "norm_jobs": norm_jobs,
                     "composition_meta": by_id.get(comp_id, {})})
        return chunk_jobs

    # priorities: 1 = D1 + centrals, 2 = jacobian, 3 = population, 4 = convergence, 5 = analog
    centrals = {"chromosol", "dermosol", "vertosol", "calcarosol", "kandosol"}
    for c in comp_list:
        cid = c["id"]
        fam = c["family"]
        pri = 1 if cid in centrals else {"jacobian": 2, "population": 3, "convergence": 4}[fam]
        keep = prof["keep_chunks_central"] if cid in centrals else 0
        add_cry_run(cid, cid, str(soils / f"{cid}.soil"), c["latitude"], 5.0, [], fam,
                    c["change"], pri, keep, norm=(fam != "convergence"))

    if not args.ids or "chromosol" in args.ids:
        for vid, comp, extra, change in PHYSICS_VARIANTS:
            add_cry_run(vid, comp, str(soils / f"{comp}.soil"), ref_lat, 5.0, extra,
                        "d1_variant", change, 1, 0, norm=False)
        vid, comp, box, change = BOX10_VARIANT
        add_cry_run(vid, comp, str(soils / f"{comp}.soil"), ref_lat, box,
                    ["--world-half-width-m", "25"], "d1_variant", change, 1, 0, norm=False)

        # sampler validation on the kept chromosol central production chunks
        central = next(r for r in runs if r["id"] == "chromosol")
        for vid, sampler, replays in (("val_uniform", "uniform", prof["uniform_replays"]),
                                      ("val_k8", "is", prof["k8_replays"])):
            vjobs = []
            for c in range(min(prof["validation_chunks"], len(central["produce_chunks"]))):
                base = root / "runs" / vid / "cry" / f"c{c:03d}"
                src_prefix = root / "runs" / "chromosol" / "cry" / f"c{c:03d}p"
                vjobs.append(add_job({
                    "id": f"{vid}/transport/{c:03d}", "kind": "transport", "priority": 1,
                    "deps": [central["produce_chunks"][c]],
                    "cmd": [gc4, "transport", "--soil", central["soil"],
                            "--input", str(src_prefix) + ".prod.bin",
                            "--sampler", sampler, "--replays", str(replays),
                            "--seed", str(seed_for(vid, c)), "--out", str(base.parent),
                            "--tag", base.name + "t", "--print-every", "0"],
                    "prefix": str(base) + "t", "expect": [".thist.bin", ".tdepth.bin", ".json"],
                    "est_cpu_s": prof["cry_chunk_s"] * COST["records_per_exposure_s"] * replays
                    / (COST["uniform_events_per_s"] if sampler == "uniform"
                       else COST["transport_events_per_s"]),
                }))
            runs.append({"id": vid, "composition": "chromosol", "soil": central["soil"],
                         "family": "d1_validation",
                         "change": f"chromosol central chunks re-transported, sampler={sampler}, "
                                   f"replays={replays}",
                         "latitude": ref_lat, "cry_box_m": 5.0, "extra_args": [],
                         "cry_chunks": vjobs, "produce_chunks": central["produce_chunks"][:len(vjobs)],
                         "norm_jobs": [], "composition_meta": by_id.get("chromosol", {})})

        # analog runs
        for aid, comp, box, half, expo, chunk in prof["analog"]:
            setup = setup_file(ref_lat, box)
            n = max(1, round(expo / chunk))
            ajobs = []
            for c in range(n):
                base = root / "runs" / aid / "analog" / f"c{c:03d}"
                ajobs.append(add_job({
                    "id": f"{aid}/analog/{c:03d}", "kind": "analog", "priority": 5,
                    "cmd": [gc4, "analog", "--soil", str(soils / f"{comp}.soil"),
                            "--cry-setup", setup, "--exposure-s", str(chunk),
                            "--world-half-width-m", str(half),
                            "--seed", str(seed_for(aid, c)), "--out", str(base.parent),
                            "--tag", base.name, "--print-every", "0"],
                    "prefix": str(base), "expect": [".pulses.bin", ".tags.bin", ".json"],
                    "est_cpu_s": chunk * COST["analog_per_exposure_s_box5"] * (box / 5.0) ** 2,
                }))
            runs.append({"id": aid, "composition": comp, "soil": str(soils / f"{comp}.soil"),
                         "family": "analog", "change": f"analog, CRY box {box} m",
                         "latitude": ref_lat, "cry_box_m": box, "extra_args": [],
                         "analog_jobs": ajobs, "cry_chunks": [], "produce_chunks": [],
                         "norm_jobs": [], "composition_meta": by_id.get(comp, {})})

    # keep production records that validation needs until validation is done
    needed_by_validation = set()
    for r in runs:
        if r["family"] == "d1_validation":
            needed_by_validation.update(r["produce_chunks"])
    for job in jobs:
        if job["kind"] == "transport" and job.get("delete_after"):
            pid = job["deps"][0] if job["deps"] else None
            if pid in needed_by_validation:
                job["delete_after"] = []

    total_cpu = sum(j["est_cpu_s"] for j in jobs)
    plan = {
        "created": now(), "profile": args.profile, "root": str(root), "gc4": gc4,
        "soils": str(soils), "compositions_file": str(Path(args.compositions).resolve()),
        "reference_latitude": ref_lat, "profile_settings": prof, "cost_model": COST,
        "n_jobs": len(jobs), "est_core_hours_mac": total_cpu / 3600.0,
        "runs": runs, "jobs": jobs,
    }
    write_json(root / "plan.json", plan)
    kinds = {}
    for j in jobs:
        k = kinds.setdefault(j["kind"], [0, 0.0])
        k[0] += 1
        k[1] += j["est_cpu_s"] / 3600
    print(f"plan: {len(runs)} runs, {len(jobs)} jobs, ~{total_cpu / 3600:.0f} core-h (Mac rates)")
    for k, (n, h) in sorted(kinds.items()):
        print(f"  {k:10s} {n:6d} jobs  ~{h:8.1f} core-h")



# ----------------------------------------------------------------------------
# folding: every finished chunk is added into per-run accumulators and its
# large binaries deleted, so disk use stays ~10 MB per run instead of ~1 GB.
# ----------------------------------------------------------------------------
ISO_OF = {iso: iso for iso in ISOTOPES}


def run_of(job):
    return job["id"].split("/")[0]


def _load_acc(path, fields):
    import numpy as np

    if path.exists():
        with np.load(path, allow_pickle=False) as z:
            acc = {k: z[k].copy() for k in z.files}
        acc["folded"] = list(json.loads(str(acc["folded"])))
        acc["meta"] = json.loads(str(acc["meta"]))
        return acc
    acc = {k: None for k in fields}
    acc["folded"] = []
    acc["meta"] = {}
    return acc


def _save_acc(path, acc):
    import numpy as np

    path.parent.mkdir(parents=True, exist_ok=True)
    out = {k: v for k, v in acc.items() if k not in ("folded", "meta") and v is not None}
    out["folded"] = np.array(json.dumps(acc["folded"]))
    out["meta"] = np.array(json.dumps(acc["meta"]))
    tmp = path.with_suffix(".tmp.npz")
    np.savez(tmp, **out)
    os.replace(tmp, path)


def _add(acc, key, value):
    acc[key] = value.astype("f8") if acc.get(key) is None else acc[key] + value


def fold_job(root, job):
    """Fold a verified job's outputs into ROOT/accum/<run>/. Idempotent."""
    import numpy as np

    kind = job["kind"]
    if kind not in ("produce", "transport", "norm"):
        return
    prefix = Path(job["prefix"])
    manifest = load_json(str(prefix) + ".json")
    s = manifest["shapes"]
    acc_dir = root / "accum" / run_of(job)

    def arr(suffix, shape):
        return np.fromfile(str(prefix) + suffix, dtype="<f8").reshape(shape)

    if kind == "produce":
        path = acc_dir / "prod.npz"
        acc = _load_acc(path, ["prodhist", "proddepth", "flux_track_cm"])
        if job["id"] not in acc["folded"]:
            nz = len(s["flux_z_edges_cm"]) - 1
            ne = int(round((s["flux_log_emax"] - s["flux_log_emin"]) * s["flux_e_per_decade"]))
            _add(acc, "prodhist", arr(".prodhist.bin", (s["n_classes"], s["n_ebins"])))
            _add(acc, "proddepth", arr(".proddepth.bin",
                                       (s["n_classes"], s["n_ebins_coarse"], s["n_depth_coarse"])))
            _add(acc, "flux_track_cm", arr(".flux.bin", (nz, ne)))
            m = acc["meta"]
            m["exposure_s"] = m.get("exposure_s", 0.0) + manifest["exposure_s"]
            m["events"] = m.get("events", 0) + manifest["events"]
            m["records"] = m.get("records", 0) + manifest["records"]
            m["source_area_cm2"] = manifest["source_area_cm2"]
            m["geant4_version"] = manifest.get("geant4_version")
            m["capture_model"] = manifest.get("capture_model")
            m["physics"] = manifest.get("physics")
            m["thermal_scattering"] = manifest.get("thermal_scattering")
            m.setdefault("wall_s", []).append(manifest["extra"].get("wall_s"))
            acc["folded"].append(job["id"])
            _save_acc(path, acc)
        for suffix in (".prodhist.bin", ".proddepth.bin", ".flux.bin"):
            Path(str(prefix) + suffix).unlink(missing_ok=True)

    elif kind == "transport":
        path = acc_dir / "cry.npz"
        acc = _load_acc(path, ["t_class", "t_batch", "t_class_batch_10keV", "tdepth"])
        if job["id"] not in acc["folded"]:
            th = arr(".thist.bin", (s["n_classes"], s["batches"], s["n_ebins"]))
            nb = th.shape[2] // 10
            _add(acc, "t_class", th.sum(1))
            _add(acc, "t_batch", th.sum(0))
            _add(acc, "t_class_batch_10keV",
                 th[:, :, : nb * 10].reshape(th.shape[0], th.shape[1], nb, 10).sum(-1))
            _add(acc, "tdepth", arr(".tdepth.bin", (len(s["line_energies_mev"]), s["n_depth_fine"])))
            ex = manifest["extra"]
            m = acc["meta"]
            m["exposure_s"] = m.get("exposure_s", 0.0) + ex["input_exposure_s"]
            m["source_area_cm2"] = ex["input_source_area_cm2"]
            m["records"] = m.get("records", 0) + ex["input_records"]
            m["hits"] = m.get("hits", 0) + manifest["transport_hits"]
            m["events"] = m.get("events", 0) + manifest["events"]
            m["roulette_killed"] = m.get("roulette_killed", 0) + ex["roulette_killed"]
            m["sampler"] = ex["sampler"]
            m["replays"] = ex["replays"]
            m.setdefault("wall_s", []).append(ex.get("wall_s"))
            acc["folded"].append(job["id"])
            _save_acc(path, acc)
        for suffix in (".thist.bin", ".tdepth.bin"):
            Path(str(prefix) + suffix).unlink(missing_ok=True)

    elif kind == "norm":
        iso = job["id"].split("/")[2]
        path = acc_dir / f"norm_{iso}.npz"
        acc = _load_acc(path, ["weighted_batch", "tdepth"])
        if job["id"] not in acc["folded"]:
            ex = manifest["extra"]
            th = arr(".thist.bin", (s["n_classes"], s["batches"], s["n_ebins"])).sum(0)
            n = ex["norm_samples"]
            # sum of factor_c * N_c * hist_c; divided by sum N_c at reduce time
            _add(acc, "weighted_batch", th * ex["norm_rate_factor"] * n)
            _add(acc, "tdepth", arr(".tdepth.bin", (len(s["line_energies_mev"]), s["n_depth_fine"]))
                 * ex["norm_rate_factor"] * n)
            m = acc["meta"]
            m["samples"] = m.get("samples", 0) + n
            m["mineral_areal_g_cm2"] = ex["norm_mineral_areal_g_cm2"]
            m["library_chains"] = ex["library_chains"]
            m["library_groups"] = ex["library_groups"]
            m["hits"] = m.get("hits", 0) + manifest["transport_hits"]
            m.setdefault("wall_s", []).append(ex.get("wall_s"))
            acc["folded"].append(job["id"])
            _save_acc(path, acc)
        for suffix in (".thist.bin", ".tdepth.bin"):
            Path(str(prefix) + suffix).unlink(missing_ok=True)

# ----------------------------------------------------------------------------
# run
# ----------------------------------------------------------------------------
def job_paths(job):
    prefix = Path(job["prefix"])
    return prefix, Path(str(prefix) + ".done"), Path(str(prefix) + ".fail"), Path(str(prefix) + ".log")


def job_done(job):
    return job_paths(job)[1].exists()


def verify_outputs(job):
    prefix = Path(job["prefix"])
    missing = [e for e in job["expect"] if not Path(str(prefix) + e).exists()]
    if missing:
        return f"missing outputs {missing}"
    try:
        manifest = load_json(str(prefix) + ".json")
    except Exception as exc:  # noqa: BLE001
        return f"unreadable manifest: {exc}"
    if manifest.get("events", 0) <= 0:
        return "manifest reports 0 events"
    return None


def cmd_run(args):
    root = Path(args.root).resolve()
    plan = load_json(root / "plan.json")
    jobs = {j["id"]: j for j in plan["jobs"]}
    order = sorted(plan["jobs"], key=lambda j: (j["priority"], j["id"]))
    if args.only_kinds:
        order = [j for j in order if j["kind"] in args.only_kinds]
    stop_file = root / "STOP"
    log = open(root / "driver.log", "a")

    def say(msg):
        line = f"[{now()}] {msg}"
        print(line, flush=True)
        log.write(line + "\n")
        log.flush()

    attempts = {}
    for j in order:
        fail = job_paths(j)[2]
        if fail.exists():
            try:
                attempts[j["id"]] = load_json(fail).get("attempts", 1)
            except Exception:  # noqa: BLE001
                attempts[j["id"]] = 1

    running = {}  # id -> (Popen, start, logfile)
    say(f"driver start: {len(order)} jobs in scope, --jobs {args.jobs}")
    interrupted = {"flag": False}

    def on_signal(signum, frame):  # noqa: ARG001
        interrupted["flag"] = True
        say(f"signal {signum}: finishing running jobs, starting no new ones")

    signal.signal(signal.SIGTERM, on_signal)
    signal.signal(signal.SIGINT, on_signal)

    def ready(j):
        if job_done(j) or j["id"] in running:
            return False
        if attempts.get(j["id"], 0) >= args.max_attempts:
            return False
        return all(job_done(jobs[d]) for d in j["deps"] if d in jobs)

    last_report = 0.0
    while True:
        # reap
        for jid, (proc, start, lf) in list(running.items()):
            rc = proc.poll()
            if rc is None:
                continue
            lf.close()
            j = jobs[jid]
            _, done, fail, logp = job_paths(j)
            wall = time.time() - start
            problem = None if rc == 0 else f"exit code {rc}"
            if problem is None:
                problem = verify_outputs(j)
            if problem is None:
                try:
                    fold_job(root, j)
                except Exception as exc:  # noqa: BLE001
                    problem = f"fold failed: {exc!r}"
            if problem is None:
                for path in j.get("delete_after", []):
                    Path(path).unlink(missing_ok=True)
                write_json(done, {"id": jid, "finished": now(), "wall_s": wall,
                                  "attempt": attempts.get(jid, 0) + 1})
                if fail.exists():
                    fail.unlink()
                say(f"done  {jid} ({wall / 60:.1f} min)")
            else:
                attempts[jid] = attempts.get(jid, 0) + 1
                tail = ""
                try:
                    tail = "".join(open(logp, errors="replace").readlines()[-30:])
                except OSError:
                    pass
                write_json(fail, {"id": jid, "problem": problem, "attempts": attempts[jid],
                                  "when": now(), "log_tail": tail})
                say(f"FAIL  {jid}: {problem} (attempt {attempts[jid]}/{args.max_attempts})")
            del running[jid]

        stopping = interrupted["flag"] or stop_file.exists()
        if not stopping:
            for j in order:
                if len(running) >= args.jobs:
                    break
                if not ready(j):
                    continue
                prefix, _, _, logp = job_paths(j)
                prefix.parent.mkdir(parents=True, exist_ok=True)
                lf = open(logp, "w")
                cmd = ["nice", "-n", str(args.nice)] + j["cmd"]
                proc = subprocess.Popen(cmd, stdout=lf, stderr=subprocess.STDOUT,
                                        cwd=str(prefix.parent), start_new_session=True)
                running[j["id"]] = (proc, time.time(), lf)

        if not running:
            remaining = [j for j in order if not job_done(j)]
            blocked = [j for j in remaining if attempts.get(j["id"], 0) >= args.max_attempts]
            if stopping:
                say(f"stopped: {len(remaining)} jobs not done")
            elif not remaining:
                say("all jobs in scope are done")
            else:
                say(f"nothing runnable: {len(remaining)} not done, {len(blocked)} failed "
                    f"{args.max_attempts}x (see *.fail); the rest depend on them")
            break

        if time.time() - last_report > 600:
            n_done = sum(1 for j in order if job_done(j))
            say(f"progress: {n_done}/{len(order)} done, {len(running)} running")
            last_report = time.time()
        time.sleep(2)


# ----------------------------------------------------------------------------
# status
# ----------------------------------------------------------------------------
def cmd_status(args):
    root = Path(args.root).resolve()
    plan = load_json(root / "plan.json")
    table = {}
    used = 0.0
    remaining_est = 0.0
    for j in plan["jobs"]:
        _, done, fail, _ = job_paths(j)
        state = "done" if done.exists() else ("failed" if fail.exists() else "todo")
        row = table.setdefault(j["kind"], {"done": 0, "failed": 0, "todo": 0})
        row[state] += 1
        if state == "done":
            try:
                used += load_json(done)["wall_s"]
            except Exception:  # noqa: BLE001
                pass
        else:
            remaining_est += j["est_cpu_s"]
    print(f"{'kind':10s} {'done':>7s} {'failed':>7s} {'todo':>7s}")
    for k, row in sorted(table.items()):
        print(f"{k:10s} {row['done']:7d} {row['failed']:7d} {row['todo']:7d}")
    print(f"core-hours used (wall of finished jobs): {used / 3600:.1f}")
    print(f"core-hours remaining (Mac-rate estimate): {remaining_est / 3600:.1f}")
    fails = sorted(root.rglob("*.fail"))
    for f in fails[:10]:
        info = load_json(f)
        print(f"  FAILED {info['id']}: {info['problem']} (attempts {info['attempts']})")
    if len(fails) > 10:
        print(f"  ... {len(fails) - 10} more .fail files")


# ----------------------------------------------------------------------------
# reduce
# ----------------------------------------------------------------------------
def cmd_reduce(args):
    """Turn each finished run's accumulators into ROOT/reduced/<run>.npz + .json.

    Arrays (float32 unless noted), all as *sums of weights*; multiply by the
    scale in the JSON to get counts per second:
      t_class [class][Edep 1 keV]           CRY, summed over batches
      t_batch [batch][Edep 1 keV]           CRY, summed over classes
      t_class_batch_10keV [class][batch][Edep 10 keV]
      tdepth  [line][source depth 1 cm]     CRY
      prodhist [class][E 1 keV], proddepth [class][E 10 keV][depth 5 cm]
                                            production counts (per exposure_s)
      flux_track_cm [z bin][log E bin] (f8) neutron track length (per exposure_s,
                                            per source_area_cm2)
      norm_<iso>_batch [batch][Edep 1 keV]  counts/s per Bq/kg of parent in the
                                            mineral (already scaled)
      norm_<iso>_tdepth [line][depth]       same units
      analog_pulses, analog_tags            record arrays (analog runs)
    """
    import numpy as np

    root = Path(args.root).resolve()
    plan = load_json(root / "plan.json")
    jobs = {j["id"]: j for j in plan["jobs"]}
    out = root / "reduced"
    out.mkdir(exist_ok=True)
    n_written = 0

    for run in plan["runs"]:
        rid = run["id"]
        target = out / f"{rid}.npz"
        needed = run.get("cry_chunks", []) + run.get("norm_jobs", []) + run.get("analog_jobs", [])
        if run["family"] != "d1_validation":
            needed = needed + run.get("produce_chunks", [])
        if not needed or not all(job_done(jobs[j]) for j in needed):
            continue
        if target.exists() and not args.force:
            continue
        acc_dir = root / "accum" / rid
        data = {}
        info = {"run": {k: v for k, v in run.items() if k not in ("composition_meta",)},
                "composition": run.get("composition_meta", {})}

        if run.get("cry_chunks"):
            acc = _load_acc(acc_dir / "cry.npz", [])
            if len(acc["folded"]) != len(run["cry_chunks"]):
                print(f"skip {rid}: {len(acc['folded'])}/{len(run['cry_chunks'])} transport chunks folded")
                continue
            for k in ("t_class", "t_batch", "t_class_batch_10keV", "tdepth"):
                data[k] = acc[k].astype("f4")
            m = acc["meta"]
            m["counts_per_s_scale"] = 1.0 / (m["exposure_s"] * m["source_area_cm2"])
            info["cry"] = m
            if run["family"] != "d1_validation":
                pacc = _load_acc(acc_dir / "prod.npz", [])
                if len(pacc["folded"]) != len(run["produce_chunks"]):
                    print(f"skip {rid}: production not fully folded")
                    continue
                data["prodhist"] = pacc["prodhist"].astype("f4")
                data["proddepth"] = pacc["proddepth"].astype("f4")
                data["flux_track_cm"] = pacc["flux_track_cm"]
                info["production"] = pacc["meta"]

        if run.get("norm_jobs"):
            info["norm"] = {}
            ok = True
            for iso in ISOTOPES:
                n_expected = sum(1 for j in run["norm_jobs"] if f"/norm/{iso}/" in j)
                acc = _load_acc(acc_dir / f"norm_{iso}.npz", [])
                if len(acc["folded"]) != n_expected:
                    ok = False
                    break
                n = acc["meta"]["samples"]
                data[f"norm_{iso}_batch"] = (acc["weighted_batch"] / n).astype("f4")
                data[f"norm_{iso}_tdepth"] = (acc["tdepth"] / n).astype("f4")
                info["norm"][iso] = acc["meta"]
            if not ok:
                print(f"skip {rid}: NORM not fully folded")
                continue

        if run.get("analog_jobs"):
            pulse_dt = np.dtype([("event", "<i8"), ("t_ns", "<f8"), ("edep_MeV", "<f4"),
                                 ("edepSoilGamma_MeV", "<f4"), ("edepSoilOther_MeV", "<f4"),
                                 ("primaryPDG", "<i4")])
            tag_dt = np.dtype([("event", "<i8"), ("t_ns", "<f8"), ("E_MeV", "<f4"), ("pdg", "<i4")])
            pulses, tags = [], []
            exposure = 0.0
            offset = 0
            for aj in run["analog_jobs"]:
                prefix = Path(jobs[aj]["prefix"])
                m = load_json(str(prefix) + ".json")
                p = np.fromfile(str(prefix) + ".pulses.bin", dtype=pulse_dt, offset=256)
                t = np.fromfile(str(prefix) + ".tags.bin", dtype=tag_dt, offset=256)
                p["event"] += offset
                t["event"] += offset
                offset += m["events"]
                p["t_ns"] += exposure * 1e9
                t["t_ns"] += exposure * 1e9
                exposure += m["exposure_s"]
                pulses.append(p)
                tags.append(t)
            data["analog_pulses"] = np.concatenate(pulses)
            data["analog_tags"] = np.concatenate(tags)
            info["analog"] = {"exposure_s": exposure, "events": offset,
                              "source_area_cm2": run["cry_box_m"] ** 2 * 1e4,
                              "note": "t_ns = CRY shower time + per-chunk offset; showers "
                                      "inside a chunk are not time-ordered"}

        np.savez_compressed(target, **data)
        write_json(out / f"{rid}.json", info)
        n_written += 1
        print(f"reduced {rid}")
    print(f"reduce: wrote {n_written} run files to {out}")


# ----------------------------------------------------------------------------
# bundle
# ----------------------------------------------------------------------------
def cmd_bundle(args):
    root = Path(args.root).resolve()
    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M")
    target = root / f"v4_results_{stamp}.tar.gz"
    with tarfile.open(target, "w:gz") as tar:
        tar.add(root / "plan.json", arcname="v4_results/plan.json")
        if (root / "driver.log").exists():
            tar.add(root / "driver.log", arcname="v4_results/driver.log")
        if (root / "reduced").exists():
            tar.add(root / "reduced", arcname="v4_results/reduced")
        for p in sorted((root / "runs").rglob("*.json")):
            tar.add(p, arcname="v4_results/manifests/" + str(p.relative_to(root / "runs")))
        for p in sorted(root.rglob("*.fail")):
            tar.add(p, arcname="v4_results/failures/" + str(p.relative_to(root)))
        for p in sorted((root / "libs").glob("*.json")):
            tar.add(p, arcname="v4_results/libs/" + p.name)
        for extra in ("RUN_NOTES.md", "SMOKE.md"):
            if (root / extra).exists():
                tar.add(root / extra, arcname="v4_results/" + extra)
    size = target.stat().st_size / 1e6
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    print(f"bundle: {target} ({size:.1f} MB) sha256 {digest}")


# ----------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("plan")
    p.add_argument("--root", required=True)
    p.add_argument("--soils", required=True)
    p.add_argument("--compositions", required=True)
    p.add_argument("--profile", default="full", choices=sorted(PROFILES))
    p.add_argument("--ids", nargs="*")
    p.add_argument("--gc4", default=str(DEFAULT_GC4))

    p = sub.add_parser("run")
    p.add_argument("--root", required=True)
    p.add_argument("--jobs", type=int, required=True)
    p.add_argument("--max-attempts", type=int, default=2)
    p.add_argument("--nice", type=int, default=10)
    p.add_argument("--only-kinds", nargs="*")

    for name in ("status", "reduce", "bundle"):
        p = sub.add_parser(name)
        p.add_argument("--root", required=True)
        if name == "reduce":
            p.add_argument("--force", action="store_true")

    args = ap.parse_args()
    {"plan": cmd_plan, "run": cmd_run, "status": cmd_status, "reduce": cmd_reduce,
     "bundle": cmd_bundle}[args.cmd](args)


if __name__ == "__main__":
    main()
