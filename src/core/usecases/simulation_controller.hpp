#pragma once
#include "core/domain/fmu_package.hpp"
#include "core/domain/simulation_config.hpp"
#include "core/domain/simulation_result.hpp"
#include "ports/i_simulation_backend.hpp"
#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

namespace libfmu_toolkit {

class SimulationController {
public:
    explicit SimulationController(ISimulationBackend& backend);

    [[nodiscard]] SimulationResult run(
        const FmuPackage& pkg,
        const SimulationConfig& config,
        std::function<void(size_t, double, size_t)> progressCallback = nullptr);

    void cancel() noexcept { cancelled_.store(true); }

private:
    ISimulationBackend& backend_;
    std::atomic<bool> cancelled_{false};

    void recordStep(const FmuPackage& pkg,
                    const SimulationConfig& config,
                    const std::unordered_map<std::string, const ModelVariable*>& varIndex,
                    SimulationResult& result,
                    double t);
};

} // namespace libfmu_toolkit
