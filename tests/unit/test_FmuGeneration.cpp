#include <gtest/gtest.h>

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path makeTempDir(const std::string& name) {
    const fs::path dir = fs::temp_directory_path() / ("lib_fmu_toolkit_" + name);
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

} // namespace

TEST(FmuGenerationTest, GeneratesValidatedFmi2ModelDescriptionFromJson) {
    const fs::path jsonPath = fs::path(LIB_FMU_TOOLKIT_SOURCE_DIR) / "data/generation/fmi2_from_json_example.json";

    libfmu_toolkit::ModelDescriptionGenerator generator;
    const std::string xml = generator.generateFromJson(jsonPath.string());

    EXPECT_NE(xml.find("fmiVersion=\"2.0\""), std::string::npos);
    EXPECT_NE(xml.find("name=\"u[1]\""), std::string::npos);
    EXPECT_NE(xml.find("name=\"u[2]\""), std::string::npos);
    EXPECT_NE(xml.find("<Outputs>"), std::string::npos);
    EXPECT_NE(xml.find("<InitialUnknowns>"), std::string::npos);
}

TEST(FmuGenerationTest, GeneratesFmi3ArraysWithoutScalarExpansion) {
    const fs::path jsonPath = fs::path(LIB_FMU_TOOLKIT_SOURCE_DIR) / "data/generation/fmi3_from_json_example.json";

    libfmu_toolkit::ModelDescriptionGenerator generator;
    const std::string xml = generator.generateFromJson(jsonPath.string());

    EXPECT_NE(xml.find("fmiVersion=\"3.0\""), std::string::npos);
    EXPECT_NE(xml.find("<Float64 name=\"u\""), std::string::npos);
    EXPECT_NE(xml.find("<Dimension start=\"3\"/>"), std::string::npos);
    EXPECT_EQ(xml.find("u[1]"), std::string::npos);
}

TEST(FmuGenerationTest, GeneratesCGetSetHelpersFromJson) {
    const fs::path jsonPath = fs::path(LIB_FMU_TOOLKIT_SOURCE_DIR) / "data/generation/fmi3_from_json_example.json";

    libfmu_toolkit::FmiCCodeGenerator generator;
    const std::string code = generator.generateFromJson(jsonPath.string());

    EXPECT_NE(code.find("typedef struct json_vector_fmi3Data"), std::string::npos);
    EXPECT_NE(code.find("json_vector_fmi3_get_float64"), std::string::npos);
    EXPECT_NE(code.find("json_vector_fmi3_set_float64"), std::string::npos);
    EXPECT_NE(code.find("weights[3]"), std::string::npos);
}

TEST(FmuGenerationTest, FacadeCanGenerateBothArtifactsToDisk) {
    const fs::path jsonPath = fs::path(LIB_FMU_TOOLKIT_SOURCE_DIR) / "data/generation/fmi2_from_json_example.json";
    const fs::path outDir   = makeTempDir("fmu_generation");

    libfmu_toolkit::FmuArtifactGenerator generator;
    libfmu_toolkit::FmuArtifactGenerationOptions options;
    options.outputDirectory = outDir;

    const auto paths = generator.generateFromJson(jsonPath.string(), options);

    EXPECT_TRUE(fs::exists(paths.modelDescriptionXmlPath));
    EXPECT_TRUE(fs::exists(paths.cSourcePath));
}
