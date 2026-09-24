#pragma once

#include <G4Types.hh>

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Command-line options of the v4 executable `gc4`. Every physical
 * setting is a run-time option so one binary serves every composition.
 *
 *   gc4 produce       --soil S.soil --events N --seed K --out DIR [--source cry|gun]
 *   gc4 norm-library  --soil S.soil --isotope K40|U238|Th232 --events N --seed K --out DIR
 *   gc4 transport     --soil S.soil --input FILE.bin --seed K --out DIR [--replays R]
 *   gc4 analog        --soil S.soil --events N --seed K --out DIR
 *
 * Run `gc4 help` for every option and its default.
 */
struct V4Options
{
    enum class Mode
    {
        Produce,
        NormLibrary,
        Transport,
        Analog
    };

    Mode mode = Mode::Produce;
    std::string modeName;

    // common
    std::string soilPath;
    std::string outDir = ".";
    std::string tag = "run";
    std::uint64_t seed = 1;
    std::uint64_t events = 0;
    G4double exposure_s = 0.0; // produce/analog (CRY): stop once reached
    std::string physics = "FTFP_BERT_HP"; // or QGSP_BIC_HP
    bool thermalScattering = false;
    bool hpPhotoEvaporation = false;
    std::string captureModel = "hp"; // hp | default
    G4double worldHalfWidth_m = 20.0;
    G4double airHeight_m = 20.0;
    G4int printEvery = 100000;

    // produce / analog
    std::string source = "cry"; // produce: cry | gun
    std::string crySetup = "setupCry.file";
    std::string gunParticle = "neutron";
    G4double gunEnergy_MeV = 14.0;
    G4double gunBox_m = 1.0;
    G4double minGamma_MeV = 0.3;
    bool writeRecords = true;
    bool fluxTally = true;

    // norm-library
    std::string isotope = "K40";

    // transport
    std::string input;            // production .bin or NORM library .bin
    G4int replays = 1;            // per production record / library group
    G4double rMax_m = 5.0;        // lateral extent of position sampling
    G4double shadowFraction = 0.5;
    G4double broadScale_cm = 30.0;
    std::string sampler = "is";   // is | uniform
    G4int batches = 32;
    G4double normDepth_cm = 100.0;
    G4double rouletteTau = 5.0;   // optical depth where roulette starts (<=0 off)
    std::uint64_t normSamples = 0; // NORM: groups to transport (events)

    // analog
    G4double sourceHeight_m = 1.0;

    static V4Options Parse(const std::vector<std::string>& args);
    static std::string Usage();
    std::string Describe() const;
};
