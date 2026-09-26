#include "v4/V4Actions.hh"

#include <G4Event.hh>
#include <G4Gamma.hh>
#include <G4HadronicProcess.hh>
#include <G4Ions.hh>
#include <G4Isotope.hh>
#include <G4Neutron.hh>
#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4SDManager.hh>
#include <G4Step.hh>
#include <G4SystemOfUnits.hh>
#include <G4Track.hh>
#include <G4TrackingManager.hh>
#include <G4VProcess.hh>
#include <G4ios.hh>

#include <algorithm>

#include "v4/V4SensitiveDetector.hh"

namespace
{
V4Generator* Generator()
{
    auto* action = G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction();
    return dynamic_cast<V4Generator*>(const_cast<G4VUserPrimaryGeneratorAction*>(action));
}

V4SensitiveDetector* GeSD()
{
    return dynamic_cast<V4SensitiveDetector*>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("GeSD", false)
    );
}

bool RecordsGammas(const V4Options& o)
{
    return o.mode == V4Options::Mode::Produce || o.mode == V4Options::Mode::NormLibrary;
}
} // namespace

/* ================================== Run ================================== */

void V4RunAction::BeginOfRunAction(const G4Run* /*run*/)
{
    start = std::chrono::steady_clock::now();
    shared->output.Open();
}

void V4RunAction::EndOfRunAction(const G4Run* /*run*/)
{
    const double wall = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - start
    )
                            .count();
    auto* generator = Generator();
    std::string extra = generator->ExtraJson();
    // splice the wall time into the generator's JSON object
    const std::string wallField = "\"wall_s\": " + std::to_string(wall);
    if (extra.size() > 2) {
        extra.insert(extra.size() - 1, ", " + wallField);
    }
    else {
        extra = "{" + wallField + "}";
    }
    shared->output.Close(
        generator->Exposure_s(), generator->SourceArea_cm2(), shared->eventsDone, extra
    );
    G4cout << "gc4: " << shared->eventsDone << " events, exposure "
           << generator->Exposure_s() << " s, wall " << wall << " s" << G4endl;
}

/* ================================= Event ================================= */

void V4EventAction::BeginOfEventAction(const G4Event* /*event*/)
{
    // The generator runs before this and has already filled shared->event
    // for this event, so it must not be reset here.
    shared->pending.clear();
    shared->decayCounter = 0;
}

void V4EventAction::EndOfEventAction(const G4Event* event)
{
    shared->eventsDone++;
    const auto& o = shared->options;
    if (o.printEvery > 0 && shared->eventsDone % o.printEvery == 0) {
        G4cout << "gc4: " << shared->eventsDone << " events" << G4endl;
    }

    if (o.exposure_s > 0.0 && Generator()->Exposure_s() >= o.exposure_s) {
        G4RunManager::GetRunManager()->AbortRun(true); // soft: finishes this event
    }

    if (o.mode == V4Options::Mode::Transport) {
        shared->output.CountTransportEvent();
        auto* sd = GeSD();
        const double edep = sd != nullptr ? sd->TotalEdep() / MeV : 0.0;
        if (edep > 0.001 && shared->event.weight > 0.0) {
            // weight is per event, set by V4TransportGenerator
            const auto& st = shared->event;
            if (o.debugWeight > 0.0 && st.weight > o.debugWeight) {
                G4cout << "HEAVY w=" << st.weight << " edep=" << edep << " E=" << st.eLeading_MeV
                       << " z=" << st.zSource_cm << " mu=" << st.muFirst << " x=" << st.x_cm
                       << " y=" << st.y_cm << " n=" << st.nGammas << " class=" << st.originClass
                       << " batch=" << st.batch << G4endl;
            }
            shared->output.FillTransport(
                shared->event.originClass,
                shared->event.batch,
                edep,
                shared->event.weight,
                shared->event.zSource_cm
            );
        }
    }
    else if (o.mode == V4Options::Mode::Analog) {
        auto* sd = GeSD();
        if (sd != nullptr) {
            for (const auto& [index, bucket] : sd->Buckets()) {
                v4::AnalogPulse pulse {};
                pulse.event = event->GetEventID();
                pulse.t_ns = static_cast<double>(index) * V4SensitiveDetector::BUCKET_NS;
                pulse.edep_MeV = static_cast<float>(
                    bucket.edepByOrigin[0] + bucket.edepByOrigin[1] + bucket.edepByOrigin[2]
                );
                pulse.edepSoilGamma_MeV = static_cast<float>(bucket.edepByOrigin[1]);
                pulse.edepSoilOther_MeV = static_cast<float>(bucket.edepByOrigin[2]);
                pulse.primaryPDG = bucket.primaryPDG;
                shared->output.AddPulse(pulse);
            }
        }
        for (const auto& tag : shared->tagBuffer) {
            shared->output.AddTag(tag);
        }
        shared->tagBuffer.clear();
    }
}

