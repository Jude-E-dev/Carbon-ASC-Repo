#include "v4/V4Options.hh"

#include <sstream>
#include <stdexcept>

namespace
{
bool ToBool(const std::string& v)
{
    if (v == "1" || v == "true" || v == "on" || v == "yes") {
        return true;
    }
    if (v == "0" || v == "false" || v == "off" || v == "no") {
        return false;
    }
    throw std::runtime_error("expected a boolean, got '" + v + "'");
}
} // namespace

V4Options V4Options::Parse(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        throw std::runtime_error(Usage());
    }
    V4Options o;
    o.modeName = args[1];
    if (o.modeName == "produce") {
        o.mode = Mode::Produce;
    }
    else if (o.modeName == "norm-library") {
        o.mode = Mode::NormLibrary;
        o.minGamma_MeV = 0.03;
        o.fluxTally = false;
    }
    else if (o.modeName == "transport") {
        o.mode = Mode::Transport;
    }
    else if (o.modeName == "analog") {
        o.mode = Mode::Analog;
        o.worldHalfWidth_m = 10.0;
        o.airHeight_m = 5.0;
    }
    else {
        throw std::runtime_error(Usage());
    }

    for (std::size_t i = 2; i < args.size(); i++) {
        const std::string& key = args[i];
        if (key.rfind("--", 0) != 0 || i + 1 >= args.size()) {
            throw std::runtime_error("bad argument '" + key + "'\n" + Usage());
        }
        const std::string& v = args[++i];
        if (key == "--soil") o.soilPath = v;
        else if (key == "--out") o.outDir = v;
        else if (key == "--tag") o.tag = v;
        else if (key == "--seed") o.seed = std::stoull(v);
        else if (key == "--events") o.events = std::stoull(v);
        else if (key == "--exposure-s") o.exposure_s = std::stod(v);
        else if (key == "--physics") o.physics = v;
        else if (key == "--thermal-scattering") o.thermalScattering = ToBool(v);
        else if (key == "--hp-photo-evaporation") o.hpPhotoEvaporation = ToBool(v);
        else if (key == "--capture-model") o.captureModel = v;
        else if (key == "--world-half-width-m") o.worldHalfWidth_m = std::stod(v);
        else if (key == "--air-height-m") o.airHeight_m = std::stod(v);
        else if (key == "--print-every") o.printEvery = std::stoi(v);
        else if (key == "--source") o.source = v;
        else if (key == "--cry-setup") o.crySetup = v;
        else if (key == "--gun-particle") o.gunParticle = v;
        else if (key == "--gun-energy-mev") o.gunEnergy_MeV = std::stod(v);
        else if (key == "--gun-box-m") o.gunBox_m = std::stod(v);
        else if (key == "--min-gamma-mev") o.minGamma_MeV = std::stod(v);
        else if (key == "--write-records") o.writeRecords = ToBool(v);
        else if (key == "--flux-tally") o.fluxTally = ToBool(v);
        else if (key == "--isotope") o.isotope = v;
        else if (key == "--input") o.input = v;
        else if (key == "--replays") o.replays = std::stoi(v);
        else if (key == "--r-max-m") o.rMax_m = std::stod(v);
        else if (key == "--shadow-fraction") o.shadowFraction = std::stod(v);
        else if (key == "--broad-scale-cm") o.broadScale_cm = std::stod(v);
        else if (key == "--sampler") o.sampler = v;
        else if (key == "--batches") o.batches = std::stoi(v);
        else if (key == "--norm-depth-cm") o.normDepth_cm = std::stod(v);
        else if (key == "--roulette-tau") o.rouletteTau = std::stod(v);
        else if (key == "--norm-samples") o.normSamples = std::stoull(v);
        else if (key == "--source-height-m") o.sourceHeight_m = std::stod(v);
        else {
            throw std::runtime_error("unknown option '" + key + "'\n" + Usage());
        }
    }

    if (o.soilPath.empty()) {
        throw std::runtime_error("--soil is required\n" + Usage());
    }
    if (o.physics != "FTFP_BERT_HP" && o.physics != "QGSP_BIC_HP") {
        throw std::runtime_error("--physics must be FTFP_BERT_HP or QGSP_BIC_HP");
    }
    if (o.captureModel != "hp" && o.captureModel != "default") {
        throw std::runtime_error("--capture-model must be hp or default");
    }
    if (o.mode == Mode::Transport) {
        if (o.input.empty()) {
            throw std::runtime_error("transport needs --input");
        }
        if (o.sampler != "is" && o.sampler != "uniform") {
            throw std::runtime_error("--sampler must be is or uniform");
        }
        if (o.replays < 1 || o.batches < 1) {
            throw std::runtime_error("--replays and --batches must be >= 1");
        }
        const G4double needed = o.rMax_m + 1.0;
        if (o.worldHalfWidth_m < needed) {
            o.worldHalfWidth_m = needed;
        }
    }
    else if (o.events == 0) {
        if (o.exposure_s > 0.0 && (o.mode == Mode::Analog || o.source == "cry")) {
            o.events = 2000000000ULL; // upper bound; the run stops at --exposure-s
        }
        else {
            throw std::runtime_error("--events (or --exposure-s for CRY) is required for "
                                     + o.modeName);
        }
    }
    if (o.mode == Mode::Produce && o.source != "cry" && o.source != "gun") {
        throw std::runtime_error("--source must be cry or gun");
    }
    if (o.mode == Mode::NormLibrary && o.isotope != "K40" && o.isotope != "U238"
        && o.isotope != "Th232")
    {
        throw std::runtime_error("--isotope must be K40, U238 or Th232");
    }
    return o;
}

std::string V4Options::Usage()
{
    return R"(Usage:
  gc4 produce      --soil S.soil (--events N | --exposure-s T) --seed K --out DIR [options]
  gc4 norm-library --soil S.soil --isotope K40|U238|Th232 --events N --seed K --out DIR
  gc4 transport    --soil S.soil --input FILE.bin --seed K --out DIR [options]
  gc4 analog       --soil S.soil (--events N | --exposure-s T) --seed K --out DIR [options]

Common:  --tag NAME  --physics FTFP_BERT_HP|QGSP_BIC_HP  --thermal-scattering 0|1
         --hp-photo-evaporation 0|1  --capture-model hp|default (hp)
         --world-half-width-m W (20; analog 10)
         --air-height-m H (20; analog 5)  --print-every N
Produce: --source cry|gun  --cry-setup FILE  --gun-particle P --gun-energy-mev E
         --gun-box-m L  --min-gamma-mev E (0.3)  --write-records 0|1  --flux-tally 0|1
Library: --isotope K40|U238|Th232  --min-gamma-mev E (0.03)
Transport: --input FILE (GC4PROD or GC4NLIB)  --replays R (1)  --norm-samples N
         --sampler is|uniform  --r-max-m R (5)  --shadow-fraction P (0.5)
         --broad-scale-cm A (30)  --batches B (32)  --norm-depth-cm D (100)
         --roulette-tau T (5; <=0 disables)
Analog:  --cry-setup FILE  --source-height-m H (1.0)
)";
}

std::string V4Options::Describe() const
{
    std::ostringstream s;
    s << "mode=" << modeName << ";soil=" << soilPath << ";seed=" << seed
      << ";events=" << events << ";exposure_target=" << exposure_s << ";physics=" << physics
      << ";ts=" << thermalScattering << ";hppe=" << hpPhotoEvaporation
      << ";cap=" << captureModel
      << ";W=" << worldHalfWidth_m << ";Hair=" << airHeight_m;
    return s.str();
}
