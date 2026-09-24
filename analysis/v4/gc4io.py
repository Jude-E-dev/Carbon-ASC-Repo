"""Readers for the files written by the v4 `gc4` executable.

Formats are defined in include/v4/V4Records.hh and include/v4/V4Output.hh.
Every run writes `<tag>.json` (manifest) plus mode-specific binaries:

  produce       <tag>.prod.bin (optional), .prodhist.bin, .proddepth.bin, .flux.bin
  norm-library  <tag>.nlib.bin, .prodhist.bin, .proddepth.bin
  transport     <tag>.thist.bin, .tdepth.bin
  analog        <tag>.pulses.bin, .tags.bin
"""

import json
from pathlib import Path

import numpy as np

HEADER_DTYPE = np.dtype(
    [
        ("magic", "S8"),
        ("version", "<u4"),
        ("recordSize", "<u4"),
        ("exposure_s", "<f8"),
        ("sourceArea_cm2", "<f8"),
        ("nEvents", "<u8"),
        ("nRecords", "<u8"),
        ("seed", "<u8"),
        ("soilName", "S64"),
        ("extra", "S136"),
    ]
)
assert HEADER_DTYPE.itemsize == 256

RECORD_DTYPE = np.dtype(
    [
        ("event", "<i4"),
        ("t_ns", "<f4"),
        ("E_MeV", "<f4"),
        ("z_cm", "<f4"),
        ("mu", "<f4"),
        ("parentPDG", "<i4"),
        ("parentE_MeV", "<f4"),
        ("primaryPDG", "<i4"),
        ("proc", "<i2"),
        ("targetZ", "u1"),
        ("targetA", "u1"),
    ]
)
assert RECORD_DTYPE.itemsize == 36

PULSE_DTYPE = np.dtype(
    [
        ("event", "<i8"),
        ("t_ns", "<f8"),
        ("edep_MeV", "<f4"),
        ("edepSoilGamma_MeV", "<f4"),
        ("edepSoilOther_MeV", "<f4"),
        ("primaryPDG", "<i4"),
    ]
)
assert PULSE_DTYPE.itemsize == 32

TAG_DTYPE = np.dtype([("event", "<i8"), ("t_ns", "<f8"), ("E_MeV", "<f4"), ("pdg", "<i4")])
assert TAG_DTYPE.itemsize == 24

PROC_NAMES = {
    0: "other",
    1: "eBrem",
    2: "annihil",
    3: "otherEM",
    4: "nCapture",
    5: "neutronInelastic",
    6: "otherInelastic",
    7: "radioactiveDecay",
    8: "muCapture",
    9: "photoNuclear",
    10: "decay",
    11: "otherHadronic",
}

ORIGIN_CLASSES = [
    "em_other",
    "capture_H",
    "capture_Si",
    "capture_Fe",
    "capture_other",
    "inelastic_C",
    "inelastic_O",
    "inelastic_Si",
    "inelastic_Fe",
    "inelastic_Al",
    "inelastic_other",
    "decay",
]
N_CLASSES = len(ORIGIN_CLASSES)


def origin_class(proc, target_z):
    """Vectorised twin of v4::OriginClass (src/v4/V4Records.cc)."""
    proc = np.asarray(proc)
    z = np.asarray(target_z).astype(int)
    out = np.zeros(np.broadcast(proc, z).shape, dtype=int)
    cap = proc == 4
    out[cap & (z == 1)] = 1
    out[cap & (z == 14)] = 2
    out[cap & (z == 26)] = 3
    out[cap & ~np.isin(z, [1, 14, 26])] = 4
    had = np.isin(proc, [5, 6, 8, 9, 11])
    for cls, zz in [(5, 6), (6, 8), (7, 14), (8, 26), (9, 13)]:
        out[had & (z == zz)] = cls
    out[had & ~np.isin(z, [6, 8, 14, 26, 13])] = 10
    out[proc == 7] = 11
    return out


def read_manifest(prefix):
    return json.loads(Path(str(prefix) + ".json").read_text())


def read_header(path):
    with open(path, "rb") as f:
        return np.frombuffer(f.read(256), dtype=HEADER_DTYPE)[0]


def read_records(path, dtype=RECORD_DTYPE):
    """Header and record array of a .prod.bin / .nlib.bin / .pulses.bin / .tags.bin."""
    header = read_header(path)
    data = np.fromfile(path, dtype=dtype, offset=256)
    if len(data) != int(header["nRecords"]):
        raise ValueError(f"{path}: header says {header['nRecords']} records, file has {len(data)}")
    return header, data


def read_array(prefix, suffix, shape):
    data = np.fromfile(str(prefix) + suffix, dtype="<f8")
    return data.reshape(shape)


def production_arrays(prefix):
    m = read_manifest(prefix)
    s = m["shapes"]
    hist = read_array(prefix, ".prodhist.bin", (s["n_classes"], s["n_ebins"]))
    depth = read_array(
        prefix, ".proddepth.bin", (s["n_classes"], s["n_ebins_coarse"], s["n_depth_coarse"])
    )
    out = {"manifest": m, "prodhist": hist, "proddepth": depth}
    flux_path = Path(str(prefix) + ".flux.bin")
    if flux_path.exists():
        n_e = int(round((s["flux_log_emax"] - s["flux_log_emin"]) * s["flux_e_per_decade"]))
        n_z = len(s["flux_z_edges_cm"]) - 1
        out["flux"] = read_array(prefix, ".flux.bin", (n_z, n_e))
    return out


def transport_arrays(prefix):
    m = read_manifest(prefix)
    s = m["shapes"]
    hist = read_array(prefix, ".thist.bin", (s["n_classes"], s["batches"], s["n_ebins"]))
    depth = read_array(prefix, ".tdepth.bin", (len(s["line_energies_mev"]), s["n_depth_fine"]))
    return {"manifest": m, "thist": hist, "tdepth": depth}


def transport_rate_scale(manifest):
    """Factor turning summed transport weights into counts per second.

    Production input:  counts/s = sum(w) / (exposure_s * source_area_cm2)
    NORM library input: counts/s per (Bq of parent / kg of mineral)
                        = sum(w) * norm_rate_factor
    """
    extra = manifest["extra"]
    if extra["input_type"] == "norm-library":
        return extra["norm_rate_factor"]
    return 1.0 / (extra["input_exposure_s"] * extra["input_source_area_cm2"])
