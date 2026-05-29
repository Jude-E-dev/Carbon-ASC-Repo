# Geant4 Simulation for Cosmic Ray Induced Soil Organic Carbon Measurements

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

## Structure

Simulation is broken into three stages:
1. Soil characterisation (gamma rays produced in soil and recorded)
2. Pre-detector transport (recorded gamma rays propagated and particles incident to detector recorded)
3. Detection (particles propagated through detector to produce simulated spectra)

The program can be run in three modes:
1. Info (record verbose data in csv)
2. Binary (record only necessary data for the next stage, in raw binary format)
3. All (record both csv and binary)

## Running the Program
Run `./groundCarbon soil-character <mode> <runs>` to run `<runs>` times.

In binary mode, this will produce an `out[...].bin` file in
the directory specified in the `GCConfig {}` instance in `main`. Multiple of these files can be merged with `./groundCarbon merge
merged.bin file1.bin file2.bin [...]`. In info mode, this will produce an `out[...].csv` file in the directory specified in the `GCConfig {}`
instance in `main`.

To do a pre-detector transport run, use `./groundCarbon transport <mode> <path.bin> <runs>` to run `<runs>` times using data from `path.bin`,
which must be a file produced by `soil-character`. This again produces `.bin` and `.csv` files. Binary files can be merged in the same way as
before.

To do a detection run, use `./groundCarbon detect <mode> <path.bin> <runs>` to run `<runs>` times using data from `path.bin`, which must be
a file produced by `transport`. This will only produce csv files, regardless of mode.

Geant4 can be openned in UI mode at any point by running `groundCarbon <stage> <path.bin?>` without a number of runs. Using `beamOn` in the UI
will produce outputs as before.
