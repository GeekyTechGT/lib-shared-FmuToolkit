#pragma once

/// @file simulation_result.hpp
/// @brief Internal domain time-series result of a co-simulation run.
/// @internal
///
/// Internal counterpart of the public SimResult. Uses #OutputValue from the
/// public header so that no value conversion is needed when mapping to SimResult.

// OutputValue is defined in the public API header to ensure single definition.
#include <libfmu_toolkit/types.hpp>   // OutputValue

#include <map>
#include <string>
#include <vector>

namespace libfmu_toolkit {

/// @brief Recorded time points and per-variable value series of a simulation.
/// @req FR-04.6
struct SimulationResult {
    std::vector<double> timePoints; ///< Recorded time points, one per recording interval.
    /// @brief Per-variable time series: variable name → values aligned with #timePoints.
    std::map<std::string, std::vector<OutputValue>> series;

    /// @brief Returns the number of recorded time points.
    /// @req FR-04.6
    size_t stepCount() const noexcept { return timePoints.size(); }
    /// @brief Returns `true` if no time points were recorded.
    /// @req FR-04.6
    bool empty()       const noexcept { return timePoints.empty(); }
};

} // namespace libfmu_toolkit
