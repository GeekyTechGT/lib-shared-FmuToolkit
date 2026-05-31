/// @file simulation_controller.cpp
/// @brief Implementation of the SimulationController use case (FR-04).
/// @internal

#include "simulation_controller.hpp"

#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace libfmu_toolkit {

SimulationController::SimulationController(ISimulationBackend& backend)
    : backend_(backend) {}

SimulationResult SimulationController::run(
    const FmuPackage& pkg,
    const SimulationConfig& config,
    std::function<void(size_t, double, size_t)> progressCallback)
{
    cancelled_.store(false);

    // Build a single O(1) name→variable lookup used by both parameter
    // initialisation and per-step recording.
    std::unordered_map<std::string, const ModelVariable*> varIndex;
    varIndex.reserve(pkg.variables.size());
    for (const auto& var : pkg.variables)
        varIndex[var.name] = &var;

    SimulationResult result;

    const size_t totalSteps = static_cast<size_t>(
        std::round((config.stopTime - config.startTime) / config.stepSize));

    backend_.initialize(pkg, config);

    for (const auto& [name, paramVal] : config.initialParameters) {
        auto it = varIndex.find(name);
        if (it != varIndex.end() && it->second->causality == Causality::Input) {
            FmiValue inVal = std::visit([](auto&& v) -> FmiValue {
                return FmiValue{v};
            }, paramVal);
            backend_.setInput(it->second->valueReference, it->second->dataType, inVal);
        }
    }

    size_t recordIndex = 0;

    for (size_t step = 0; step <= totalSteps; ++step) {
        if (cancelled_.load()) break;

        const double t = config.startTime + static_cast<double>(step) * config.stepSize;

        if (step % config.recordingInterval == 0) {
            recordStep(pkg, config, varIndex, result, t);
            if (progressCallback)
                progressCallback(recordIndex, t, totalSteps);
            ++recordIndex;
        }

        if (step < totalSteps) {
            bool ok = backend_.doStep(t, config.stepSize);
            if (!ok) break;
        }
    }

    backend_.terminate();
    return result;
}

void SimulationController::recordStep(
    const FmuPackage& pkg,
    const SimulationConfig& config,
    const std::unordered_map<std::string, const ModelVariable*>& varIndex,
    SimulationResult& result,
    double t)
{
    result.timePoints.push_back(t);

    if (!config.outputVariables.empty()) {
        for (const auto& varName : config.outputVariables) {
            auto it = varIndex.find(varName);
            if (it != varIndex.end()) {
                FmiValue val = backend_.getOutput(it->second->valueReference, it->second->dataType);
                OutputValue outVal = std::visit([](auto&& v) -> OutputValue {
                    return OutputValue{v};
                }, val);
                result.series[varName].push_back(outVal);
            }
        }
    } else {
        for (const auto* varPtr : pkg.outputs()) {
            FmiValue val = backend_.getOutput(varPtr->valueReference, varPtr->dataType);
            OutputValue outVal = std::visit([](auto&& v) -> OutputValue {
                return OutputValue{v};
            }, val);
            result.series[varPtr->name].push_back(outVal);
        }
    }
}

} // namespace libfmu_toolkit
