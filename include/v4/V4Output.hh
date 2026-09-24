#pragma once

#include <G4Types.hh>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "v4/V4Options.hh"
#include "v4/V4Records.hh"

/**
 * @brief Everything a v4 run writes, shared by the user actions of one
 * (serial) run: record streams, histograms and the JSON manifest.
 *
 * Histogram layouts (row-major float64, described again in the manifest):
 *   produce    prodhist  [class][E: 1 keV bins, 0-12 MeV]
 *              proddepth [class][E: 10 keV bins][depth: 5 cm bins, 0-150 cm]
 *              flux      [z bin][log10(E/MeV) bin]  neutron track length, cm
 *   transport  thist     [class][batch][Edep: 1 keV bins, 0-12 MeV]  sum of w
 *              tdepth    [line][depth: 1 cm bins, 0-150 cm]           sum of w
 */
class V4Output
{
  public:
    explicit V4Output(const V4Options& options);

    static constexpr int N_EBINS = 12000;       // 1 keV, 0-12 MeV
    static constexpr double EBIN_MEV = 0.001;
    static constexpr int N_EBINS_COARSE = 1200; // 10 keV
    static constexpr int N_DEPTH_COARSE = 30;   // 5 cm, 0-150 cm
    static constexpr int N_DEPTH_FINE = 150;    // 1 cm
    static constexpr int FLUX_E_PER_DECADE = 20;
    static constexpr double FLUX_LOG_EMIN = -10.0; // log10(E/MeV)
    static constexpr double FLUX_LOG_EMAX = 4.0;

    static const std::vector<double>& FluxZEdges(); // cm, upward
    static const std::vector<double>& LineEnergies(); // MeV, for tdepth
    static constexpr double LINE_HALF_WIDTH_MEV = 0.002;

    void Open();
    void Close(double exposure_s, double sourceArea_cm2, std::uint64_t nEvents,
               const std::string& extraJson);

    // produce / norm-library
    void AddRecord(const v4::ProductionRecord& record);
    void FillProduction(int originClass, double E_MeV, double z_cm);
    void FillFlux(double zMid_cm, double E_MeV, double length_cm);

    // transport
    void FillTransport(int originClass, int batch, double edep_MeV, double weight,
                       double zSource_cm);
    void CountTransportEvent() { transportEvents++; }

    // analog
    void AddPulse(const v4::AnalogPulse& pulse);
    void AddTag(const v4::TagCrossing& tag);

    std::string Prefix() const;

  private:
    void WriteArray(const std::string& suffix, const std::vector<double>& data) const;

    const V4Options& options;
    std::ofstream records;
    std::ofstream pulses;
    std::ofstream tags;
    std::uint64_t nRecords = 0;
    std::uint64_t nPulses = 0;
    std::uint64_t nTags = 0;
    std::vector<v4::ProductionRecord> recordBuffer;

    std::vector<double> prodHist;
    std::vector<double> prodDepth;
    std::vector<double> flux;
    std::vector<double> tHist;
    std::vector<double> tDepth;
    std::uint64_t transportEvents = 0;
    std::uint64_t transportHits = 0;
    double sumWeight = 0.0;
};
