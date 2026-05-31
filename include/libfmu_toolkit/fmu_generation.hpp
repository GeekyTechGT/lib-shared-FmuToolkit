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
/// @req FR-08.6
struct LIB_FMU_TOOLKIT_API FmuArtifactGenerationOptions {
    std::filesystem::path outputDirectory{"."};                       ///< Destination directory for generated files.
    std::string           modelDescriptionFileName{"modelDescription.xml"}; ///< XML output file name.
    std::string           cSourceFileName;                            ///< C source file name (empty = derive from model identifier).
    bool                  generateModelDescription{true};             ///< Emit `modelDescription.xml` when `true`.
    bool                  generateCSource{true};                      ///< Emit the C accessor source when `true`.
    bool                  overwrite{true};                            ///< Overwrite existing files when `true`.
};

/// @brief Paths of the artifacts generated from a JSON model description.
/// @req FR-08.6
struct LIB_FMU_TOOLKIT_API GeneratedArtifactPaths {
    std::filesystem::path modelDescriptionXmlPath; ///< Path of the generated XML (empty if not generated).
    std::filesystem::path cSourcePath;             ///< Path of the generated C source (empty if not generated).
};

/// @brief Generates a validated `modelDescription.xml` from a JSON definition.
/// @req FR-08.2
class LIB_FMU_TOOLKIT_API ModelDescriptionGenerator {
public:
    /// @brief Generates the `modelDescription.xml` content as a string.
    /// @param jsonFilePath Path to the JSON model definition.
    /// @returns The serialized, semantically validated XML document.
    /// @throws FmuValidationError if the resulting model fails semantic validation.
    /// @req FR-08.2
    [[nodiscard]] std::string generateFromJson(std::string_view jsonFilePath) const;

    /// @brief Generates and writes `modelDescription.xml` to disk.
    /// @param jsonFilePath    Path to the JSON model definition.
    /// @param outputDirectory Destination directory.
    /// @param fileName        Output file name.
    /// @param overwrite       Overwrite an existing file when `true`.
    /// @returns The path of the written XML file.
    /// @req FR-08.2
    [[nodiscard]] std::filesystem::path writeFromJson(
        std::string_view          jsonFilePath,
        std::filesystem::path     outputDirectory,
        std::string_view          fileName = "modelDescription.xml",
        bool                      overwrite = true) const;
};

/// @brief Generates FMI-oriented C getter/setter helpers from a JSON definition.
/// @req FR-08.4
class LIB_FMU_TOOLKIT_API FmiCCodeGenerator {
public:
    /// @brief Generates the C accessor source as a string.
    /// @param jsonFilePath Path to the JSON model definition.
    /// @returns The generated C source text.
    /// @req FR-08.4
    [[nodiscard]] std::string generateFromJson(std::string_view jsonFilePath) const;

    /// @brief Generates and writes the C accessor source to disk.
    /// @param jsonFilePath    Path to the JSON model definition.
    /// @param outputDirectory Destination directory.
    /// @param fileName        Output file name (empty = `<modelIdentifier>_accessors.c`).
    /// @param overwrite       Overwrite an existing file when `true`.
    /// @returns The path of the written C source file.
    /// @req FR-08.4
    [[nodiscard]] std::filesystem::path writeFromJson(
        std::string_view          jsonFilePath,
        std::filesystem::path     outputDirectory,
        std::string_view          fileName = {},
        bool                      overwrite = true) const;
};

/// @brief Facade that can generate only XML, only C code, or both.
/// @req FR-08.6
class LIB_FMU_TOOLKIT_API FmuArtifactGenerator {
public:
    /// @brief Generates the artifacts selected by @p options from a JSON model.
    /// @param jsonFilePath Path to the JSON model definition.
    /// @param options      Output selection, file naming, and overwrite policy.
    /// @returns The paths of the generated artifacts (empty members when not generated).
    /// @req FR-08.6
    [[nodiscard]] GeneratedArtifactPaths generateFromJson(
        std::string_view                        jsonFilePath,
        const FmuArtifactGenerationOptions&     options = {}) const;
};

} // namespace libfmu_toolkit
