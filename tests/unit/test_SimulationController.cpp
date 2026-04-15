#include <gtest/gtest.h>

#include "core/domain/fmu_package.hpp"
#include "core/domain/simulation_config.hpp"
#include "core/domain/simulation_result.hpp"
#include "core/exceptions/fmu_exception.hpp"
#include "core/usecases/simulation_controller.hpp"
#include "ports/i_simulation_backend.hpp"

#include <atomic>
#include <cmath>
#include <map>
#include <stdexcept>

using namespace libfmu_toolkit;

// ---------------------------------------------------------------------------
// Mock simulation backend
// ---------------------------------------------------------------------------

class MockSimulationBackend final : public ISimulationBackend {
public:
    // Counters for call verification
    int initCount{0};
    int stepCount{0};
    int terminateCount{0};
    int setInputCount{0};
    int getOutputCount{0};

    bool shouldFailOnStep{false};   // throw on doStep
    bool shouldDiscardOnStep{false}; // return false on doStep (early termination)
    int  discardAfterSteps{-1};     // discard after N steps (-1 = never)

    // Tracks inputs set
    std::map<uint32_t, FmiValue> lastInputs;

    // Simulated output: returns currentTime_ for any real output request
    double currentTime_{0.0};

    void initialize(const FmuPackage& /*pkg*/,
                    const SimulationConfig& config) override {
        ++initCount;
        currentTime_ = config.startTime;
        initialized_ = true;
    }

    void setInput(uint32_t vr, DataType /*dtype*/, const FmiValue& value) override {
        ++setInputCount;
        lastInputs[vr] = value;
    }

    FmiValue getOutput(uint32_t /*vr*/, DataType /*dtype*/) override {
        ++getOutputCount;
        return FmiValue{currentTime_};
    }

    bool doStep(double currentTime, double stepSize) override {
        if (shouldFailOnStep)
            throw FmuSimulationException("mock: forced step failure");

        ++stepCount;

        if (discardAfterSteps >= 0 && stepCount > discardAfterSteps)
            return false;
        if (shouldDiscardOnStep)
            return false;

        currentTime_ = currentTime + stepSize;
        return true;
    }

    void terminate() override {
        ++terminateCount;
        initialized_ = false;
    }

    bool isInitialized() const noexcept override { return initialized_; }

private:
    bool initialized_{false};
};

// ---------------------------------------------------------------------------
// Helper to build a package with one output variable
// ---------------------------------------------------------------------------

namespace {

FmuPackage makeSimPackage() {
    FmuPackage pkg;
    pkg.fmiVersion     = FmiVersion::FMI2;
    pkg.modelName      = "MockModel";
    pkg.modelIdentifier = "MockModel";

    ModelVariable out;
    out.name           = "y";
    out.valueReference = 0;
    out.causality      = Causality::Output;
    out.variability    = Variability::Continuous;
    out.dataType       = DataType::Real;
    pkg.variables.push_back(std::move(out));

    ModelVariable inp;
    inp.name           = "u";
    inp.valueReference = 1;
    inp.causality      = Causality::Input;
    inp.variability    = Variability::Continuous;
    inp.dataType       = DataType::Real;
    pkg.variables.push_back(std::move(inp));

    return pkg;
}

SimulationConfig makeConfig(double start, double stop, double step) {
    SimulationConfig cfg;
    cfg.startTime = start;
    cfg.stopTime  = stop;
    cfg.stepSize  = step;
    cfg.recordingInterval = 1;
    return cfg;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(SimulationControllerTest, CallsInitializeAndTerminate) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 0.1, 0.01);

    (void)controller.run(pkg, cfg);

    EXPECT_EQ(backend.initCount,     1);
    EXPECT_EQ(backend.terminateCount, 1);
}

TEST(SimulationControllerTest, ProducesCorrectStepCount) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 1.0, 0.1);

    auto result = controller.run(pkg, cfg);

    // totalSteps = round((1.0-0.0)/0.1) = 10; recorded at steps 0,1,...,10 → 11 points
    EXPECT_EQ(result.stepCount(), 11u);
    EXPECT_EQ(backend.stepCount, 10);
}

TEST(SimulationControllerTest, TimePointsStartAtStartTime) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(2.0, 2.5, 0.1);

    auto result = controller.run(pkg, cfg);

    ASSERT_FALSE(result.timePoints.empty());
    EXPECT_NEAR(result.timePoints.front(), 2.0, 1e-12);
}

TEST(SimulationControllerTest, RecordsOutputVariable) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 0.3, 0.1);

    auto result = controller.run(pkg, cfg);

    EXPECT_FALSE(result.series.empty());
    EXPECT_NE(result.series.find("y"), result.series.end());
    EXPECT_EQ(result.series.at("y").size(), result.timePoints.size());
}

TEST(SimulationControllerTest, RecordingIntervalReducesDataPoints) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 1.0, 0.1);
    cfg.recordingInterval = 2; // record every 2 steps

    auto result = controller.run(pkg, cfg);

    // totalSteps=10; steps at 0,2,4,6,8,10 → 6 points
    EXPECT_EQ(result.stepCount(), 6u);
}

TEST(SimulationControllerTest, EarlyTerminationOnDiscardedStep) {
    MockSimulationBackend backend;
    backend.discardAfterSteps = 3; // discard after 3 steps
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 2.0, 0.1);

    auto result = controller.run(pkg, cfg);

    // Should stop before reaching the full 20 steps
    EXPECT_LT(result.stepCount(), 21u);
}

TEST(SimulationControllerTest, PropagatesProgressCallback) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 0.5, 0.1);

    int callbackInvocations = 0;
    (void)controller.run(pkg, cfg, [&](size_t, double, size_t) {
        ++callbackInvocations;
    });

    // 6 time points recorded → callback called 6 times
    EXPECT_EQ(callbackInvocations, 6);
}

TEST(SimulationControllerTest, SpecificOutputVariablesOnlyRecordRequested) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();

    // Add a second output variable
    ModelVariable y2;
    y2.name           = "y2";
    y2.valueReference = 2;
    y2.causality      = Causality::Output;
    y2.variability    = Variability::Continuous;
    y2.dataType       = DataType::Real;
    pkg.variables.push_back(std::move(y2));

    SimulationConfig cfg = makeConfig(0.0, 0.3, 0.1);
    cfg.outputVariables = {"y"}; // only record "y"

    auto result = controller.run(pkg, cfg);

    EXPECT_NE(result.series.find("y"), result.series.end());
    EXPECT_EQ(result.series.find("y2"), result.series.end());
}

TEST(SimulationControllerTest, CancelStopsSimulation) {
    MockSimulationBackend backend;
    SimulationController  controller(backend);

    FmuPackage pkg = makeSimPackage();
    SimulationConfig cfg = makeConfig(0.0, 10.0, 0.1);

    int callCount = 0;
    (void)controller.run(pkg, cfg, [&](size_t step, double, size_t) {
        ++callCount;
        if (step == 5) controller.cancel();
    });

    // Should stop well before the 101 full steps
    EXPECT_LT(callCount, 101);
    EXPECT_GT(callCount, 0);
}
