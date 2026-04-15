#pragma once

// ParameterValue and LogLevel are defined in the public API header.
#include <libfmu_toolkit/types.hpp>   // ParameterValue, LogLevel, also re-exports all enums

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace libfmu_toolkit {

struct SimulationConfig {
    double startTime{0.0};
    double stopTime{1.0};
    double stepSize{0.001};
    double tolerance{1e-6};
    LogLevel logLevel{LogLevel::Warning};
    std::map<std::string, ParameterValue> initialParameters;
    std::vector<std::string> outputVariables;
    uint32_t recordingInterval{1};
};

} // namespace libfmu_toolkit
