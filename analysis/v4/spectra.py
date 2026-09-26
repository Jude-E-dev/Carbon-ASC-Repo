"""Expected detector spectra from reduced v4 runs (run_v4.py reduce).

    from spectra import Run, Resolution
    run = Run.load(".../reduced", "chromosol")
    cry = run.cry()                     # Spectrum: counts/s per 1 keV bin, MC variance
    norm = run.norm()                   # at the soil's own K, U, Th
    field = (cry + norm).smear(Resolution.hpge()).scale(86400)   # counts per day

A Spectrum carries the expected rate per bin and its Monte Carlo covariance
estimated from the 32 shower batches (diagonal per bin; `batches` keeps the
per-batch arrays so any derived quantity can be propagated exactly).
"""

import json
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

N_BATCH = 32
EBIN_MEV = 0.001
ISOTOPES = ("K40", "U238", "Th232")
CLASSES = ["em_other", "capture_H", "capture_Si", "capture_Fe", "capture_other",
           "inelastic_C", "inelastic_O", "inelastic_Si", "inelastic_Fe", "inelastic_Al",
           "inelastic_other", "decay"]


def energies(n=12000):
    """Bin centres in MeV."""
    return (np.arange(n) + 0.5) * EBIN_MEV


def window(lo_mev, hi_mev):
    return slice(int(round(lo_mev / EBIN_MEV)), int(round(hi_mev / EBIN_MEV)))


@dataclass
class Spectrum:
    """Rate per 1 keV bin with per-batch decomposition for MC errors.

    batches[b] sums to `rate`; the MC variance of any linear functional f is
    N_BATCH * var_b(f(batches[b])).
    """

    batches: np.ndarray  # [N_BATCH][n_bins]
    unit: str = "counts/s"
    label: str = ""

    @property
    def rate(self):
        return self.batches.sum(0)

    def functional(self, f):
        """Value and MC sigma of f(rate) for f linear (e.g. a window sum)."""
        vals = np.array([f(b) for b in self.batches])
        return vals.sum(), float(np.sqrt(N_BATCH * vals.var(ddof=1)))

    def window(self, lo, hi):
        s = window(lo, hi)
        return self.functional(lambda b: b[s].sum())

    def __add__(self, other):
        return Spectrum(self.batches + other.batches, self.unit, f"{self.label}+{other.label}")

    def __sub__(self, other):
        return Spectrum(self.batches - other.batches, self.unit, f"{self.label}-{other.label}")

    def scale(self, k, unit=None):
        return Spectrum(self.batches * k, unit or self.unit, self.label)

    def smear(self, res):
        return Spectrum(res.apply(self.batches), self.unit, self.label + "~res")


class Resolution:
    """Gaussian energy resolution, FWHM(E) = sqrt(a + b E + c E^2) in keV (E in keV)."""

    def __init__(self, a, b, c, name=""):
        self.a, self.b, self.c, self.name = a, b, c, name
        self._cache = {}

    @classmethod
    def hpge(cls):
        # typical large coaxial HPGe: 1.0 keV at 122 keV, 1.9 keV at 1332 keV
        # (-> ~2.3 keV at 2.2 MeV, ~3.1 keV at 4.4 MeV)
        return cls(0.75, 0.0021, 1.1e-7, "HPGe")

    @classmethod
    def ideal(cls):
        return cls(0.0, 0.0, 0.0, "ideal")

    def fwhm_kev(self, e_kev):
        return np.sqrt(self.a + self.b * e_kev + self.c * e_kev ** 2)

    def apply(self, arr):
        """Smear the last axis (1 keV bins) of arr."""
        n = arr.shape[-1]
        if self.a == self.b == self.c == 0:
            return arr.copy()
        key = n
        if key not in self._cache:
            e = (np.arange(n) + 0.5)
            sig = self.fwhm_kev(e) / 2.3548
            half = int(np.ceil(5 * sig.max()))
            offs = np.arange(-half, half + 1)
            # kernel[k][i]: fraction of bin i's content that lands in bin i + offs[k]
            ker = np.exp(-0.5 * (offs[:, None] / np.maximum(sig[None, :], 1e-6)) ** 2)
            ker /= ker.sum(0, keepdims=True)
            self._cache[key] = (offs, ker)
        offs, ker = self._cache[key]
        out = np.zeros_like(arr, dtype="f8")
        for k, o in enumerate(offs):
            contrib = arr * ker[k]
            if o >= 0:
                out[..., o:] += contrib[..., : n - o]
            else:
                out[..., :o] += contrib[..., -o:]
        return out


