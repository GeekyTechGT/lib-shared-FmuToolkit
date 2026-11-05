// =============================================================================
// Integration tests — Generatore.fmu and Sommatore.fmu (FMI 3.0)
//
// These tests require the real FMU shared libraries to be present and
// executable on the current platform (x86_64 Windows).  They exercise the
// complete load → validate → simulate → export lifecycle using two known
// FMI 3.0 models:
//
//   Generatore : output y = t * 5.0  (ramp generator, no inputs)
//   Sommatore  : output y = u1 + u2  (two-input adder)
//
// Each test includes a platform guard so it is skipped rather than failed
// when run on a platform where the DLLs are not compatible.
// =============================================================================

#include <gtest/gtest.h>

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

using namespace libfmu_toolkit;

// ---------------------------------------------------------------------------
// Helper — resolve a test-data FMU path and skip if it is missing
// ---------------------------------------------------------------------------

static std::filesystem::path fmuPath(const std::string& name) {
    return std::filesystem::path{LIB_FMU_TOOLKIT_TEST_DATA_DIR} / name;
}

#define SKIP_IF_FMU_MISSING(path)                                     \
    do {                                                               \
        if (!std::filesystem::exists(path)) {                         \
            GTEST_SKIP() << "FMU not found: " << (path).string();    \
        }                                                             \
    } while (false)

// ---------------------------------------------------------------------------
// Helper — extract a double from an OutputValue variant
// ---------------------------------------------------------------------------

static double asDouble(const OutputValue& v) {
    return std::visit([](auto&& x) -> double {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double>)  return x;
        if constexpr (std::is_same_v<T, int64_t>) return static_cast<double>(x);
        if constexpr (std::is_same_v<T, bool>)    return x ? 1.0 : 0.0;
        return 0.0;
    }, v);
}

// ===========================================================================
// Generatore.fmu — FMI 3.0 ramp generator: y = t * 5.0
// ===========================================================================

class GeneratoreTest : public ::testing::Test {
protected:
    const std::filesystem::path path = fmuPath("Generatore.fmu");
    FmuEngine engine;
    uint64_t  handle{0};

    void SetUp() override {
        SKIP_IF_FMU_MISSING(path);
        handle = engine.load(path.string());
    }
    void TearDown() override {
        if (handle) engine.unload(handle);
    }
};

TEST_F(GeneratoreTest, LoadSucceeds) {
    EXPECT_GT(handle, 0u);
}

TEST_F(GeneratoreTest, InfoReportsFMI3) {
    const auto info = engine.info(handle);
    EXPECT_EQ(info.fmiVersion, FmiVersion::FMI3);
}

TEST_F(GeneratoreTest, InfoModelName) {
    const auto info = engine.info(handle);
    EXPECT_EQ(info.modelName, "FmuBus");
}

TEST_F(GeneratoreTest, InfoModelIdentifier) {
    const auto info = engine.info(handle);
    EXPECT_EQ(info.modelIdentifier, "Generatore");
}

TEST_F(GeneratoreTest, InfoHasOutputVariable_y) {
    const auto info = engine.info(handle);
    const auto* y   = info.findVariable("y");
    ASSERT_NE(y, nullptr);
    EXPECT_EQ(y->causality, Causality::Output);
    EXPECT_EQ(y->dataType,  DataType::Float64);
}

TEST_F(GeneratoreTest, DefaultExperimentParsed) {
    const auto info = engine.info(handle);
    EXPECT_TRUE(info.defaultExperiment.hasStartTime);
    EXPECT_TRUE(info.defaultExperiment.hasStopTime);
    EXPECT_TRUE(info.defaultExperiment.hasStepSize);
    EXPECT_DOUBLE_EQ(info.defaultExperiment.startTime, 0.0);
    EXPECT_DOUBLE_EQ(info.defaultExperiment.stopTime,  30.0);
    EXPECT_DOUBLE_EQ(info.defaultExperiment.stepSize,  1.0);
}

TEST_F(GeneratoreTest, ValidationPasses) {
    const auto result = engine.validate(handle);
    for (const auto& e : result.errors)
        ADD_FAILURE() << "Validation error: " << e.message;
    EXPECT_TRUE(result.passed);
}

TEST_F(GeneratoreTest, SimulateRamp_OutputMatchesExpected) {
    // y(t) = t * 5.0  — simulate 5 steps of 1 second each
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 5.0;
    cfg.stepSize  = 1.0;

    const SimResult result = engine.simulate(handle, cfg);

    ASSERT_FALSE(result.empty());
    ASSERT_NE(result.series.find("y"), result.series.end());

    const auto& ySeries = result.series.at("y");
    ASSERT_EQ(ySeries.size(), result.timePoints.size());

    // Check ramp: y(t) = t * 5.0
    for (size_t i = 0; i < result.timePoints.size(); ++i) {
        const double t        = result.timePoints[i];
        const double expected = t * 5.0;
        const double actual   = asDouble(ySeries[i]);
        EXPECT_NEAR(actual, expected, 1e-6)
            << "at t=" << t << " expected y=" << expected << " got y=" << actual;
    }
}

