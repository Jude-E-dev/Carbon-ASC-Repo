#include "v4/V4Output.hh"

#include <G4Version.hh>
#include <G4ios.hh>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace
{
int EnergyBin(double E_MeV, double width, int n)
{
    if (E_MeV < 0.0) {
        return -1;
    }
    const int bin = static_cast<int>(E_MeV / width);
    return bin < n ? bin : -1;
}

int FluxEnergyBin(double E_MeV)
{
    if (E_MeV <= 0.0) {
        return -1;
    }
    const double x = std::log10(E_MeV);
    if (x < V4Output::FLUX_LOG_EMIN || x >= V4Output::FLUX_LOG_EMAX) {
        return -1;
    }
    return static_cast<int>((x - V4Output::FLUX_LOG_EMIN) * V4Output::FLUX_E_PER_DECADE);
}

constexpr int N_FLUX_E = static_cast<int>(
    (V4Output::FLUX_LOG_EMAX - V4Output::FLUX_LOG_EMIN) * V4Output::FLUX_E_PER_DECADE
);
} // namespace

const std::vector<double>& V4Output::FluxZEdges()
{
    static const std::vector<double> edges = [] {
        std::vector<double> e;
        for (int z = -150; z < 0; z += 2) {
            e.push_back(z);
        }
        for (double z : {0.0, 5.0, 10.0, 20.0, 50.0, 100.0, 150.0, 200.0, 300.0, 500.0,
                         1000.0, 2000.0})
        {
            e.push_back(z);
        }
        return e;
    }();
    return edges;
}

const std::vector<double>& V4Output::LineEnergies()
{
    static const std::vector<double> lines {0.511,  1.2946, 1.4608, 1.7788, 2.2232,
                                            2.6145, 3.5394, 4.4389, 4.9335, 6.1299,
                                            7.6311, 7.6455};
    return lines;
}

V4Output::V4Output(const V4Options& options) : options {options} {}

std::string V4Output::Prefix() const
{
    return options.outDir + "/" + options.tag;
}

void V4Output::Open()
{
    std::filesystem::create_directories(options.outDir);
    const auto mode = options.mode;
    const auto header = v4::MakeHeader("GC4TMP", 0);

    if ((mode == V4Options::Mode::Produce && options.writeRecords)
        || mode == V4Options::Mode::NormLibrary)
    {
        const std::string ext =
            mode == V4Options::Mode::Produce ? ".prod.bin" : ".nlib.bin";
        records.open(Prefix() + ext, std::ios::binary | std::ios::trunc);
        records.write(reinterpret_cast<const char*>(&header), sizeof(header));
        recordBuffer.reserve(1 << 16);
    }
    if (mode == V4Options::Mode::Produce || mode == V4Options::Mode::NormLibrary) {
        prodHist.assign(static_cast<std::size_t>(v4::N_ORIGIN_CLASSES) * N_EBINS, 0.0);
        prodDepth.assign(
            static_cast<std::size_t>(v4::N_ORIGIN_CLASSES) * N_EBINS_COARSE * N_DEPTH_COARSE,
            0.0
        );
        flux.assign((FluxZEdges().size() - 1) * N_FLUX_E, 0.0);
    }
    if (mode == V4Options::Mode::Transport) {
        tHist.assign(
            static_cast<std::size_t>(v4::N_ORIGIN_CLASSES) * options.batches * N_EBINS, 0.0
        );
        tDepth.assign(LineEnergies().size() * N_DEPTH_FINE, 0.0);
    }
    if (mode == V4Options::Mode::Analog) {
        pulses.open(Prefix() + ".pulses.bin", std::ios::binary | std::ios::trunc);
        tags.open(Prefix() + ".tags.bin", std::ios::binary | std::ios::trunc);
        pulses.write(reinterpret_cast<const char*>(&header), sizeof(header));
        tags.write(reinterpret_cast<const char*>(&header), sizeof(header));
    }
}

