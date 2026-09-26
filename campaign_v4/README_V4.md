# v4 campaign: can passive cosmogenic gamma spectrometry see soil carbon through NORM and soil-composition backgrounds?

Package built on the Mac, 2026-09-24. Research plan: `Agents/plan/RESEARCH_PLAN.md`
(on the Mac). Operating procedure on nuccomp1: `RUNBOOK_V4.md`. Findings made while
building and testing the package: `PREP_NOTES_V4.md` (read it: several of them change
how results must be interpreted).

## 1. What this campaign computes

For each of **~160 simulated situations** it produces the gamma spectrum that the
**existing germanium detector** (bare Ge cylinder, r 8.3 cm, h 16.8 cm, centre 20 cm
above the ground, unchanged from v1-v3) would record from:

1. **cosmic rays** (CRY showers at the ground surface, reference site Canberra,
   latitude -35.28), split by the nucleus and process that made each gamma, and
2. **the soil's own NORM**, as three templates per soil (K-40, U-238 series, Th-232
   series, each per 1 Bq/kg of the parent in the mineral), which the analysis scales
   by each soil's measured K, U and Th.

The analysis (on the Mac, afterwards) combines them into expected field spectra and
asks, signal by signal (4.438 MeV carbon, the C/O ratio, 2.223 MeV hydrogen, capture
ratios, the whole spectrum), whether the SOC information survives NORM counting noise
and site-to-site variation in the rest of the soil.

## 2. The simulation (one binary, `gc4`, four modes)

| mode | primaries | geometry | writes |
|---|---|---|---|
| `produce` | CRY showers on a 5 x 5 m box at z = 0 | soil 40 x 40 m, 1.5 m deep (6 SLGA layers) under 20 m of air; vacuum below | every gamma >= 0.3 MeV born in the soil: E, depth, direction, time, **creator process, target nucleus**, parent, cosmic primary; then killed. Neutron track-length tallies vs depth and energy |
| `transport` | the production records, 4 replays each (or NORM library groups) | same soil + the Ge detector | Edep histograms (1 keV, 0-12 MeV) by 12 origin classes x 32 batches, with weights |
| `norm-library` | K-40 / U-238 / Th-232 at rest; full chain in secular equilibrium | soil | the gammas of each decay, grouped so prompt coincidences (e.g. 2614 + 583 keV) are kept |
| `analog` | CRY showers 1 m above the ground | soil, detector, a virtual tag plane at 45 cm | every detector pulse (100 ns buckets) with time and origin (direct shower / soil gamma / other soil particle) and every particle crossing the tag plane |

Key differences from v3 (see PREP_NOTES_V4.md for the evidence):

- **Soils are read at run time** from `soils/<id>.soil`; no source patching, one binary.
- **Laterally large world and per-area normalisation.** v3 replayed gammas produced
  under a 25 m2 source over a 19.6 m2 disk with the same exposure (rates 4/pi high) and
  lost every neutron at its 5 x 5 x 0.5 m boundaries.
- **Importance-sampled transport.** Each production gamma is placed laterally by a
  mixture of (a) the detector's "shadow" for its direction (tight, for unscattered rays,
  and 40 cm wider, for small-angle scatters), (b) a heavy-tailed density around the
  detector and (c) a defensive uniform share over the 5 m disk that bounds every
  weight, with weight 1/q. Unbiased (validated against uniform
  sampling) and ~10^3 times more efficient for full-energy peaks than v3's uniform disk.
  The detect stage is merged into it: no second replay stage.
- **Neutron capture photons from evaluated data** (`V4CaptureModel`: `G4NeutronHPCapture`,
  with exact n + p -> d + gamma for H-1) forced on every Geant4 version: 11.4's default
  drops the Si-28(n,g) 3.539/4.934 MeV lines, and plain `G4NeutronHPCapture` on 11.4
  smears the 2.223 MeV hydrogen line.
- **Soil model v2**: trace neutron absorbers (Gd, Sm, Eu, Dy, Cl, Li, B), structural
  (clay) H, carbonate C, realistic organic matter (5% H), depth profiles to 1.5 m, water
  decoupled from SOC.

## 3. The runs

`plan.json` (from `run_v4.py plan`) lists every job. Families:

| family | runs | what varies | CRY | NORM |
|---|---|---|---|---|
| central | 5 | chromosol, dermosol, vertosol, calcarosol, kandosol at their geometric means | yes | yes |
| jacobian | 118 | one soil parameter at a time about each central soil: SOC x0.25/x0.5/x2/x4, SOC x2 in 0-15 cm only, water dry/wet/near-saturated, bulk density +-10%, OM hydrogen 10%, structural H 0/high, Gd p10/p90, B 5/60 ppm, carbonate, major-element matrix lo/hi, Fe x0.5/x2, a seed replicate; chromosol also at latitudes -12.46 and -42.88 | yes | yes |
| population | 25 | 5 real NGSA sites per Order, each at its own measured composition | yes | yes |
| convergence | 3 | chromosol central with 0.5, 1.0, 2.0 m of soil | yes | no |
| d1_variant | 8 | chromosol central: QGSP_BIC_HP; thermal scattering; default capture model; world 5 x 5 m; world 80 x 80 m; 0.5 m air; v3-like geometry; CRY box 10 m | yes | no |
| d1_validation | 2 | chromosol central production chunks 0-3 re-transported with the uniform sampler (20 replays) and with 8 IS replays | transport only | no |
| analog | 2 | chromosol central: CRY box 5 m for 12 h of sky; CRY box 10 m for 3 h | analog | no |

Per CRY run: **1800 s of simulated sky** in 20 production chunks of 90 s, each
followed by its transport (4 replays). Per NORM template: 32 M library groups in 4
chunks. NORM libraries: 2 M chains per isotope, shared by every soil.

Production records are deleted once transported, except chunks 0-3 of the five
central runs (kept for later re-analysis, ~0.4 GB per run).

## 4. Outputs and units

`run_v4.py reduce` writes `reduced/<run>.npz` and `reduced/<run>.json`. Arrays are
**sums of weights**; the JSON gives the scale:

- CRY: counts per second = array x `cry.counts_per_s_scale`
  (= 1 / (exposure_s x source_area_cm2)). Multiply by 86,400 for counts per day.
- NORM: `norm_<iso>_batch` is already counts per second **per Bq/kg of the parent in
  the mineral phase**; the soil's activities are in the composition JSON
  (`norm.activity_bq_per_kg_mineral`).
- `t_class[c][E]`: origin classes 0 em_other, 1 capture_H, 2 capture_Si, 3 capture_Fe,
  4 capture_other, 5 inelastic_C, 6 inelastic_O, 7 inelastic_Si, 8 inelastic_Fe,
  9 inelastic_Al, 10 inelastic_other, 11 decay ("inelastic" = any non-capture hadronic
  reaction, including muon capture and photonuclear). The class is decided by the
  production gamma, so e.g. the Compton continuum of 4.438 MeV gammas from oxygen is in
  class 6.
- `t_batch[b][E]`: the same spectrum split into 32 batches of whole cosmic showers
  (shower id mod 32). **Uncertainties come from the scatter between batches**
  (`sigma = sqrt(32 x var_b)` for a sum), which includes every level of correlation
  (shared showers, replays, detector binomial) that broke v3's error bars.
- `prodhist[c][E]`: gammas born in the soil per class (divide by exposure for s^-1,
  whole column, per source area).
- `flux_track_cm[z][logE]`: neutron track length; divide by
  (exposure_s x source_area_cm2 x slab thickness) for flux in n cm^-2 s^-1.
- `tdepth[line][cm]`: depth of origin of full-energy counts at 12 line energies.
- Analog: `analog_pulses` (event, t_ns, edep, edep from soil gammas, from other soil
  particles, primary PDG) and `analog_tags` (event, t_ns, E, pdg).

## 5. Expected values (Mac, Geant4 11.4.2, 2026-09-24)

See RUNBOOK_V4.md Appendix B for per-job wall times and output sizes measured in the
pilot. Physics sanity values (chromosol central):

| quantity | expected |
|---|---|
| CRY exposure per shower (5 m box, lat -35.28) | ~1.30e-4 s |
| production records per s of sky | ~28,000-32,000 |
| total CRY count rate in the Ge (> 1 keV) | ~4-5 counts/s |
| NORM Th-232 2.614 MeV peak | ~1.5e6 counts/day (at 37 Bq/kg) |
| NORM continuum near 2.22 MeV | ~2-4e4 counts per 10 keV per day |
| analog total pulse rate | ~15 counts/s (muons dominate the energy) |

## 6. Known limitations

- The 4.438 MeV C-12 line: Geant4 HP emits about half of it with no Doppler shift and
  the rest with a one-sided low-energy tail (~16-26 keV rms); real lines from fast
  neutrons are broadened symmetrically by tens of keV. The analysis treats the line
  shape as a systematic (wide windows, kinematic broadening).
- CRY has no atmospheric pressure or snow/vegetation; altitude tables exist only at
  0, 2100 and 11300 m. Cosmic flux normalisation is a nuisance parameter in the analysis.
- Boron is assumed (20 ppm central, 5 and 60 ppm variants): NGSA reports only
  aqua-regia B.
- The detector is the idealised v1-v3 crystal (the user chose to keep it); resolution
  is applied in the analysis.
