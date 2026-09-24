#pragma once

#include <G4Types.hh>

#include <cstdint>
#include <string>

/**
 * @brief On-disk formats shared by the v4 (`gc4`) modes and read by
 * analysis/v4/gc4io.py. All little-endian, fixed-size, packed.
 */
namespace v4
{

/// Creator-process codes stored in ProductionRecord::proc.
enum ProcCode : std::int16_t
{
    PROC_OTHER = 0,
    PROC_EBREM = 1,
    PROC_ANNIHIL = 2,
    PROC_OTHER_EM = 3,
    PROC_NCAPTURE = 4,
    PROC_NEUTRON_INELASTIC = 5,
    PROC_OTHER_INELASTIC = 6,
    PROC_RADIOACTIVE_DECAY = 7,
    PROC_MU_CAPTURE = 8,
    PROC_PHOTONUCLEAR = 9,
    PROC_DECAY = 10,
    PROC_OTHER_HADRONIC = 11,
};

ProcCode ClassifyProcess(const std::string& processName);

/// Origin classes used to split transport histograms (see OriginClass()).
enum OriginClass : int
{
    ORIGIN_EM_OTHER = 0,
    ORIGIN_CAPTURE_H = 1,
    ORIGIN_CAPTURE_SI = 2,
    ORIGIN_CAPTURE_FE = 3,
    ORIGIN_CAPTURE_OTHER = 4,
    ORIGIN_INELASTIC_C = 5,
    ORIGIN_INELASTIC_O = 6,
    ORIGIN_INELASTIC_SI = 7,
    ORIGIN_INELASTIC_FE = 8,
    ORIGIN_INELASTIC_AL = 9,
    ORIGIN_INELASTIC_OTHER = 10,
    ORIGIN_DECAY = 11,
    N_ORIGIN_CLASSES = 12
};

int OriginClass(std::int16_t proc, int targetZ);

#pragma pack(push, 1)
/// One gamma born in the soil (production mode), 36 bytes.
struct ProductionRecord
{
    std::int32_t event;      ///< production event (shower / decay chain) id
    float t_ns;              ///< global time since the event started; in GC4NLIB
                             ///< files the index of the decay within the chain
    float E_MeV;             ///< kinetic energy at birth
    float z_cm;              ///< height of the birth point (<= 0 in soil)
    float mu;                ///< cos(polar angle) of the direction, +1 = up
    std::int32_t parentPDG;  ///< particle that made it
    float parentE_MeV;       ///< its kinetic energy at the start of that step
    std::int32_t primaryPDG; ///< cosmic primary (CRY) or parent nuclide (NORM)
    std::int16_t proc;       ///< ProcCode
    std::uint8_t targetZ;    ///< target nucleus of the hadronic interaction,
    std::uint8_t targetA;    ///< or the decaying ion; 0 for EM processes
};
#pragma pack(pop)
static_assert(sizeof(ProductionRecord) == 36, "ProductionRecord must be 36 bytes");

#pragma pack(push, 1)
/// Header of every gc4 binary file, 256 bytes. Rewritten at end of run.
struct FileHeader
{
    char magic[8];            ///< "GC4PROD", "GC4NLIB", "GC4ANLG"
    std::uint32_t version;    ///< format version (1)
    std::uint32_t recordSize; ///< bytes per record
    double exposure_s;        ///< simulated real time (CRY) or 0 (library)
    double sourceArea_cm2;    ///< area the primaries were spread over
    std::uint64_t nEvents;    ///< primaries simulated
    std::uint64_t nRecords;   ///< records that follow
    std::uint64_t seed;       ///< RNG seed of the run
    char soilName[64];
    char extra[136];          ///< free text (key=value;...), NUL padded
};
#pragma pack(pop)
static_assert(sizeof(FileHeader) == 256, "FileHeader must be 256 bytes");

#pragma pack(push, 1)
/// One pulse in analog mode: detector energy in a 100 ns time bucket.
struct AnalogPulse
{
    std::int64_t event;
    double t_ns;              ///< start of the bucket
    float edep_MeV;           ///< all energy in the bucket
    float edepSoilGamma_MeV;  ///< carried by descendants of soil-born gammas
    float edepSoilOther_MeV;  ///< by descendants of other soil-born particles
    std::int32_t primaryPDG;  ///< cosmic primary behind the largest deposit
};
#pragma pack(pop)
static_assert(sizeof(AnalogPulse) == 32, "AnalogPulse must be 32 bytes");

/// Origin of a track in analog mode (V4TrackInfo::origin).
enum TrackOrigin : int
{
    ORIGIN_DIRECT = 0,     ///< never passed through a soil-born ancestor
    ORIGIN_SOIL_GAMMA = 1, ///< descends from a gamma born in the soil
    ORIGIN_SOIL_OTHER = 2  ///< descends from another particle born in the soil
};

#pragma pack(push, 1)
/// A particle entering the virtual tag volume above the detector (analog).
struct TagCrossing
{
    std::int64_t event;
    double t_ns;
    float E_MeV;
    std::int32_t pdg;
};
#pragma pack(pop)
static_assert(sizeof(TagCrossing) == 24, "TagCrossing must be 24 bytes");

FileHeader MakeHeader(const char* magic, std::uint32_t recordSize);

} // namespace v4
