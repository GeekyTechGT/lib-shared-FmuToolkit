#pragma once

/// @file simulation_config.hpp
/// @brief Internal domain configuration for a co-simulation run.
/// @internal
///
/// This is the internal counterpart of the public SimConfig. The PIMPL facade
/// maps SimConfig to SimulationConfig before handing it to the use-case layer,
/// keeping the domain free of any public-API coupling beyond the shared scalar
/// value types (#ParameterValue, #LogLevel) defined in `types.hpp`.

// ParameterValue and LogLevel are defined in the public API header.
#include <libfmu_toolkit/types.hpp>   // ParameterValue, LogLevel, also re-exports all enums

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace libfmu_toolkit {

/// @brief Timing, tolerance, initial values and recording policy for a simulation.
/// @req FR-04.1
struct SimulationConfig {
    double startTime{0.0};   ///< Simulation start time (seconds).
    double stopTime{1.0};    ///< Simulation stop time (seconds).
    double stepSize{0.001};  ///< Communication step size (seconds).
    double tolerance{1e-6};  ///< Solver relative tolerance.
    LogLevel logLevel{LogLevel::Warning}; ///< Verbosity of FMU-internal log output.

    /// @brief Initial values for parameters/inputs, applied before exiting init mode.
    std::map<std::string, ParameterValue> initialParameters;
    /// @brief Output variables to record; empty means record all outputs.
    std::vector<std::string> outputVariables;
    /// @brief Record every N-th communication step (1 = every step).
    uint32_t recordingInterval{1};
};

} // namespace libfmu_toolkit
