#include "v4/V4Generators.hh"

#include <CRYSetup.h>

#include <G4Event.hh>
#include <G4Gamma.hh>
#include <G4Geantino.hh>
#include <G4IonTable.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleTable.hh>
#include <G4PhysicalConstants.hh>
#include <G4RandomDirection.hh>
#include <G4SystemOfUnits.hh>
#include <Randomize.hh>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>

#include "RNGWrapper.hh"
#include "v4/V4DetectorConstruction.hh"

/* ================================== CRY ================================== */

V4CRYGenerator::V4CRYGenerator(const std::string& setupPath, double zOffset_cm)
    : gun {new G4ParticleGun()}
    , zOffset_cm {zOffset_cm}
{
    std::ifstream in(setupPath);
    if (!in) {
        throw std::runtime_error("cannot open CRY setup " + setupPath);
    }
    std::stringstream text;
    std::string line;
    while (std::getline(in, line)) {
        text << line << ' ';
        std::istringstream words(line);
        std::string key;
        if (words >> key && key == "subboxLength") {
            double length_m = 0.0;
            words >> length_m;
            boxLength_cm = length_m * 100.0;
        }
    }
    if (boxLength_cm <= 0.0) {
        throw std::runtime_error("CRY setup has no subboxLength: " + setupPath);
    }
    setupText = text.str();

#ifndef GC_CRY_DATA_PATH
#error "GC_CRY_DATA_PATH is not defined (set CRYHOME before running CMake)"
#endif
    auto* setup = new CRYSetup(setupText, GC_CRY_DATA_PATH);
    RNGWrapper<CLHEP::HepRandomEngine>::set(
        CLHEP::HepRandom::getTheEngine(), &CLHEP::HepRandomEngine::flat
    );
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
    generator = std::make_unique<CRYGenerator>(setup);
}

void V4CRYGenerator::GeneratePrimaries(G4Event* event)
{
    particles.clear();
    generator->genEvent(&particles);
    auto* table = G4ParticleTable::GetParticleTable();
    for (auto* p : particles) {
        gun->SetParticleDefinition(table->FindParticle(p->PDGid()));
        gun->SetParticleEnergy(p->ke() * MeV);
        gun->SetParticlePosition(
            G4ThreeVector(p->x() * m, p->y() * m, p->z() * m + zOffset_cm * cm)
        );
        gun->SetParticleMomentumDirection(G4ThreeVector(p->u(), p->v(), p->w()));
        gun->SetParticleTime(p->t());
        gun->GeneratePrimaryVertex(event);
        delete p;
    }
    particles.clear();
}

double V4CRYGenerator::Exposure_s() const
{
    return generator->timeSimulated();
}

std::string V4CRYGenerator::ExtraJson() const
{
    std::string escaped;
    for (char c : setupText) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        escaped += c;
    }
    return "{\"cry_setup\": \"" + escaped + "\", \"cry_z_offset_cm\": "
        + std::to_string(zOffset_cm) + "}";
}

/* ================================== Gun ================================== */

V4GunGenerator::V4GunGenerator(const V4Options& options)
    : gun {new G4ParticleGun()}
    , box_cm {options.gunBox_m * 100.0}
{
    auto* definition = G4ParticleTable::GetParticleTable()->FindParticle(options.gunParticle);
    if (definition == nullptr) {
        throw std::runtime_error("unknown gun particle " + options.gunParticle);
    }
    gun->SetParticleDefinition(definition);
    gun->SetParticleEnergy(options.gunEnergy_MeV * MeV);
    gun->SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
}

void V4GunGenerator::GeneratePrimaries(G4Event* event)
{
    const double x = (G4UniformRand() - 0.5) * box_cm;
    const double y = (G4UniformRand() - 0.5) * box_cm;
    gun->SetParticlePosition(G4ThreeVector(x * cm, y * cm, 0.1 * mm));
    gun->GeneratePrimaryVertex(event);
}

/* ============================== NORM library ============================= */

