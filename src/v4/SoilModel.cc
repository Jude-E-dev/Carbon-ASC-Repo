#include "v4/SoilModel.hh"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
constexpr G4double SUM_TOLERANCE = 1.0e-6;

std::runtime_error ParseError(const std::string& path, int line, const std::string& what)
{
    return std::runtime_error(
        "SoilModel: " + path + ":" + std::to_string(line) + ": " + what
    );
}

void CheckSum(
    const std::vector<std::pair<std::string, G4double>>& parts,
    const std::string& path,
    int line
)
{
    G4double sum = 0.0;
    for (const auto& part : parts) {
        if (part.second < 0.0) {
            throw ParseError(path, line, "negative mass fraction for " + part.first);
        }
        sum += part.second;
    }
    if (std::abs(sum - 1.0) > SUM_TOLERANCE) {
        std::ostringstream msg;
        msg << "mass fractions sum to " << sum << ", not 1";
        throw ParseError(path, line, msg.str());
    }
}
} // namespace

G4double SoilLayer::PhaseMassFraction(const std::string& phase) const
{
    for (const auto& part : phases) {
        if (part.first == phase) {
            return part.second;
        }
    }
    return 0.0;
}

SoilModel SoilModel::Load(const std::string& path)
{
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("SoilModel: cannot open " + path);
    }

    SoilModel model;
    std::string raw;
    int lineNo = 0;
    bool sawHeader = false;
    bool sawEnd = false;

    while (std::getline(in, raw)) {
        lineNo++;
        const auto hash = raw.find('#');
        if (hash != std::string::npos) {
            raw.erase(hash);
        }
        std::istringstream line(raw);
        std::string key;
        if (!(line >> key)) {
            continue;
        }

        if (key == "gc-soil") {
            int version = 0;
            line >> version;
            if (version != 1) {
                throw ParseError(path, lineNo, "unsupported gc-soil version");
            }
            sawHeader = true;
        }
        else if (key == "name") {
            line >> model.name;
        }
        else if (key == "phase") {
            SoilPhase phase;
            int n = 0;
            line >> phase.name >> phase.density_g_cm3 >> n;
            for (int i = 0; i < n; i++) {
                std::string el;
                G4double frac = NAN;
                line >> el >> frac;
                phase.elements.emplace_back(el, frac);
            }
            if (!line || phase.density_g_cm3 <= 0.0) {
                throw ParseError(path, lineNo, "malformed phase");
            }
            CheckSum(phase.elements, path, lineNo);
            model.phases.push_back(phase);
        }
        else if (key == "layer") {
            SoilLayer layer;
            int n = 0;
            line >> layer.zTop_cm >> layer.zBottom_cm >> layer.density_g_cm3 >> n;
            for (int i = 0; i < n; i++) {
                std::string ph;
                G4double frac = NAN;
                line >> ph >> frac;
                layer.phases.emplace_back(ph, frac);
            }
            if (!line || layer.zBottom_cm >= layer.zTop_cm
                || layer.density_g_cm3 <= 0.0) {
                throw ParseError(path, lineNo, "malformed layer");
            }
            CheckSum(layer.phases, path, lineNo);
            model.layers.push_back(layer);
        }
        else if (key == "end") {
            sawEnd = true;
            break;
        }
        else {
            throw ParseError(path, lineNo, "unknown key '" + key + "'");
        }
    }

    if (!sawHeader || !sawEnd || model.layers.empty() || model.name.empty()) {
        throw std::runtime_error(
            "SoilModel: " + path + " is incomplete (header, name, layers, end)"
        );
    }

    // Layers must tile the column from z = 0 downwards with no gaps.
    G4double expectTop = 0.0;
    for (const auto& layer : model.layers) {
        if (std::abs(layer.zTop_cm - expectTop) > 1e-9) {
            throw std::runtime_error("SoilModel: layers of " + path + " are not contiguous");
        }
        for (const auto& part : layer.phases) {
            if (model.FindPhase(part.first) == nullptr) {
                throw std::runtime_error(
                    "SoilModel: layer references unknown phase " + part.first
                );
            }
        }
        expectTop = layer.zBottom_cm;
    }
    return model;
}

const SoilPhase* SoilModel::FindPhase(const std::string& phaseName) const
{
    for (const auto& phase : phases) {
        if (phase.name == phaseName) {
            return &phase;
        }
    }
    return nullptr;
}

G4double SoilModel::Depth_cm() const
{
    return -layers.back().zBottom_cm;
}

G4double SoilModel::MassDepth(G4double z_cm) const
{
    G4double total = 0.0;
    for (const auto& layer : layers) {
        if (z_cm >= layer.zTop_cm) {
            break;
        }
        const G4double lo = std::max(z_cm, layer.zBottom_cm);
        total += layer.density_g_cm3 * (layer.zTop_cm - lo);
        if (z_cm >= layer.zBottom_cm) {
            break;
        }
    }
    return total;
}

G4double SoilModel::PhaseAreal(const std::string& phase, G4double maxDepth_cm) const
{
    G4double total = 0.0;
    for (const auto& layer : layers) {
        const G4double top = layer.zTop_cm;
        const G4double bottom = std::max(layer.zBottom_cm, -maxDepth_cm);
        if (bottom >= top) {
            break;
        }
        total += layer.density_g_cm3 * layer.PhaseMassFraction(phase) * (top - bottom);
    }
    return total;
}

G4double SoilModel::SamplePhaseDepth(
    const std::string& phase,
    G4double maxDepth_cm,
    G4double u1,
    G4double u2
) const
{
    const G4double total = PhaseAreal(phase, maxDepth_cm);
    if (total <= 0.0) {
        throw std::runtime_error("SoilModel: no " + phase + " to sample from");
    }
    G4double target = u1 * total;
    for (const auto& layer : layers) {
        const G4double top = layer.zTop_cm;
        const G4double bottom = std::max(layer.zBottom_cm, -maxDepth_cm);
        if (bottom >= top) {
            break;
        }
        const G4double mass =
            layer.density_g_cm3 * layer.PhaseMassFraction(phase) * (top - bottom);
        if (target <= mass || bottom <= -maxDepth_cm) {
            return top - u2 * (top - bottom);
        }
        target -= mass;
    }
    return -maxDepth_cm * u2;
}