TEST_F(GeneratoreTest, SimulateRecordedStepCount) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 10.0;
    cfg.stepSize  = 1.0;

    const SimResult result = engine.simulate(handle, cfg);
    // 11 recording points: t = 0, 1, 2, ..., 10
    EXPECT_EQ(result.stepCount(), 11u);
}

TEST_F(GeneratoreTest, ExportCsvContainsOutputColumn) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 3.0;
    cfg.stepSize  = 1.0;
    const SimResult result = engine.simulate(handle, cfg);

    const std::filesystem::path csvPath =
        std::filesystem::temp_directory_path() / "lib_fmu_toolkit_generatore.csv";
    ASSERT_NO_THROW(FmuEngine::exportCsv(result, csvPath.string()));

    std::ifstream ifs(csvPath);
    std::string header;
    std::getline(ifs, header);
    EXPECT_NE(header.find("time"), std::string::npos);
    EXPECT_NE(header.find("y"),    std::string::npos);

    std::error_code ec;
    std::filesystem::remove(csvPath, ec); // ignore errors (Windows AV may briefly lock the file)
}

TEST_F(GeneratoreTest, ExportJsonContainsTimeSeries) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 3.0;
    cfg.stepSize  = 1.0;
    const SimResult result = engine.simulate(handle, cfg);

    const std::filesystem::path jsonPath =
        std::filesystem::temp_directory_path() / "lib_fmu_toolkit_generatore.json";
    ASSERT_NO_THROW(FmuEngine::exportJson(result, jsonPath.string()));

    std::ifstream ifs(jsonPath);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("timePoints"), std::string::npos);
    EXPECT_NE(content.find("\"y\""),      std::string::npos);

    std::error_code ec;
    std::filesystem::remove(jsonPath, ec); // ignore errors (Windows AV may briefly lock the file)
}

TEST_F(GeneratoreTest, FmuSessionStepByStep) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 5.0;
    cfg.stepSize  = 1.0;

    FmuSession session = engine.openSession(handle, cfg);
    EXPECT_TRUE(session.isActive());

    double t = 0.0;
    for (int step = 0; step <= 5; ++step) {
        const OutputValue val = session.getValue("y");
        const double actual   = asDouble(val);
        const double expected = t * 5.0;
        EXPECT_NEAR(actual, expected, 1e-6)
            << "at step " << step << " (t=" << t << ")";

        if (step < 5) {
            EXPECT_TRUE(session.doStep(t, cfg.stepSize));
            t += cfg.stepSize;
        }
    }

    session.terminate();
    EXPECT_FALSE(session.isActive());
}

TEST_F(GeneratoreTest, UnloadReleasesHandle) {
    engine.unload(handle);
    handle = 0; // prevent double-unload in TearDown
    EXPECT_THROW((void)engine.info(0), std::out_of_range);
}

// ===========================================================================
// Sommatore.fmu — FMI 3.0 adder: y = u1 + u2
// ===========================================================================

class SommatoreTest : public ::testing::Test {
protected:
    const std::filesystem::path path = fmuPath("Sommatore.fmu");
    FmuEngine engine;
    uint64_t  handle{0};

    void SetUp() override {
        SKIP_IF_FMU_MISSING(path);
        handle = engine.load(path.string());
    }
    void TearDown() override {
        if (handle) engine.unload(handle);
    }
};

TEST_F(SommatoreTest, LoadSucceeds) {
    EXPECT_GT(handle, 0u);
}

TEST_F(SommatoreTest, InfoReportsFMI3) {
    EXPECT_EQ(engine.info(handle).fmiVersion, FmiVersion::FMI3);
}

TEST_F(SommatoreTest, InfoModelName) {
    EXPECT_EQ(engine.info(handle).modelName, "Sommatore");
}

TEST_F(SommatoreTest, InfoHasTwoInputs) {
    const auto inputs = engine.info(handle).inputs();
    ASSERT_EQ(inputs.size(), 2u);
    // Both must be named u1 and u2
    std::vector<std::string> names;
    for (const auto* v : inputs) names.push_back(v->name);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "u1") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "u2") != names.end());
}

TEST_F(SommatoreTest, InfoHasOutputVariable_y) {
    const auto* y = engine.info(handle).findVariable("y");
    ASSERT_NE(y, nullptr);
    EXPECT_EQ(y->causality, Causality::Output);
    EXPECT_EQ(y->dataType,  DataType::Float64);
}

TEST_F(SommatoreTest, DefaultExperimentParsed) {
    const auto& de = engine.info(handle).defaultExperiment;
    EXPECT_TRUE(de.hasStopTime);
    EXPECT_DOUBLE_EQ(de.stopTime,  10.0);
    EXPECT_DOUBLE_EQ(de.stepSize,  0.01);
}