V4NormLibraryGenerator::V4NormLibraryGenerator(const V4Options& options)
    : gun {new G4ParticleGun()}
{
    if (options.isotope == "K40") {
        Z = 19;
        A = 40;
    }
    else if (options.isotope == "U238") {
        Z = 92;
        A = 238;
    }
    else {
        Z = 90;
        A = 232;
    }
    gun->SetParticleEnergy(0.0);
    gun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    // Depth is irrelevant to the library (only energies are used); 25 cm
    // keeps every beta and its bremsstrahlung inside the soil.
    gun->SetParticlePosition(G4ThreeVector(0, 0, -25 * cm));
}

void V4NormLibraryGenerator::GeneratePrimaries(G4Event* event)
{
    if (ion == nullptr) {
        ion = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
        gun->SetParticleDefinition(ion);
    }
    gun->SetParticleTime(0.0);
    gun->GeneratePrimaryVertex(event);
}

/* =============================== Transport =============================== */

namespace
{
// Mass attenuation coefficient (cm2/g) of a quartz-rich soil vs energy, used
// only to decide roulette survival; the answer stays unbiased if it is off.
double MuRho(double E_MeV)
{
    static const double E[] = {0.03, 0.1, 0.3, 0.5, 1.0, 1.5, 2.0,
                               3.0,  4.0, 5.0, 6.0, 8.0, 10., 20.};
    static const double M[] = {0.95,  0.17,  0.107, 0.0873, 0.0636, 0.0518, 0.0445,
                               0.0358, 0.0308, 0.0275, 0.0252, 0.0223, 0.0204, 0.017};
    const int n = sizeof(E) / sizeof(E[0]);
    if (E_MeV <= E[0]) {
        return M[0];
    }
    for (int i = 1; i < n; i++) {
        if (E_MeV <= E[i]) {
            const double f = std::log(E_MeV / E[i - 1]) / std::log(E[i] / E[i - 1]);
            return M[i - 1] * std::pow(M[i] / M[i - 1], f);
        }
    }
    return M[n - 1];
}

constexpr double SHADOW_MARGIN_CM = 3.0;
constexpr double UPWARD_MU_MIN = 0.05;
constexpr double ROULETTE_MIN_SURVIVAL = 1.0e-3;
} // namespace

V4TransportGenerator::V4TransportGenerator(
    const V4Options& options,
    const SoilModel& soil,
    V4EventState* state
)
    : options {options}
    , soil {soil}
    , state {state}
    , gun {new G4ParticleGun()}
    , input(options.input, std::ios::binary)
{
    if (!input) {
        throw std::runtime_error("cannot open transport input " + options.input);
    }
    input.read(reinterpret_cast<char*>(&inputHeader), sizeof(inputHeader));
    if (!input || inputHeader.recordSize != sizeof(v4::ProductionRecord)) {
        throw std::runtime_error("bad header in " + options.input);
    }
    if (std::strncmp(inputHeader.magic, "GC4PROD", 7) == 0) {
        isLibrary = false;
    }
    else if (std::strncmp(inputHeader.magic, "GC4NLIB", 7) == 0) {
        isLibrary = true;
        LoadLibrary();
        normArealMineral_g_cm2 = soil.PhaseAreal("mineral", options.normDepth_cm);
    }
    else {
        throw std::runtime_error("unknown input type in " + options.input);
    }
}