void V4Output::AddRecord(const v4::ProductionRecord& record)
{
    if (!records.is_open()) {
        return;
    }
    recordBuffer.push_back(record);
    nRecords++;
    if (recordBuffer.size() >= (1 << 16)) {
        records.write(
            reinterpret_cast<const char*>(recordBuffer.data()),
            static_cast<std::streamsize>(recordBuffer.size() * sizeof(v4::ProductionRecord))
        );
        recordBuffer.clear();
    }
}

void V4Output::FillProduction(int originClass, double E_MeV, double z_cm)
{
    const int e = EnergyBin(E_MeV, EBIN_MEV, N_EBINS);
    if (e >= 0) {
        prodHist[static_cast<std::size_t>(originClass) * N_EBINS + e] += 1.0;
    }
    const int ec = EnergyBin(E_MeV, 10 * EBIN_MEV, N_EBINS_COARSE);
    const int d = static_cast<int>(-z_cm / 5.0);
    if (ec >= 0 && d >= 0 && d < N_DEPTH_COARSE) {
        prodDepth[(static_cast<std::size_t>(originClass) * N_EBINS_COARSE + ec)
                      * N_DEPTH_COARSE
                  + d] += 1.0;
    }
}

void V4Output::FillFlux(double zMid_cm, double E_MeV, double length_cm)
{
    const int e = FluxEnergyBin(E_MeV);
    if (e < 0) {
        return;
    }
    const auto& edges = FluxZEdges();
    const auto it = std::upper_bound(edges.begin(), edges.end(), zMid_cm);
    if (it == edges.begin() || it == edges.end()) {
        return;
    }
    const auto zBin = static_cast<std::size_t>(it - edges.begin() - 1);
    flux[zBin * N_FLUX_E + e] += length_cm;
}

void V4Output::FillTransport(int originClass, int batch, double edep_MeV, double weight,
                             double zSource_cm)
{
    sumWeight += weight;
    const int e = EnergyBin(edep_MeV, EBIN_MEV, N_EBINS);
    if (e < 0) {
        return;
    }
    transportHits++;
    tHist[(static_cast<std::size_t>(originClass) * options.batches + batch) * N_EBINS + e] +=
        weight;

    const auto& lines = LineEnergies();
    for (std::size_t i = 0; i < lines.size(); i++) {
        if (std::abs(edep_MeV - lines[i]) <= LINE_HALF_WIDTH_MEV) {
            const int d = static_cast<int>(-zSource_cm);
            if (d >= 0 && d < N_DEPTH_FINE) {
                tDepth[i * N_DEPTH_FINE + d] += weight;
            }
        }
    }
}

void V4Output::AddPulse(const v4::AnalogPulse& pulse)
{
    pulses.write(reinterpret_cast<const char*>(&pulse), sizeof(pulse));
    nPulses++;
}

void V4Output::AddTag(const v4::TagCrossing& tag)
{
    tags.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
    nTags++;
}

void V4Output::WriteArray(const std::string& suffix, const std::vector<double>& data) const
{
    std::ofstream out(Prefix() + suffix, std::ios::binary | std::ios::trunc);
    out.write(
        reinterpret_cast<const char*>(data.data()),
        static_cast<std::streamsize>(data.size() * sizeof(double))
    );
    if (!out) {
        throw std::runtime_error("failed writing " + Prefix() + suffix);
    }
}

