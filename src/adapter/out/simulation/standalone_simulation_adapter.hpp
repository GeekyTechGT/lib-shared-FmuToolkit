#pragma once

/// @file standalone_simulation_adapter.hpp
/// @brief In-process FMI co-simulation backend implementing ISimulationBackend.
/// @internal

#include "domain/port/out/i_simulation_backend.hpp"

namespace libfmu_toolkit {

/// @brief Drives a single FMU instance through the FMI co-simulation API.
///
/// Concrete ISimulationBackend that calls the resolved FMI 2.0 or FMI 3.0
/// function pointers directly (instantiate, initialize, set/get, doStep,
/// terminate). The FMI-version-specific work is delegated to the private
/// `*Fmi2`/`*Fmi3` helpers, selected once at initialization.
///
/// @par Requirements
/// Implements FR-04.1, FR-05.1 … FR-05.6 (co-simulation lifecycle, step-by-step
/// session backing) for both FMI 2.0 and FMI 3.0.
/// @req FR-04.1
class StandaloneSimulationAdapter final : public ISimulationBackend {
public:
    StandaloneSimulationAdapter() = default;
    ~StandaloneSimulationAdapter() override;

    /// @brief Instantiates and initializes the FMU instance.
    /// @req FR-04.1
    void initialize(const FmuPackage& pkg, const SimulationConfig& config) override;
    /// @brief Writes a typed value to an input/parameter by value reference.
    /// @req FR-05.3
    void setInput(uint32_t valueRef, DataType dtype, const FmiValue& value) override;
    /// @brief Reads a typed value from a variable by value reference.
    /// @req FR-05.4
    FmiValue getOutput(uint32_t valueRef, DataType dtype) override;
    /// @brief Advances the FMU by one communication step.
    /// @returns `false` if the FMU requests early termination.
    /// @req FR-05.2
    bool doStep(double currentTime, double stepSize) override;
    /// @brief Terminates and frees the FMU instance.
    /// @req FR-05.5
    void terminate() override;
    /// @brief Returns `true` once initialize() has succeeded.
    /// @req FR-05.1
    bool isInitialized() const noexcept override { return initialized_; }

    /// @name FMI 3.0 Event Mode, Binary and Clock access
    /// @{
    /// @brief One fmi3DoStep with its full outcome (FMI 2.0: a plain step reported as completed).
    /// @throws FmuSimulationException on a failing status or an unresolved function.
    StepOutcome doStepEx(double currentTime, double stepSize) override;
    /// @brief fmi3EnterEventMode. @throws FmuSimulationException if the session was opened without `eventModeUsed`.
    void enterEventMode() override;
    /// @brief fmi3EnterStepMode (Event Mode to Step Mode).
    void enterStepMode() override;
    /// @brief fmi3UpdateDiscreteStates; call again while `discreteStatesNeedUpdate` is set.
    DiscreteStatesOutcome updateDiscreteStates() override;
    /// @brief fmi3GetBinary: a copy of the buffer of one Binary variable (the FMU may reuse its own).
    std::vector<uint8_t> getBinary(uint32_t valueRef) override;
    /// @brief fmi3SetBinary: hands `bytes` to one Binary variable.
    void setBinary(uint32_t valueRef, std::span<const uint8_t> bytes) override;
    /// @brief fmi3GetClock: whether the Clock is active.
    bool getClock(uint32_t valueRef) override;
    /// @brief fmi3SetClock: activates or deactivates an input Clock (Event Mode).
    void setClock(uint32_t valueRef, bool active) override;
    /// @brief fmi3GetIntervalDecimal: seconds until the next tick of a Clock, with its qualifier.
    ClockInterval getClockInterval(uint32_t valueRef) override;
    /// @}

private:
    const FmuPackage* pkg_{nullptr};   ///< Non-owning pointer to the simulated package.
    void* fmiComponent_{nullptr};      ///< Opaque FMI component/instance handle.
    bool initialized_{false};          ///< `true` between successful init and terminate.
    bool eventModeUsed_{false};        ///< FMI 3.0 instance created with Event Mode.

    /// @brief Throws unless the FMI 3.0 Event Mode / Clock entry points were resolved and the instance is FMI 3.0.
    /// @brief Throws FmuSimulationException unless the instance is an initialised FMI 3.0 one that exports the Event Mode / Binary / Clock functions.
    /// @param operation Name of the calling operation, for the message.
    const FmuPackage::Fmi3Functions& requireEventApi(const char* operation) const;

    /// @name FMI 2.0 implementation helpers
    /// @{
    /// @brief FMI 2.0 instantiate + initialization sequence. @req FR-04.1
    void initFmi2(const FmuPackage& pkg, const SimulationConfig& config);
    /// @brief FMI 2.0 typed set by value reference. @req FR-05.3
    void setInputFmi2(uint32_t vr, DataType dtype, const FmiValue& value);
    /// @brief FMI 2.0 typed get by value reference. @req FR-05.4
    FmiValue getOutputFmi2(uint32_t vr, DataType dtype);
    /// @brief FMI 2.0 communication step. @req FR-05.2
    bool doStepFmi2(double t, double dt);
    /// @brief FMI 2.0 terminate + free instance. @req FR-05.5
    void terminateFmi2();
    /// @}

    /// @name FMI 3.0 implementation helpers
    /// @{
    /// @brief FMI 3.0 instantiate + initialization sequence. @req FR-04.1
    void initFmi3(const FmuPackage& pkg, const SimulationConfig& config);
    /// @brief FMI 3.0 typed set by value reference. @req FR-05.3
    void setInputFmi3(uint32_t vr, DataType dtype, const FmiValue& value);
    /// @brief FMI 3.0 typed get by value reference. @req FR-05.4
    FmiValue getOutputFmi3(uint32_t vr, DataType dtype);
    /// @brief FMI 3.0 communication step. @req FR-05.2
    bool doStepFmi3(double t, double dt);
    /// @brief FMI 3.0 terminate + free instance. @req FR-05.5
    void terminateFmi3();
    /// @}
};

} // namespace libfmu_toolkit