/* ================================ Stepping =============================== */

void V4SteppingAction::UserSteppingAction(const G4Step* step)
{
    const auto& o = shared->options;
    const auto* track = step->GetTrack();
    const auto* pre = step->GetPreStepPoint();
    const auto* post = step->GetPostStepPoint();

    if (o.mode == V4Options::Mode::Produce && o.fluxTally
        && track->GetDefinition() == G4Neutron::Definition())
    {
        const double zMid = 0.5 * (pre->GetPosition().z() + post->GetPosition().z()) / cm;
        shared->output.FillFlux(zMid, pre->GetKineticEnergy() / MeV, step->GetStepLength() / cm);
    }

    if (RecordsGammas(o)) {
        const auto* secondaries = step->GetSecondaryInCurrentStep();
        if (secondaries == nullptr || secondaries->empty()) {
            return;
        }
        if (o.mode == V4Options::Mode::NormLibrary) {
            // Chain decays are separated by up to Gyr, beyond what any float
            // time can resolve, so each radioactive decay gets an index and
            // every secondary inherits it (see V4TrackingAction).
            const auto* process = post->GetProcessDefinedStep();
            const auto* info = dynamic_cast<const V4TrackInfo*>(track->GetUserInformation());
            const std::int32_t primary = info != nullptr ? info->primaryPDG : 0;
            int decayId = info != nullptr ? info->decayId : 0;
            if (process != nullptr
                && v4::ClassifyProcess(process->GetProcessName()) == v4::PROC_RADIOACTIVE_DECAY)
            {
                // De-excitation of a short-lived excited daughter belongs to
                // the decay that fed it (e.g. Tl-208 -> Pb-208* -> 2614 + 583
                // keV in coincidence); ground states and isomers start a new
                // group.
                const auto* ion = dynamic_cast<const G4Ions*>(track->GetDefinition());
                const bool promptLevel = ion != nullptr && ion->GetExcitationEnergy() > 0.0
                    && track->GetDefinition()->GetPDGLifeTime() < 1.0 * microsecond;
                if (!promptLevel || decayId == 0) {
                    decayId = ++shared->decayCounter;
                }
            }
            for (const auto* secondary : *secondaries) {
                if (secondary->GetUserInformation() == nullptr) {
                    const_cast<G4Track*>(secondary)->SetUserInformation(
                        new V4TrackInfo(primary, v4::ORIGIN_SOIL_OTHER, decayId)
                    );
                }
            }
        }
        bool haveOrigin = false;
        V4Origin origin;
        for (const auto* secondary : *secondaries) {
            if (secondary->GetDefinition() != G4Gamma::Definition()
                || secondary->GetKineticEnergy() < o.minGamma_MeV * MeV)
            {
                continue;
            }
            if (!haveOrigin) {
                const auto* process = post->GetProcessDefinedStep();
                const std::string name =
                    process != nullptr ? std::string(process->GetProcessName()) : "";
                origin.proc = v4::ClassifyProcess(name);
                origin.parentPDG = track->GetDefinition()->GetPDGEncoding();
                origin.parentE_MeV = static_cast<float>(pre->GetKineticEnergy() / MeV);
                const auto* hadronic = dynamic_cast<const G4HadronicProcess*>(process);
                if (hadronic != nullptr) {
                    const auto* isotope =
                        const_cast<G4HadronicProcess*>(hadronic)->GetTargetIsotope();
                    if (isotope != nullptr) {
                        origin.targetZ = static_cast<std::uint8_t>(isotope->GetZ());
                        origin.targetA = static_cast<std::uint8_t>(
                            std::min(255, isotope->GetN())
                        );
                    }
                }
                else if (origin.proc == v4::PROC_RADIOACTIVE_DECAY) {
                    const auto* def = track->GetDefinition();
                    origin.targetZ = static_cast<std::uint8_t>(def->GetAtomicNumber());
                    origin.targetA =
                        static_cast<std::uint8_t>(std::min(255, def->GetAtomicMass()));
                }
                haveOrigin = true;
            }
            shared->pending[secondary] = origin;
        }
        return;
    }

    if (o.mode == V4Options::Mode::Analog) {
        const auto* postVolume = post->GetPhysicalVolume();
        const auto* preVolume = pre->GetPhysicalVolume();
        if (postVolume != nullptr && postVolume != preVolume
            && postVolume->GetName() == "TagVolume")
        {
            v4::TagCrossing tag {};
            tag.event = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
            tag.t_ns = post->GetGlobalTime() / ns;
            tag.E_MeV = static_cast<float>(post->GetKineticEnergy() / MeV);
            tag.pdg = track->GetDefinition()->GetPDGEncoding();
            shared->tagBuffer.push_back(tag);
        }
    }
}

