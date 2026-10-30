# Geant4 Simulation for Cosmic Ray Induced Soil Organic Carbon Measurements

Geant4 simulation of the gamma rays that cosmic-ray showers (and the soil's own natural
radioactivity) produce in soil, and of what a germanium detector 20 cm above the ground
would record. It asks whether passive cosmogenic gamma spectrometry can measure soil
organic carbon (SOC).

The repository holds two programs:

- **`gc4` (v4, final)**: the simulation used for the final report. Soil compositions
  are read at run time, so one binary serves every soil. See
  [`campaign_v4/README_V4.md`](campaign_v4/README_V4.md) for what it computes, the run
  families and the output units, and
  [`campaign_v4/PREP_NOTES_V4.md`](campaign_v4/PREP_NOTES_V4.md) for the validation
  checks.
- **`groundCarbon` (v1-v3, legacy)**: the original three-stage program (soil
  characterisation, pre-detector transport, detection), kept for reference and
  described at the end of this file.

## Repository layout

```
gc4.cc                    v4 entry point
include/v4/, src/v4/      v4 sources (geometry, physics, generators, actions, output)
campaign_v4/              v4 campaign: build script, job driver, documentation
analysis/v4/              readers for gc4 output and reduced campaign results
analysis/*.ipynb          analysis notebooks
groundCarbon.cc, include/, src/, macros/   legacy v1-v3 program
setupCry.file             example CRY setup (the v4 driver writes its own per run)
```

## Requirements

- Geant4 11.x with its datasets (the campaign used 11.3; 11.4 was used for testing).
- [CRY 1.7](https://nuclear.llnl.gov/simulation/) cosmic-ray shower library, with
  `CRYHOME` pointing at its directory (CMake reads the library, headers and data from it).
- Python 3 with numpy for the campaign driver's `reduce` step and the analysis readers.

Soil composition files (`.soil`, see `include/v4/SoilModel.hh` for the format) are
built from NGSA geochemistry by a separate soil builder and are not part of this
repository.

## Building `gc4`

```bash
source env.sh            # or your own script that sources geant4.sh and exports CRYHOME
mkdir build-v4 && cd build-v4
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_GEANT4_UIVIS=OFF
make gc4
```

`campaign_v4/build_v4.sh` does the same (and builds CRY if needed), taking the
environment script from `GC_ENV` (see `campaign_v4/env_nuccomp1.sh.example`).

## Running `gc4`

```
gc4 produce      --soil S.soil (--events N | --exposure-s T) --seed K --out DIR [options]
gc4 norm-library --soil S.soil --isotope K40|U238|Th232 --events N --seed K --out DIR
gc4 transport    --soil S.soil --input FILE.bin --seed K --out DIR [options]
gc4 analog       --soil S.soil (--events N | --exposure-s T) --seed K --out DIR [options]
```

- `produce`: CRY showers into a laterally large soil; every gamma born in the soil is
  recorded with its creator process and target nucleus, then killed.
- `norm-library`: K-40, U-238 and Th-232 decay chains; the gammas of each decay.
- `transport`: replays production records (or samples NORM library groups) at
  importance-sampled positions into the detector; weighted energy-deposit histograms.
- `analog`: full showers above soil and detector, with every detector pulse.

`gc4 help` lists every option and its default. Each run writes binary arrays and a JSON
manifest (`<tag>.json`) that records its options and array shapes.

The whole campaign (163 runs) is planned and executed by `campaign_v4/run_v4.py`:

```bash
python3 campaign_v4/run_v4.py plan   --root RUN --soils SOILS/soil --compositions SOILS/campaign_compositions.json
python3 campaign_v4/run_v4.py run    --root RUN --jobs 32
python3 campaign_v4/run_v4.py reduce --root RUN
```

Reduced results (`RUN/reduced/<run>.npz` and `.json`) load with
`analysis/v4/spectra.py` (`Run.load`); `analysis/v4/quicklook.py` prints a one-line
summary per run.

## Known issue

`V4CaptureModel` hands non-hydrogen captures to `G4NeutronHPCapture`, which re-selects
the target element itself, so some of them still end as hydrogen captures. The raw
2.223 MeV line is about 1.5x too strong and the other capture lines are too weak by the
hydrogen share of captures. The code is left as it was run so it matches the published
results; the analysis applies a per-run correction. Details and the fix are in
`include/v4/V4CaptureModel.hh` and `campaign_v4/README_V4.md` §6.

## Legacy program: `groundCarbon` (v1-v3)

```
Usage: ./groundCarbon [soil-character|transport|detect] [info|binary|all] <path.bin (transport/detect only)> <run count (optional)>
Or: ./groundCarbon merge <output_path.bin> output1.bin output2.bin ...

Note: if no run count is given, Geant4 will launch a GUI.
Note: choosing 'info' will output a csv, 'binary' will output a binary file.
      'all' will do both. These options have no effect when doing a 'detect' run.

An example of how to use this:
  $ mpirun -n 5 ./groundCarbon soil-character binary 1000000
  $ ./groundCarbon merge soil-character/soil.bin soil-character/*.bin
  $ mpirun -n 5 ./groundCarbon transport all soil-character/soil.bin 10000000
  $ ./groundCarbon merge transport/predetection.bin transport/*.bin
  $ ./groundCarbon detect info transport/predetection.bin 10000000
```

The simulation is broken into three stages:
1. Soil characterisation (gamma rays produced in soil and recorded)
2. Pre-detector transport (recorded gamma rays propagated and particles incident to detector recorded)
3. Detection (particles propagated through detector to produce simulated spectra)

The program can be run in three modes:
1. Info (record verbose data in csv)
2. Binary (record only necessary data for the next stage, in raw binary format)
3. All (record both csv and binary)

Run `./groundCarbon soil-character <mode> <runs>` to run `<runs>` times. In binary mode,
this produces an `out[...].bin` file in the directory specified in the `GCConfig {}`
instance in `main`. Multiple of these files can be merged with
`./groundCarbon merge merged.bin file1.bin file2.bin [...]`. In info mode, this produces
an `out[...].csv` file in the same directory.

To do a pre-detector transport run, use `./groundCarbon transport <mode> <path.bin> <runs>`
to run `<runs>` times using data from `path.bin`, which must be a file produced by
`soil-character`. This again produces `.bin` and `.csv` files, which can be merged as
before.

To do a detection run, use `./groundCarbon detect <mode> <path.bin> <runs>` to run
`<runs>` times using data from `path.bin`, which must be a file produced by `transport`.
This only produces csv files, regardless of mode.

Geant4 can be opened in UI mode at any point by running `groundCarbon <stage> <path.bin?>`
without a number of runs. Using `beamOn` in the UI produces outputs as before.

`env.sh` and `run.sh` set up the environment and run the legacy binary from `build/`
(edit the Geant4 install path in `env.sh` for your machine).