TEST_F(SommatoreTest, ValidationPasses) {
    const auto result = engine.validate(handle);
    for (const auto& e : result.errors)
        ADD_FAILURE() << "Validation error: " << e.message;
    EXPECT_TRUE(result.passed);
}

TEST_F(SommatoreTest, SimulateWithZeroInputs_OutputIsZero) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 0.1;
    cfg.stepSize  = 0.01;

    const SimResult result = engine.simulate(handle, cfg);
    ASSERT_NE(result.series.find("y"), result.series.end());

    for (const auto& val : result.series.at("y")) {
        EXPECT_NEAR(asDouble(val), 0.0, 1e-9);
    }
}

TEST_F(SommatoreTest, SimulateWithInitialParams_OutputIsSum) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 0.1;
    cfg.stepSize  = 0.01;
    cfg.initialParameters["u1"] = 3.0;
    cfg.initialParameters["u2"] = 4.0;

    const SimResult result = engine.simulate(handle, cfg);
    ASSERT_NE(result.series.find("y"), result.series.end());

    const auto& ySeries = result.series.at("y");
    ASSERT_FALSE(ySeries.empty());
    // FMI co-simulation: at step 0 (before any doStep) the FMU's initial value
    // is returned (0.0); only after the first doStep with u1=3, u2=4 does y = 7.0.
    for (size_t i = 1; i < ySeries.size(); ++i) {
        EXPECT_NEAR(asDouble(ySeries[i]), 7.0, 1e-6)
            << "at series index " << i;
    }
}

TEST_F(SommatoreTest, SimulateRecordingInterval) {
    SimConfig cfg;
    cfg.startTime         = 0.0;
    cfg.stopTime          = 1.0;
    cfg.stepSize          = 0.01;
    cfg.recordingInterval = 10; // record every 10th step

    const SimResult result = engine.simulate(handle, cfg);
    // 101 steps → 11 recording points (steps 0, 10, 20, ..., 100)
    EXPECT_EQ(result.stepCount(), 11u);
}

TEST_F(SommatoreTest, SessionSetValueUpdatesOutput) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 1.0;
    cfg.stepSize  = 0.1;

    FmuSession session = engine.openSession(handle, cfg);
    ASSERT_TRUE(session.isActive());

    // Set u1=10, u2=5 → y should be 15
    session.setValue("u1", 10.0);
    session.setValue("u2",  5.0);
    (void)session.doStep(0.0, 0.1);

    const double y = asDouble(session.getValue("y"));
    EXPECT_NEAR(y, 15.0, 1e-6);

    session.terminate();
}

TEST_F(SommatoreTest, SessionNegativeInputs) {
    SimConfig cfg;
    cfg.startTime = 0.0;
    cfg.stopTime  = 0.5;
    cfg.stepSize  = 0.1;

    FmuSession session = engine.openSession(handle, cfg);
    session.setValue("u1", -3.5);
    session.setValue("u2",  1.5);
    (void)session.doStep(0.0, 0.1);

    EXPECT_NEAR(asDouble(session.getValue("y")), -2.0, 1e-6);
    session.terminate();
}

// ===========================================================================
// Cross-FMU — load both simultaneously and verify isolation
// ===========================================================================

TEST(CrossFmuTest, BothFmusLoadedInSameEngine) {
    const auto gPath = fmuPath("Generatore.fmu");
    const auto sPath = fmuPath("Sommatore.fmu");
    if (!std::filesystem::exists(gPath) || !std::filesystem::exists(sPath))
        GTEST_SKIP() << "One or both FMUs not found";

    FmuEngine engine;
    const uint64_t hGen = engine.load(gPath.string());
    const uint64_t hSum = engine.load(sPath.string());

    EXPECT_NE(hGen, hSum);

    const auto infoGen = engine.info(hGen);
    const auto infoSum = engine.info(hSum);

    EXPECT_EQ(infoGen.modelIdentifier, "Generatore");
    EXPECT_EQ(infoSum.modelName,       "Sommatore");

    // Simulate each independently
    SimConfig cfgGen;
    cfgGen.startTime = 0.0;
    cfgGen.stopTime  = 5.0;
    cfgGen.stepSize  = 1.0;

    SimConfig cfgSum;
    cfgSum.startTime = 0.0;
    cfgSum.stopTime  = 0.1;
    cfgSum.stepSize  = 0.01;
    cfgSum.initialParameters["u1"] = 2.0;
    cfgSum.initialParameters["u2"] = 3.0;

    const SimResult rGen = engine.simulate(hGen, cfgGen);
    const SimResult rSum = engine.simulate(hSum, cfgSum);

    EXPECT_FALSE(rGen.empty());
    EXPECT_FALSE(rSum.empty());

    // Generatore: y at t=5 should be 25
    EXPECT_NEAR(asDouble(rGen.series.at("y").back()), 25.0, 1e-5);
    // Sommatore: y should always be 5
    EXPECT_NEAR(asDouble(rSum.series.at("y").back()),  5.0, 1e-6);

    engine.unload(hGen);
    engine.unload(hSum);
}