/* ================================ Tracking =============================== */

void V4TrackingAction::PreUserTrackingAction(const G4Track* track)
{
    if (track->GetUserInformation() == nullptr) {
        // primaries (secondaries get theirs in PostUserTrackingAction)
        const int origin = track->GetPosition().z() < 0.0 ? v4::ORIGIN_SOIL_OTHER
                                                          : v4::ORIGIN_DIRECT;
        const_cast<G4Track*>(track)->SetUserInformation(
            new V4TrackInfo(track->GetDefinition()->GetPDGEncoding(), origin)
        );
    }
}

void V4TrackingAction::PostUserTrackingAction(const G4Track* track)
{
    const auto* info = dynamic_cast<const V4TrackInfo*>(track->GetUserInformation());
    const std::int32_t primary = info != nullptr ? info->primaryPDG : 0;
    const int parentOrigin = info != nullptr ? info->origin : v4::ORIGIN_DIRECT;
    auto* secondaries = fpTrackingManager->GimmeSecondaries();
    if (secondaries == nullptr) {
        return;
    }
    const int decayId = info != nullptr ? info->decayId : 0;
    for (auto* secondary : *secondaries) {
        if (secondary->GetUserInformation() != nullptr) {
            continue; // already tagged at creation (norm-library)
        }
        int origin = parentOrigin;
        if (origin == v4::ORIGIN_DIRECT && secondary->GetPosition().z() < 0.0) {
            origin = secondary->GetDefinition() == G4Gamma::Definition()
                ? v4::ORIGIN_SOIL_GAMMA
                : v4::ORIGIN_SOIL_OTHER;
        }
        else if (origin == v4::ORIGIN_SOIL_OTHER
                 && secondary->GetDefinition() == G4Gamma::Definition()
                 && secondary->GetPosition().z() < 0.0)
        {
            origin = v4::ORIGIN_SOIL_GAMMA;
        }
        secondary->SetUserInformation(new V4TrackInfo(primary, origin, decayId));
    }
}

/* ================================ Stacking =============================== */

G4ClassificationOfNewTrack V4StackingAction::ClassifyNewTrack(const G4Track* track)
{
    const auto& o = shared->options;
    auto found = shared->pending.find(track);
    if (found == shared->pending.end()) {
        return fUrgent;
    }
    const V4Origin origin = found->second;
    shared->pending.erase(found);

    const auto position = track->GetPosition();
    if (track->GetParentID() == 0 || position.z() > 0.0) {
        return fUrgent;
    }

    const auto* info = dynamic_cast<const V4TrackInfo*>(track->GetUserInformation());
    v4::ProductionRecord record {};
    record.event = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
    // GC4NLIB files store the decay index in t_ns (see V4Records.hh).
    record.t_ns = o.mode == V4Options::Mode::NormLibrary && info != nullptr
        ? static_cast<float>(info->decayId)
        : static_cast<float>(track->GetGlobalTime() / ns);
    record.E_MeV = static_cast<float>(track->GetKineticEnergy() / MeV);
    record.z_cm = static_cast<float>(position.z() / cm);
    record.mu = static_cast<float>(track->GetMomentumDirection().z());
    record.parentPDG = origin.parentPDG;
    record.parentE_MeV = origin.parentE_MeV;
    record.primaryPDG = info != nullptr ? info->primaryPDG : 0;
    record.proc = origin.proc;
    record.targetZ = origin.targetZ;
    record.targetA = origin.targetA;

    shared->output.AddRecord(record);
    shared->output.FillProduction(
        v4::OriginClass(record.proc, record.targetZ), record.E_MeV, record.z_cm
    );
    return fKill;
}

/* ============================ Initialization ============================= */

void V4ActionInitialization::Build() const
{
    const auto& o = shared->options;
    SetUserAction(new V4RunAction(shared));
    SetUserAction(new V4EventAction(shared));
    SetUserAction(new V4SteppingAction(shared));
    SetUserAction(new V4TrackingAction(shared));

    switch (o.mode) {
        case V4Options::Mode::Produce:
            SetUserAction(new V4StackingAction(shared));
            if (o.source == "cry") {
                SetUserAction(new V4CRYGenerator(o.crySetup, 0.0));
            }
            else {
                SetUserAction(new V4GunGenerator(o));
            }
            break;
        case V4Options::Mode::NormLibrary:
            SetUserAction(new V4StackingAction(shared));
            SetUserAction(new V4NormLibraryGenerator(o));
            break;
        case V4Options::Mode::Transport:
            SetUserAction(new V4TransportGenerator(o, shared->soil, &shared->event));
            break;
        case V4Options::Mode::Analog:
            SetUserAction(new V4CRYGenerator(o.crySetup, o.sourceHeight_m * 100.0));
            break;
    }
}
