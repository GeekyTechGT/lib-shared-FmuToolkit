/// @file fmu_generation.hpp
/// @brief Public API for generating FMI modelDescription.xml and C accessors from JSON.
#pragma once

#include "export.hpp"
#include "types.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace libfmu_toolkit {

/// @brief Output selection and file naming options for JSON-driven generation.
struct LIB_FMU_TOOLKIT_API FmuArtifactGenerationOptions {
    std::filesystem::path outputDirectory{"."};
    std::string           modelDescriptionFileName{"modelDescription.xml"};
    std::string           cSourceFileName;
    bool                  generateModelDescription{true};
    bool                  generateCSource{true};
    bool                  overwrite{true};
};

/// @brief Paths of the artifacts generated from a JSON model description.
struct LIB_FMU_TOOLKIT_API GeneratedArtifactPaths {
    std::filesystem::path modelDescriptionXmlPath;
    std::filesystem::path cSourcePath;
};

/// @brief Generates a validated `modelDescription.xml` from a JSON definition.
class LIB_FMU_TOOLKIT_API ModelDescriptionGenerator {
public:
    [[nodiscard]] std::string generateFromJson(std::string_view jsonFilePath) const;

    [[nodiscard]] std::filesystem::path writeFromJson(
        std::string_view          jsonFilePath,
        std::filesystem::path     outputDirectory,
        std::string_view          fileName = "modelDescription.xml",
        bool                      overwrite = true) const;
};

/// @brief Generates FMI-oriented C getter/setter helpers from a JSON definition.
class LIB_FMU_TOOLKIT_API FmiCCodeGenerator {
public:
    [[nodiscard]] std::string generateFromJson(std::string_view jsonFilePath) const;

    [[nodiscard]] std::filesystem::path writeFromJson(
        std::string_view          jsonFilePath,
        std::filesystem::path     outputDirectory,
        std::string_view          fileName = {},
        bool                      overwrite = true) const;
};

/// @brief Facade that can generate only XML, only C code, or both.
class LIB_FMU_TOOLKIT_API FmuArtifactGenerator {
public:
    [[nodiscard]] GeneratedArtifactPaths generateFromJson(
        std::string_view                        jsonFilePath,
        const FmuArtifactGenerationOptions&     options = {}) const;
};

} // namespace libfmu_toolkit