@dataclass
class Run:
    rid: str
    info: dict
    data: dict = field(repr=False)

    @classmethod
    def load(cls, reduced_dir, rid):
        reduced_dir = Path(reduced_dir)
        info = json.loads((reduced_dir / f"{rid}.json").read_text())
        with np.load(reduced_dir / f"{rid}.npz") as z:
            data = {k: z[k] for k in z.files}
        return cls(rid, info, data)

    # ------------------------------------------------------------------ CRY
    @property
    def cry_scale(self):
        return self.info["cry"]["counts_per_s_scale"]

    def cry(self, classes=None):
        """CRY spectrum (counts/s per keV bin). classes: iterable of class
        indices or names to keep (default all)."""
        if classes is None:
            return Spectrum(self.data["t_batch"].astype("f8") * self.cry_scale, label="cry")
        idx = [CLASSES.index(c) if isinstance(c, str) else c for c in classes]
        # full-resolution class spectra have no batch split; use the 10 keV
        # class x batch array to apportion each batch (errors approximate)
        cls_rate = self.data["t_class"][idx].sum(0).astype("f8") * self.cry_scale
        cb = self.data["t_class_batch_10keV"][idx].sum(0).astype("f8")  # [batch][10 keV]
        tot = cb.sum(0)
        frac = np.divide(cb, tot, out=np.full_like(cb, 1.0 / N_BATCH), where=tot > 0)
        frac_1kev = np.repeat(frac, 10, axis=1)
        frac_1kev = np.pad(frac_1kev, ((0, 0), (0, cls_rate.size - frac_1kev.shape[1])),
                           constant_values=1.0 / N_BATCH)
        return Spectrum(frac_1kev * cls_rate[None, :], label="cry:" + ",".join(map(str, idx)))

    def class_rates(self, lo, hi):
        """Rate (counts/s) in [lo, hi) MeV per origin class, with MC sigma (10 keV
        resolution of the batch split)."""
        s10 = slice(int(round(lo / 0.01)), int(round(hi / 0.01)))
        cb = self.data["t_class_batch_10keV"][:, :, s10].sum(-1).astype("f8") * self.cry_scale
        val = cb.sum(1)
        sig = np.sqrt(N_BATCH * cb.var(1, ddof=1))
        return {CLASSES[c]: (val[c], sig[c]) for c in range(len(CLASSES))}

    # ----------------------------------------------------------------- NORM
    def activities(self):
        """Bq per kg of mineral of each parent, from the composition JSON."""
        comp = self.info.get("composition", {})
        act = comp.get("norm_bq_per_kg_mineral")
        if act is None:
            raise KeyError(f"{self.rid}: no NORM activities in the reduced JSON")
        return act

    def norm(self, activities=None, isotopes=ISOTOPES):
        act = activities or self.activities()
        total = None
        for iso in isotopes:
            arr = self.data[f"norm_{iso}_batch"].astype("f8") * act[iso]
            total = arr if total is None else total + arr
        return Spectrum(total, label="norm")

    # ----------------------------------------------------------- production
    def production_rate(self):
        """Gammas born in the soil per second per cm2 of ground, [class][E 1 keV]."""
        p = self.info["production"]
        return self.data["prodhist"].astype("f8") / (p["exposure_s"] * p["source_area_cm2"])

    def neutron_flux(self):
        """Neutron flux (n cm-2 s-1) per [z bin][log10 E bin] and the bin edges."""
        p = self.info["production"]
        edges = np.array(self.info["run"].get("flux_z_edges_cm") or FLUX_Z_EDGES)
        dz = np.diff(edges)
        flux = self.data["flux_track_cm"] / (p["exposure_s"] * p["source_area_cm2"] * dz[:, None])
        loge = np.linspace(-10, 4, flux.shape[1] + 1)
        return flux, edges, loge


FLUX_Z_EDGES = list(range(-150, 0, 2)) + [0, 5, 10, 20, 50, 100, 150, 200, 300, 500, 1000, 2000]
