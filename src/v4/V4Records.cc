#include "v4/V4Records.hh"

#include <cstring>

namespace v4
{

ProcCode ClassifyProcess(const std::string& name)
{
    if (name == "eBrem" || name == "muBrems" || name == "hBrems") {
        return PROC_EBREM;
    }
    if (name == "annihil") {
        return PROC_ANNIHIL;
    }
    if (name == "nCapture" || name == "nCaptureHP") {
        return PROC_NCAPTURE;
    }
    if (name == "neutronInelastic") {
        return PROC_NEUTRON_INELASTIC;
    }
    if (name == "Radioactivation" || name == "RadioactiveDecay"
        || name == "RadioactiveDecayBase")
    {
        return PROC_RADIOACTIVE_DECAY;
    }
    if (name == "muMinusCaptureAtRest") {
        return PROC_MU_CAPTURE;
    }
    if (name == "photonNuclear" || name == "electronNuclear"
        || name == "positronNuclear" || name == "muonNuclear")
    {
        return PROC_PHOTONUCLEAR;
    }
    if (name == "Decay") {
        return PROC_DECAY;
    }
    if (name.find("Inelastic") != std::string::npos) {
        return PROC_OTHER_INELASTIC;
    }
    if (name == "phot" || name == "compt" || name == "conv" || name == "eIoni"
        || name == "muIoni" || name == "hIoni" || name == "ionIoni"
        || name == "Rayl" || name == "msc")
    {
        return PROC_OTHER_EM;
    }
    if (name.find("Capture") != std::string::npos
        || name.find("Elastic") != std::string::npos
        || name.find("hadElastic") != std::string::npos)
    {
        return PROC_OTHER_HADRONIC;
    }
    return PROC_OTHER;
}

int OriginClass(std::int16_t proc, int targetZ)
{
    switch (proc) {
        case PROC_NCAPTURE:
            if (targetZ == 1) {
                return ORIGIN_CAPTURE_H;
            }
            if (targetZ == 14) {
                return ORIGIN_CAPTURE_SI;
            }
            if (targetZ == 26) {
                return ORIGIN_CAPTURE_FE;
            }
            return ORIGIN_CAPTURE_OTHER;
        case PROC_NEUTRON_INELASTIC:
        case PROC_OTHER_INELASTIC:
        case PROC_MU_CAPTURE:
        case PROC_PHOTONUCLEAR:
        case PROC_OTHER_HADRONIC:
            if (targetZ == 6) {
                return ORIGIN_INELASTIC_C;
            }
            if (targetZ == 8) {
                return ORIGIN_INELASTIC_O;
            }
            if (targetZ == 14) {
                return ORIGIN_INELASTIC_SI;
            }
            if (targetZ == 26) {
                return ORIGIN_INELASTIC_FE;
            }
            if (targetZ == 13) {
                return ORIGIN_INELASTIC_AL;
            }
            return ORIGIN_INELASTIC_OTHER;
        case PROC_RADIOACTIVE_DECAY:
            return ORIGIN_DECAY;
        default:
            return ORIGIN_EM_OTHER;
    }
}

FileHeader MakeHeader(const char* magic, std::uint32_t recordSize)
{
    FileHeader header {};
    std::memset(&header, 0, sizeof(header));
    std::strncpy(header.magic, magic, sizeof(header.magic) - 1);
    header.version = 1;
    header.recordSize = recordSize;
    return header;
}

} // namespace v4
