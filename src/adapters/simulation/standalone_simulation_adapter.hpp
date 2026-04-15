#pragma once
#include "ports/i_simulation_backend.hpp"

namespace libfmu_toolkit {

class StandaloneSimulationAdapter final : public ISimulationBackend {
public:
    StandaloneSimulationAdapter() = default;
    ~StandaloneSimulationAdapter() override;

    void initialize(const FmuPackage& pkg, const SimulationConfig& config) override;
    void setInput(uint32_t valueRef, DataType dtype, const FmiValue& value) override;
    FmiValue getOutput(uint32_t valueRef, DataType dtype) override;
    bool doStep(double currentTime, double stepSize) override;
    void terminate() override;
    bool isInitialized() const noexcept override { return initialized_; }

private:
    const FmuPackage* pkg_{nullptr};
    void* fmiComponent_{nullptr};
    bool initialized_{false};

    void initFmi2(const FmuPackage& pkg, const SimulationConfig& config);
    void setInputFmi2(uint32_t vr, DataType dtype, const FmiValue& value);
    FmiValue getOutputFmi2(uint32_t vr, DataType dtype);
    bool doStepFmi2(double t, double dt);
    void terminateFmi2();

    void initFmi3(const FmuPackage& pkg, const SimulationConfig& config);
    void setInputFmi3(uint32_t vr, DataType dtype, const FmiValue& value);
    FmiValue getOutputFmi3(uint32_t vr, DataType dtype);
    bool doStepFmi3(double t, double dt);
    void terminateFmi3();
};

} // namespace libfmu_toolkit
