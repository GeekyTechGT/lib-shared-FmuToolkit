#include <gtest/gtest.h>

#include <filesystem>
#include <libfmu_toolkit/lib_fmu_toolkit.hpp>
#include <stdexcept>

TEST(FmuEngineTest, ConstructionSucceeds) {
    EXPECT_NO_THROW(libfmu_toolkit::FmuEngine engine);
}

TEST(FmuEngineTest, LoadNonExistentFmuThrowsFmuLoadError) {
    libfmu_toolkit::FmuEngine engine;
    EXPECT_THROW((void)engine.load("nonexistent.fmu"), libfmu_toolkit::FmuLoadError);
}

TEST(FmuEngineTest, LoadWrongExtensionThrowsFmuLoadError) {
    libfmu_toolkit::FmuEngine engine;
    EXPECT_THROW((void)engine.load("some_archive.zip"), libfmu_toolkit::FmuLoadError);
}

TEST(FmuEngineTest, UnloadUnknownHandleThrows) {
    libfmu_toolkit::FmuEngine engine;
    EXPECT_THROW(engine.unload(9999), std::out_of_range);
}

TEST(FmuEngineTest, InfoUnknownHandleThrows) {
    libfmu_toolkit::FmuEngine engine;
    EXPECT_THROW((void)engine.info(9999), std::out_of_range);
}

TEST(FmuEngineTest, ValidateUnknownHandleThrows) {
    libfmu_toolkit::FmuEngine engine;
    EXPECT_THROW((void)engine.validate(9999), std::out_of_range);
}

TEST(FmuEngineTest, SimulateUnknownHandleThrows) {
    libfmu_toolkit::FmuEngine engine;
    libfmu_toolkit::SimConfig cfg;
    EXPECT_THROW((void)engine.simulate(9999, cfg), std::out_of_range);
}

TEST(FmuEngineTest, ExportCsvEmptyResultCreatesHeaderOnlyFile) {
    libfmu_toolkit::SimResult empty;
    const std::filesystem::path tmpPath = "libfmu_toolkit_test_empty.csv";
    EXPECT_NO_THROW(libfmu_toolkit::FmuEngine::exportCsv(empty, tmpPath.string()));
    std::filesystem::remove(tmpPath);
}

TEST(FmuEngineIntegrationTest, LoadValidateAndSimulateReferenceFmu) {
    const auto fmuPath = std::filesystem::path{LIB_FMU_TOOLKIT_TEST_DATA_DIR} / "BouncingBall.fmu";
    ASSERT_TRUE(std::filesystem::exists(fmuPath));

    libfmu_toolkit::FmuEngine engine;
    const auto handle = engine.load(fmuPath.string());

    const auto info = engine.info(handle);
    EXPECT_EQ(info.fmiVersion, libfmu_toolkit::FmiVersion::FMI2);
    EXPECT_FALSE(info.modelName.empty());
    EXPECT_FALSE(info.variables.empty());

    const auto validation = engine.validate(handle);
    EXPECT_TRUE(validation.passed);
    EXPECT_FALSE(validation.hasErrors());

    libfmu_toolkit::SimConfig cfg;
    cfg.stopTime = 0.1;
    cfg.stepSize = 0.01;
    const auto result = engine.simulate(handle, cfg);
    EXPECT_FALSE(result.empty());
    EXPECT_GE(result.stepCount(), 5U);

    engine.unload(handle);
}
