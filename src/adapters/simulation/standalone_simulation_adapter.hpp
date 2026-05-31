#pragma once

/// @file standalone_simulation_adapter.hpp
/// @brief In-process FMI co-simulation backend implementing ISimulationBackend.
/// @internal

#include "ports/i_simulation_backend.hpp"

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

private:
    const FmuPackage* pkg_{nullptr};   ///< Non-owning pointer to the simulated package.
    void* fmiComponent_{nullptr};      ///< Opaque FMI component/instance handle.
    bool initialized_{false};          ///< `true` between successful init and terminate.

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
