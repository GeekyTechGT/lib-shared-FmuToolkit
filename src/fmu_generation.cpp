#include <libfmu_toolkit/fmu_generation.hpp>
#include <libfmu_toolkit/exceptions.hpp>

#include "adapters/xml/libxml2_validator_adapter.hpp"
#include "adapters/xml/model_description_parser.hpp"
#include "core/generation/fmu_model_description.hpp"
#include "core/exceptions/fmu_exception.hpp"

#include <fstream>

namespace libfmu_toolkit {

namespace {

[[nodiscard]] std::filesystem::path externalSchemaRoot() {
    return std::filesystem::path(LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR);
}

void writeTextFile(const std::filesystem::path& path, const std::string& content, bool overwrite) {
    if (!overwrite && std::filesystem::exists(path)) {
        throw FmuLoadException("Refusing to overwrite existing file: " + path.string());
    }
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        throw FmuLoadException("Cannot open output file for writing: " + path.string());
    }
    ofs << content;
}

void validateGeneratedXmlOrThrow(const std::string& xml, FmiVersion version) {
    Libxml2ValidatorAdapter validator;
    const auto schemaPath = externalSchemaRoot()
        / (version == FmiVersion::FMI2 ? "fmi-standard-2.0/schema/fmi2ModelDescription.xsd"
                                       : "fmi-standard-3.0/schema/fmi3ModelDescription.xsd");

    ValidationReport report = validator.validate(xml, schemaPath);
    if (report.hasErrors()) {
        throw FmuLoadException("Generated modelDescription.xml does not pass XSD validation: "
                               + report.errors.front().message);
    }

    FmuPackage pkg;
    ModelDescriptionParser::parse(xml, pkg);
    ModelDescriptionSemanticValidator::validateOrThrow(pkg);
}

[[nodiscard]] std::string defaultCFileName(const JsonModelDescription& model) {
    return sanitizeIdentifier(model.modelIdentifier) + "_accessors.c";
}

} // namespace

std::string ModelDescriptionGenerator::generateFromJson(std::string_view jsonFilePath) const {
    try {
        const JsonModelDescription model = JsonModelDescriptionParser::parseFile(std::filesystem::path(jsonFilePath));
        const std::string xml = GeneratedModelDescriptionBuilder::build(model);
        validateGeneratedXmlOrThrow(xml, model.fmiVersion);
        return xml;
    } catch (const FmuException& ex) {
        throw FmuLoadError(ex.what());
    }
}

std::filesystem::path ModelDescriptionGenerator::writeFromJson(
    std::string_view      jsonFilePath,
    std::filesystem::path outputDirectory,
    std::string_view      fileName,
    bool                  overwrite) const
{
    try {
        const auto path = outputDirectory / std::filesystem::path(fileName);
        const std::string xml = generateFromJson(jsonFilePath);
        writeTextFile(path, xml, overwrite);
        return path;
    } catch (const FmuException& ex) {
        throw FmuLoadError(ex.what());
    }
}

std::string FmiCCodeGenerator::generateFromJson(std::string_view jsonFilePath) const {
    try {
        const JsonModelDescription model = JsonModelDescriptionParser::parseFile(std::filesystem::path(jsonFilePath));
        return GeneratedCodeBuilder::build(model);
    } catch (const FmuException& ex) {
        throw FmuLoadError(ex.what());
    }
}

std::filesystem::path FmiCCodeGenerator::writeFromJson(
    std::string_view      jsonFilePath,
    std::filesystem::path outputDirectory,
    std::string_view      fileName,
    bool                  overwrite) const
{
    try {
        const JsonModelDescription model = JsonModelDescriptionParser::parseFile(std::filesystem::path(jsonFilePath));
        const auto outputFileName = fileName.empty() ? defaultCFileName(model) : std::string(fileName);
        const auto path = outputDirectory / outputFileName;
        writeTextFile(path, GeneratedCodeBuilder::build(model), overwrite);
        return path;
    } catch (const FmuException& ex) {
        throw FmuLoadError(ex.what());
    }
}

GeneratedArtifactPaths FmuArtifactGenerator::generateFromJson(
    std::string_view                    jsonFilePath,
    const FmuArtifactGenerationOptions& options) const
{
    try {
        if (!options.generateModelDescription && !options.generateCSource) {
            throw FmuLoadException("At least one artifact must be requested");
        }

        GeneratedArtifactPaths paths;
        const JsonModelDescription model = JsonModelDescriptionParser::parseFile(std::filesystem::path(jsonFilePath));

        if (options.generateModelDescription) {
            const auto path = options.outputDirectory / options.modelDescriptionFileName;
            const std::string xml = GeneratedModelDescriptionBuilder::build(model);
            validateGeneratedXmlOrThrow(xml, model.fmiVersion);
            writeTextFile(path, xml, options.overwrite);
            paths.modelDescriptionXmlPath = path;
        }

        if (options.generateCSource) {
            const auto fileName = options.cSourceFileName.empty() ? defaultCFileName(model) : options.cSourceFileName;
            const auto path = options.outputDirectory / fileName;
            writeTextFile(path, GeneratedCodeBuilder::build(model), options.overwrite);
            paths.cSourcePath = path;
        }

        return paths;
    } catch (const FmuException& ex) {
        throw FmuLoadError(ex.what());
    }
}

} // namespace libfmu_toolkit
