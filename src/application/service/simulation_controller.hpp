#pragma once

/// @file simulation_controller.hpp
/// @brief Use case: drive a full co-simulation lifecycle and record results.
/// @internal

#include "domain/model/fmu_package.hpp"
#include "domain/model/simulation_config.hpp"
#include "domain/model/simulation_result.hpp"
#include "domain/port/out/i_simulation_backend.hpp"
#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

namespace libfmu_toolkit {

/// @brief Runs a complete co-simulation from start time to stop time.
///
/// Drives the FMU through instantiate → initialize → step loop → terminate via
/// an injected ISimulationBackend, recording output values at the configured
/// recording interval. The run can be cancelled cooperatively from another
/// thread via cancel().
///
/// @par Requirements
/// Implements FR-04.1 … FR-04.6 (automated co-simulation) and NFR-03.2
/// (backend injection).
/// @req FR-04.1
class SimulationController {
public:
    /// @brief Constructs the controller with an injected simulation backend.
    /// @param backend Backend that performs the actual FMI calls.
    /// @req FR-04.1
    explicit SimulationController(ISimulationBackend& backend);

    /// @brief Runs the simulation and returns the recorded time series.
    /// @param pkg              The loaded FMU package to simulate.
    /// @param config           Timing, tolerances, initial values, recording policy.
    /// @param progressCallback Optional callback `(recordIndex, currentTime, totalSteps)`.
    /// @returns A SimulationResult with time points and per-variable series.
    /// @throws FmuSimulationException on a fatal FMI error during the run.
    /// @req FR-04.1
    [[nodiscard]] SimulationResult run(
        const FmuPackage& pkg,
        const SimulationConfig& config,
        std::function<void(size_t, double, size_t)> progressCallback = nullptr);

    /// @brief Requests cooperative cancellation of an in-progress run (thread-safe).
    /// @req FR-04.1
    void cancel() noexcept { cancelled_.store(true); }

private:
    ISimulationBackend& backend_;          ///< Injected simulation backend (not owned).
    std::atomic<bool> cancelled_{false};   ///< Cancellation flag polled by the step loop.

    /// @brief Records the current values of the tracked variables at time @p t.
    /// @param pkg      The FMU being simulated.
    /// @param config   The active simulation configuration.
    /// @param varIndex Name → variable lookup table for recorded outputs.
    /// @param result   Result accumulator to append to.
    /// @param t        Current simulation time (seconds).
    /// @req FR-04.4
    void recordStep(const FmuPackage& pkg,
                    const SimulationConfig& config,
                    const std::unordered_map<std::string, const ModelVariable*>& varIndex,
                    SimulationResult& result,
                    double t);
};

} // namespace libfmu_toolkit