void V4Output::Close(double exposure_s, double sourceArea_cm2, std::uint64_t nEvents,
                     const std::string& extraJson)
{
    auto finishStream = [&](std::ofstream& stream, const char* magic, std::uint32_t size,
                            std::uint64_t count) {
        if (!stream.is_open()) {
            return;
        }
        auto header = v4::MakeHeader(magic, size);
        header.exposure_s = exposure_s;
        header.sourceArea_cm2 = sourceArea_cm2;
        header.nEvents = nEvents;
        header.nRecords = count;
        header.seed = options.seed;
        std::strncpy(header.soilName, options.tag.c_str(), sizeof(header.soilName) - 1);
        std::strncpy(header.extra, options.Describe().c_str(), sizeof(header.extra) - 1);
        stream.seekp(0);
        stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
        stream.close();
        if (!stream) {
            throw std::runtime_error("failed finishing a record stream");
        }
    };

    if (records.is_open()) {
        records.write(
            reinterpret_cast<const char*>(recordBuffer.data()),
            static_cast<std::streamsize>(recordBuffer.size() * sizeof(v4::ProductionRecord))
        );
        recordBuffer.clear();
        records.seekp(0, std::ios::end);
        const char* magic =
            options.mode == V4Options::Mode::Produce ? "GC4PROD" : "GC4NLIB";
        finishStream(records, magic, sizeof(v4::ProductionRecord), nRecords);
    }
    finishStream(pulses, "GC4PULS", sizeof(v4::AnalogPulse), nPulses);
    finishStream(tags, "GC4TAGS", sizeof(v4::TagCrossing), nTags);

    if (!prodHist.empty()) {
        WriteArray(".prodhist.bin", prodHist);
        WriteArray(".proddepth.bin", prodDepth);
    }
    if (options.mode == V4Options::Mode::Produce && options.fluxTally) {
        WriteArray(".flux.bin", flux);
    }
    if (!tHist.empty()) {
        WriteArray(".thist.bin", tHist);
        WriteArray(".tdepth.bin", tDepth);
    }

    std::ostringstream j;
    j.precision(17);
    j << "{\n  \"format\": \"gc4-manifest-1\",\n"
      << "  \"mode\": \"" << options.modeName << "\",\n"
      << "  \"tag\": \"" << options.tag << "\",\n"
      << "  \"soil\": \"" << options.soilPath << "\",\n"
      << "  \"seed\": " << options.seed << ",\n"
      << "  \"events\": " << nEvents << ",\n"
      << "  \"exposure_s\": " << exposure_s << ",\n"
      << "  \"source_area_cm2\": " << sourceArea_cm2 << ",\n"
      << "  \"options\": \"" << options.Describe() << "\",\n"
      << "  \"physics\": \"" << options.physics << "\",\n"
      << "  \"thermal_scattering\": " << (options.thermalScattering ? "true" : "false")
      << ",\n"
      << "  \"capture_model\": \"" << options.captureModel << "\",\n"
      << "  \"geant4_version\": \"" << G4Version << "\",\n"
      << "  \"records\": " << nRecords << ",\n"
      << "  \"pulses\": " << nPulses << ",\n"
      << "  \"tags\": " << nTags << ",\n"
      << "  \"transport_events\": " << transportEvents << ",\n"
      << "  \"transport_hits\": " << transportHits << ",\n"
      << "  \"transport_sum_weight\": " << sumWeight << ",\n"
      << "  \"shapes\": {\n"
      << "    \"n_classes\": " << v4::N_ORIGIN_CLASSES << ",\n"
      << "    \"n_ebins\": " << N_EBINS << ", \"ebin_mev\": " << EBIN_MEV << ",\n"
      << "    \"n_ebins_coarse\": " << N_EBINS_COARSE << ", \"n_depth_coarse\": "
      << N_DEPTH_COARSE << ",\n"
      << "    \"n_depth_fine\": " << N_DEPTH_FINE << ", \"batches\": " << options.batches
      << ",\n"
      << "    \"flux_e_per_decade\": " << FLUX_E_PER_DECADE << ", \"flux_log_emin\": "
      << FLUX_LOG_EMIN << ", \"flux_log_emax\": " << FLUX_LOG_EMAX << ",\n"
      << "    \"flux_z_edges_cm\": [";
    const auto& edges = FluxZEdges();
    for (std::size_t i = 0; i < edges.size(); i++) {
        j << (i ? ", " : "") << edges[i];
    }
    j << "],\n    \"line_energies_mev\": [";
    const auto& lines = LineEnergies();
    for (std::size_t i = 0; i < lines.size(); i++) {
        j << (i ? ", " : "") << lines[i];
    }
    j << "]\n  },\n  \"extra\": " << (extraJson.empty() ? "{}" : extraJson) << "\n}\n";

    std::ofstream manifest(Prefix() + ".json", std::ios::trunc);
    manifest << j.str();
    G4cout << "gc4: wrote " << Prefix() << ".json" << G4endl;
}
