# PREP_NOTES_V4: findings while building and testing the v4 package

Mac (M3 Pro), Geant4 11.4.2, 2026-09-24. Where these notes disagree with
README_V4.md or the research plan, these notes win.

## 1. Geant4 11.4 changed the neutron-capture photons (matters for every capture line)

Since Geant4 11.4 (phys-builders-V11-03-00, 2025-06) the HP physics lists use
`G4NeutronRadCaptureHP` (PhotonEvaporation cascades) instead of `G4NeutronHPCapture`
(evaluated G4NDL capture photons). Thermal neutrons on pure Si and Fe, 3000 each:

| | default model on 11.4 (`--capture-model default`) | `G4NeutronHPCapture` (`hp`, v4 default) | evaluated data |
|---|---|---|---|
| Si-28(n,g) 3.539 MeV per capture | absent | 0.61 | ~0.68 |
| Si-28(n,g) 4.934 MeV | absent | 0.56 | ~0.61 |
| Si strongest lines | 1.273 (0.33), 2.028 (0.26) | 3.539, 4.934, 8.474 | 3.539, 4.934 |
| Fe-56(n,g) 7.631 / 7.646 MeV | absent | 0.15 / 0.12 | ~0.27 / 0.22 |

Geant4 11.3 (nuccomp1, v3, Jay's runs) still used `G4NeutronHPCapture`, so v1-v3 are
unaffected.

**Hydrogen needs its own treatment.** G4NDL has no capture photon data for H-1, so
`G4NeutronHPCapture` falls back to G4PhotonEvaporation, which on 11.4 turns the 2.2246 MeV
deuteron excitation into a multi-photon continuum: in a Chromosol test the 2.223 MeV
full-energy peak fell to ~13 counts/day with nothing left in the line. (`G4NeutronRadCaptureHP`
gets hydrogen right but not Si/Fe.) `gc4` therefore registers **`V4CaptureModel`** below
20 MeV on every version: exact two-body kinematics n + p -> d + gamma for H-1 (one photon,
2.2231 MeV for thermal neutrons), `G4NeutronHPCapture` for every other target. Checked with
thermal neutrons: water 99.7% of captures give the single 2.223 MeV photon; Si 3.539 (0.61),
4.934 (0.56); Fe 7.631 (0.12), 7.646 (0.09). In a Chromosol, the 2.223 MeV peak is then
~360-420 counts/day in four independent runs.

The model covering 0-20 MeV is retired and the >20 MeV capture model keeps its upper range;
the run log prints what it replaced. The `phys_capdefault` run keeps the list default so
the size of the effect is measured.

## 2. The 4.438 MeV "carbon" line is mostly oxygen

In a short test in a Chromosol-like soil (1.2% C by mass, 2.7 s of sky, 22 nuclear
gammas within 3 keV of 4.439 MeV) 21 came from O-16 targets (O-16(n,n'alpha)C-12* and
spallation, which need E_n > ~12 MeV; cosmic neutrons supply them) and 1 from C-12.
The campaign measures the ratio properly. This matches the v3 observation that the
4.44 MeV yield falls as SOC rises. The v4 origin classes split
the two in every spectrum (classes 5 and 6).

## 3. 4.438 MeV line shape

14 MeV neutrons on graphite: ~50% of 4.439 MeV gammas at exactly 4.4389 MeV, the rest
in a one-sided low-energy tail (rms 26 keV at 14 MeV, 16 keV at 6 MeV). Real lines are
Doppler broadened symmetrically (tens of keV FWHM). O-16 targets at 50 MeV give a broad
(rms 134 keV) distribution from the cascade models. Treat line shape as a systematic.

## 4. The NORM library needed decay indices, not times

Decay times in a chain span Gyr (the Th-232 parent decays at ~1e26 ns); neither float
nor double can separate decays minutes apart at that scale, so time-windowing merged a
whole chain into one "prompt" group (it would fake coincidence summing, e.g.
Bi-212 727 keV with Tl-208 2614 keV). Each radioactive decay now gets an index; short
lived (< 1 us) excited daughters inherit it. Checked: 2614 keV is accompanied by
583 keV in 85% of cases (literature 85%), 609 keV by 1120 keV in 32%; groups per chain
3.24 -> 2.24 (Th), 1.89 (U). In GC4NLIB files the `t_ns` field holds this index.

## 5. Importance-sampled transport is unbiased

Synthetic source (100 k gammas, 2.223 and 4.439 MeV, uniform 0-40 cm, isotropic):

| estimator | 2.223 full-energy peak | 4.439 peak | CPU |
|---|---|---|---|
| IS (shadow 0.7) | 111.1 +- 2.4 | 105.8 +- 2.0 | 32 s |
| uniform disk, R 5 m, 60 replays | 121.7 +- 13.5 | 140 +- 13 | 1122 s |
| broad component only | 71 +- 8 | 83 +- 10 | 32 s |

IS and uniform agree (0.8 sigma, 2.6 sigma). The broad-only estimator is low because
its weights are heavy tailed for grazing rays and a short run misses the rare large
ones (its error bar is then also too small); the shadow component removes that. The
shadow density was checked numerically (normalisation 1.000, empirical/analytic
0.999-1.001). Default shadow fraction lowered to 0.5 to reduce continuum variance.

For NORM coincidence groups (several gammas from one decay) the shadow component is a
mixture over every upward gamma of the group, chosen uniformly; before this, only the first
stored gamma was aimed at, so e.g. Tl-208's 2.614 MeV gamma was often placed as if isotropic
and carried heavy weights. The NORM continuum near 2.2 MeV improved from ~46% to ~23% relative
error per 10 keV bin for 2 M samples (~6% at the campaign's 32 M).

**Heavy tails, and the final sampler (2026-09-26).** Logging every hit with weight > 2e4
(`--debug-weight`) showed the heaviest hits are scattered photons that start just outside
the tight shadow or 1-2 m from the detector, where only the thin broad density samples
them. With 2 replays one such hit put 2196 +- 92% counts/day into a 50 keV continuum
window that reads 164 +- 9% with 8 replays; and on the synthetic source the IS *total*
rate came out 1375 +- 46 against uniform 1588 +- 44 (3.4 sigma): a finite run usually
misses the rare large weights, so both the value and its batch error come out low. Peaks
were never affected. Two changes fix it:
- a **wide shadow** (margin 40 cm) takes half of the shadow draws, covering small-angle
  scatters next to the unscattered region;
- a **defensive uniform component** (20% of draws, uniform on the 5 m disk) bounds every
  weight by pi R^2 / (0.2 x replays).

Synthetic check with the final mixture (tight 0.3 / wide 0.3 / broad 0.2 / uniform 0.2
for upward gammas): total 1533 +- 82 (uniform 1588 +- 44), 2.223 peak 107 +- 3, 4.439 peak
111 +- 3, continuum 1-2 MeV 257 +- 51 (uniform 244 +- 16), at ~12x less CPU than uniform.
The continuum noise is replay-dominated (8 replays roughly halved it relative to 2), so the
campaign uses **4 replays**. Projected MC error per run at 1800 s of sky: peaks 2-5%,
continuum per 50 keV ~4-8%. The `val_uniform` and `val_k8` runs repeat these checks at
scale on real production records.

Timing note: runs on a memory-starved Mac showed wall rates of ~550-3000 events/s; the
CPU rate is ~10,000 events/s (checked with `time`: 72 s user for 600 k events).
The `val_uniform` run repeats this test at scale on real production records.

## 6. Other checks

- Per-area normalisation fixes v3's 4/pi: production over a 5 x 5 m source box,
  transport weights 1/q per unit area, rates divided by (exposure x box area).
- CRY's neutron spectrum spans 1 meV to 100 TeV (cosmics_*.data), so thermal and
  epithermal atmospheric neutrons are included.
- The legacy `groundCarbon` binary is untouched; it still has v3's Ca molar mass
  (69.72, gallium's). v4 uses NIST elements throughout.
- Production cost is ~9-14 CPU-s per second of sky (v3: 9.8), despite the 64x larger
  soil: the extra neutron tracking is cheap. It yields ~1.9x v3's gammas per second
  (deeper soil, no escaping neutrons).
- NORM library content: K-40 1.461 MeV 10.6% per decay; U series 609 keV 45.8%,
  1120 15.1%, 1764 15.4%; Th series 2614 34.7%, 583 29.9%, 911 26.7%.

## 7. Disk

Chunk histograms are folded into `accum/<run>/*.npz` as soon as a chunk finishes and
the chunk binaries deleted; without this the campaign would write ~180 GB. Expect
~2 GB of accumulators, ~2 GB of kept production records, ~0.5 GB of analog pulses and
transient production files of ~100 MB per running job.