void V4TransportGenerator::LoadLibrary()
{
    std::vector<v4::ProductionRecord> all(inputHeader.nRecords);
    input.read(
        reinterpret_cast<char*>(all.data()),
        static_cast<std::streamsize>(all.size() * sizeof(v4::ProductionRecord))
    );
    if (!input) {
        throw std::runtime_error("short NORM library " + options.input);
    }
    std::map<std::int32_t, std::vector<v4::ProductionRecord>> byChain;
    for (const auto& r : all) {
        byChain[r.event].push_back(r);
    }
    libraryChains = inputHeader.nEvents;
    for (auto& [chain, recs] : byChain) {
        std::sort(recs.begin(), recs.end(), [](const auto& a, const auto& b) {
            return a.t_ns < b.t_ns;
        });
        // t_ns holds the decay index: one prompt group per decay
        std::vector<v4::ProductionRecord> group;
        for (const auto& r : recs) {
            if (!group.empty() && r.t_ns != group.front().t_ns) {
                groups.push_back(group);
                group.clear();
            }
            group.push_back(r);
        }
        if (!group.empty()) {
            groups.push_back(group);
        }
    }
    if (groups.empty()) {
        throw std::runtime_error("NORM library has no gammas: " + options.input);
    }
    G4cout << "gc4 transport: NORM library " << libraryChains << " chains, "
           << groups.size() << " prompt groups" << G4endl;
}

std::uint64_t V4TransportGenerator::PlannedEvents() const
{
    if (isLibrary) {
        return options.normSamples;
    }
    return inputHeader.nRecords * static_cast<std::uint64_t>(options.replays);
}

double V4TransportGenerator::BroadDensity(double x, double y) const
{
    const double R = options.rMax_m * 100.0;
    const double a = options.broadScale_cm;
    const double rho2 = x * x + y * y;
    if (rho2 > R * R) {
        return 0.0;
    }
    return 1.0 / (CLHEP::pi * std::log(1.0 + R * R / (a * a)) * (rho2 + a * a));
}

double V4TransportGenerator::ShadowDensity(double x, double y, const G4ThreeVector& d,
                                           double z) const
{
    // Points from which an unscattered ray along d can cross the detector:
    // the segment S(h) = -(h - z) tan(theta) u, h in [bottom, top], widened by
    // a disk of radius Rs. Sampling = uniform point on the segment + uniform
    // point in the disk, so q(x) = (length of segment within Rs of x) /
    // (L * pi * Rs^2).
    const double r = V4DetectorConstruction::DETECTOR_RADIUS_CM + SHADOW_MARGIN_CM;
    const double hb = V4DetectorConstruction::DETECTOR_ELEVATION_CM
        - V4DetectorConstruction::DETECTOR_HEIGHT_CM / 2;
    const double ht = hb + V4DetectorConstruction::DETECTOR_HEIGHT_CM;
    const double mu = d.z();
    const double sinT = std::sqrt(std::max(0.0, 1.0 - mu * mu));
    const double tanT = sinT / mu;
    const double ux = sinT > 0 ? d.x() / sinT : 1.0;
    const double uy = sinT > 0 ? d.y() / sinT : 0.0;
    const double s0x = -(hb - z) * tanT * ux;
    const double s0y = -(hb - z) * tanT * uy;
    const double L = (ht - hb) * tanT;
    const double area = CLHEP::pi * r * r;
    if (L < 1e-6) {
        const double dx = x - s0x;
        const double dy = y - s0y;
        return (dx * dx + dy * dy <= r * r) ? 1.0 / area : 0.0;
    }
    // segment direction is -u
    const double px = x - s0x;
    const double py = y - s0y;
    const double along = -(px * ux + py * uy);        // projection on -u
    const double perp2 = px * px + py * py - along * along;
    if (perp2 > r * r) {
        return 0.0;
    }
    const double half = std::sqrt(r * r - std::max(0.0, perp2));
    const double lo = std::max(0.0, along - half);
    const double hi = std::min(L, along + half);
    return hi > lo ? (hi - lo) / (L * area) : 0.0;
}

