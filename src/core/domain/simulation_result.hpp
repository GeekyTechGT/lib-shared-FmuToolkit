#pragma once

// OutputValue is defined in the public API header to ensure single definition.
#include <libfmu_toolkit/types.hpp>   // OutputValue

#include <map>
#include <string>
#include <vector>

namespace libfmu_toolkit {

struct SimulationResult {
    std::vector<double> timePoints;
    std::map<std::string, std::vector<OutputValue>> series;

    size_t stepCount() const noexcept { return timePoints.size(); }
    bool empty()       const noexcept { return timePoints.empty(); }
};

} // namespace libfmu_toolkit
