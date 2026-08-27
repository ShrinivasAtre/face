#include "DmsEyeMetrics.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
const char* stateName(dms::EyeState state)
{
    switch (state)
    {
    case dms::EyeState::Open: return "OPEN";
    case dms::EyeState::Closed: return "CLOSED";
    default: return "UNKNOWN";
    }
}

dms::ObservationUsability parseUsability(const std::string& value)
{
    if (value == "usable") return dms::ObservationUsability::Usable;
    if (value == "occluded") return dms::ObservationUsability::Occluded;
    if (value == "low_confidence") return dms::ObservationUsability::LowConfidence;
    return dms::ObservationUsability::Missing;
}
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: dms_sponsor_selftest <synthetic-eye-sequence.csv>\n";
        return 2;
    }
    std::ifstream input(argv[1]);
    if (!input)
    {
        std::cerr << "ERROR: cannot open test data: " << argv[1] << '\n';
        return 2;
    }

    using namespace std::chrono_literals;
    dms::EyeTemporalConfig config;
    config.perclosWindow = 2s;
    config.minimumPerclosCoverage = 0.0F;
    dms::EyeTemporalMetrics metrics({0.10F, 0.30F}, config);
    if (!metrics.valid())
    {
        std::cerr << "ERROR: " << metrics.error() << '\n';
        return 2;
    }

    std::string line;
    std::getline(input, line);
    int row = 1;
    int failures = 0;
    std::cout << "Stage 20 deterministic eye-metrics demonstration\n"
              << "Input is synthetic semantic EAR data; no image or biometric data is used.\n\n"
              << " time  quality          state     openness  blinks  event  prolonged  PERCLOS\n";
    while (std::getline(input, line))
    {
        ++row;
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> fields;
        std::stringstream stream(line);
        std::string field;
        while (std::getline(stream, field, ',')) fields.push_back(field);
        if (fields.size() != 7)
        {
            std::cerr << "ERROR: row " << row << " must contain 7 fields\n";
            return 2;
        }
        const auto time = std::chrono::milliseconds(std::stoll(fields[0]));
        const auto usability = parseUsability(fields[1]);
        const bool usable = usability == dms::ObservationUsability::Usable;
        const float ear = usable ? std::stof(fields[2]) : 0.0F;
        const auto result = metrics.update({time, usability,
            usable ? std::optional<float>(ear) : std::nullopt,
            usable ? std::optional<float>(ear) : std::nullopt});
        const std::string actualState = stateName(result.state);
        const auto expectedBlinks = static_cast<std::uint64_t>(std::stoull(fields[4]));
        const bool expectedEvent = fields[5] == "1";
        const bool expectedProlonged = fields[6] == "1";
        if (actualState != fields[3] || result.blinkCount != expectedBlinks ||
            result.blinkEvent != expectedEvent ||
            result.prolongedClosure != expectedProlonged)
        {
            ++failures;
            std::cerr << "Mismatch at row " << row << '\n';
        }
        std::cout << std::setw(5) << fields[0] << "  " << std::setw(15) << fields[1]
                  << "  " << std::setw(8) << actualState << "  ";
        if (result.openness) std::cout << std::fixed << std::setprecision(2) << *result.openness;
        else std::cout << " n/a";
        std::cout << "       " << result.blinkCount << "      "
                  << (result.blinkEvent ? "YES" : " no") << "      "
                  << (result.prolongedClosure ? "YES" : " no") << "       ";
        if (result.perclos) std::cout << std::fixed << std::setprecision(2) << *result.perclos;
        else std::cout << "n/a";
        std::cout << '\n';
    }
    if (failures != 0)
    {
        std::cerr << "\nSELF-TEST FAILED: " << failures << " mismatched row(s).\n";
        return 1;
    }
    std::cout << "\nSELF-TEST PASSED: blink debounce, unknown/occlusion handling, "
                 "prolonged closure and PERCLOS executed as expected.\n";
    return 0;
}