V4TransportGenerator::Placement
V4TransportGenerator::SamplePosition(const G4ThreeVector& d, double z_cm)
{
    Placement p {};
    if (options.sampler == "uniform") {
        const double R = options.rMax_m * 100.0;
        const double rho = R * std::sqrt(G4UniformRand());
        const double phi = CLHEP::twopi * G4UniformRand();
        p.x_cm = rho * std::cos(phi);
        p.y_cm = rho * std::sin(phi);
        p.invQ_cm2 = CLHEP::pi * R * R;
        return p;
    }

    const bool upward = d.z() > UPWARD_MU_MIN;
    const double ps = upward ? options.shadowFraction : 0.0;
    if (G4UniformRand() < ps) {
        shadowDraws++;
        const double r = V4DetectorConstruction::DETECTOR_RADIUS_CM + SHADOW_MARGIN_CM;
        const double hb = V4DetectorConstruction::DETECTOR_ELEVATION_CM
            - V4DetectorConstruction::DETECTOR_HEIGHT_CM / 2;
        const double ht = hb + V4DetectorConstruction::DETECTOR_HEIGHT_CM;
        const double mu = d.z();
        const double sinT = std::sqrt(std::max(0.0, 1.0 - mu * mu));
        const double tanT = sinT / mu;
        const double ux = sinT > 0 ? d.x() / sinT : 1.0;
        const double uy = sinT > 0 ? d.y() / sinT : 0.0;
        const double h = hb + G4UniformRand() * (ht - hb);
        const double cx = -(h - z_cm) * tanT * ux;
        const double cy = -(h - z_cm) * tanT * uy;
        const double rr = r * std::sqrt(G4UniformRand());
        const double phi = CLHEP::twopi * G4UniformRand();
        p.x_cm = cx + rr * std::cos(phi);
        p.y_cm = cy + rr * std::sin(phi);
    }
    else {
        const double R = options.rMax_m * 100.0;
        const double a = options.broadScale_cm;
        const double u = G4UniformRand();
        const double rho = a * std::sqrt(std::pow(1.0 + R * R / (a * a), u) - 1.0);
        const double phi = CLHEP::twopi * G4UniformRand();
        p.x_cm = rho * std::cos(phi);
        p.y_cm = rho * std::sin(phi);
    }
    double q = (1.0 - ps) * BroadDensity(p.x_cm, p.y_cm);
    if (ps > 0.0) {
        q += ps * ShadowDensity(p.x_cm, p.y_cm, d, z_cm);
    }
    p.invQ_cm2 = q > 0.0 ? 1.0 / q : 0.0;
    return p;
}

double V4TransportGenerator::RouletteWeight(double E_MeV, double z_cm)
{
    if (options.rouletteTau <= 0.0) {
        return 1.0;
    }
    const double tau = soil.MassDepth(z_cm) * MuRho(E_MeV);
    if (tau <= options.rouletteTau) {
        return 1.0;
    }
    const double survive =
        std::max(ROULETTE_MIN_SURVIVAL, std::exp(-(tau - options.rouletteTau)));
    if (G4UniformRand() < survive) {
        return 1.0 / survive;
    }
    rouletteKilled++;
    return 0.0;
}

bool V4TransportGenerator::NextProductionRecord(v4::ProductionRecord& out)
{
    input.read(reinterpret_cast<char*>(&out), sizeof(out));
    if (input.gcount() != static_cast<std::streamsize>(sizeof(out))) {
        return false;
    }
    recordsRead++;
    return true;
}

void V4TransportGenerator::GeneratePrimaries(G4Event* event)
{
    std::vector<const v4::ProductionRecord*> emit;
    v4::ProductionRecord single {};
    double baseWeight = 1.0;

    if (!isLibrary) {
        if (!haveCurrent || replayIndex >= options.replays) {
            if (!NextProductionRecord(current)) {
                throw std::runtime_error("transport ran past the end of " + options.input);
            }
            haveCurrent = true;
            replayIndex = 0;
        }
        replayIndex++;
        single = current;
        emit.push_back(&single);
        baseWeight = 1.0 / options.replays;
        state->batch = static_cast<int>(
            static_cast<std::uint32_t>(current.event) % static_cast<std::uint32_t>(options.batches)
        );
    }
    else {
        const auto index = static_cast<std::size_t>(G4UniformRand() * groups.size());
        const auto& group = groups[std::min(index, groups.size() - 1)];
        for (const auto& r : group) {
            emit.push_back(&r);
        }
        state->batch = static_cast<int>(samplesDone % options.batches);
        samplesDone++;
    }

    // depth and directions
    double z_cm = 0.0;
    if (!isLibrary) {
        z_cm = emit.front()->z_cm;
    }
    else {
        z_cm = soil.SamplePhaseDepth(
            "mineral", options.normDepth_cm, G4UniformRand(), G4UniformRand()
        );
    }
    std::vector<G4ThreeVector> directions;
    double maxE = 0.0;
    const v4::ProductionRecord* leading = emit.front();
    for (const auto* r : emit) {
        if (!isLibrary) {
            const double mu = std::clamp(static_cast<double>(r->mu), -1.0, 1.0);
            const double sinT = std::sqrt(1.0 - mu * mu);
            const double phi = CLHEP::twopi * G4UniformRand();
            directions.emplace_back(sinT * std::cos(phi), sinT * std::sin(phi), mu);
        }
        else {
            directions.push_back(G4RandomDirection());
        }
        if (r->E_MeV > maxE) {
            maxE = r->E_MeV;
            leading = r;
        }
    }

    const double roulette = RouletteWeight(maxE, z_cm);
    const Placement where = SamplePosition(directions.front(), z_cm);
    state->weight = baseWeight * roulette * where.invQ_cm2;
    state->originClass = v4::OriginClass(leading->proc, leading->targetZ);
    state->zSource_cm = z_cm;

    const G4ThreeVector position(where.x_cm * cm, where.y_cm * cm, z_cm * cm);
    if (state->weight <= 0.0) {
        // Killed by roulette (or outside every sampling region): a geantino
        // keeps the event well formed at negligible cost.
        gun->SetParticleDefinition(G4Geantino::Definition());
        gun->SetParticleEnergy(1.0 * MeV);
        gun->SetParticlePosition(position);
        gun->SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
        gun->GeneratePrimaryVertex(event);
        return;
    }
    gun->SetParticleDefinition(G4Gamma::Definition());
    for (std::size_t i = 0; i < emit.size(); i++) {
        gun->SetParticleEnergy(emit[i]->E_MeV * MeV);
        gun->SetParticleMomentumDirection(directions[i]);
        gun->SetParticlePosition(position);
        gun->SetParticleTime(0.0);
        gun->GeneratePrimaryVertex(event);
    }
}

std::string V4TransportGenerator::ExtraJson() const
{
    std::ostringstream j;
    j.precision(17);
    j << "{\"input\": \"" << options.input << "\", \"input_type\": \""
      << (isLibrary ? "norm-library" : "production") << "\", \"sampler\": \""
      << options.sampler << "\", \"replays\": " << options.replays
      << ", \"r_max_m\": " << options.rMax_m
      << ", \"shadow_fraction\": " << options.shadowFraction
      << ", \"broad_scale_cm\": " << options.broadScale_cm
      << ", \"roulette_tau\": " << options.rouletteTau
      << ", \"roulette_killed\": " << rouletteKilled
      << ", \"shadow_draws\": " << shadowDraws
      << ", \"input_exposure_s\": " << inputHeader.exposure_s
      << ", \"input_source_area_cm2\": " << inputHeader.sourceArea_cm2
      << ", \"input_events\": " << inputHeader.nEvents
      << ", \"input_records\": " << inputHeader.nRecords
      << ", \"records_read\": " << recordsRead;
    if (isLibrary) {
        // counts/s per (Bq of parent per kg of mineral) = factor * sum(w)
        const double arealBq_cm2 = normArealMineral_g_cm2 * 1.0e-3;
        const double factor = arealBq_cm2 * static_cast<double>(groups.size())
            / (static_cast<double>(libraryChains) * static_cast<double>(samplesDone));
        j << ", \"norm_depth_cm\": " << options.normDepth_cm
          << ", \"norm_mineral_areal_g_cm2\": " << normArealMineral_g_cm2
          << ", \"library_chains\": " << libraryChains
          << ", \"library_groups\": " << groups.size()
          << ", \"norm_samples\": " << samplesDone
          << ", \"norm_rate_factor\": " << factor;
    }
    j << "}";
    return j.str();
}
