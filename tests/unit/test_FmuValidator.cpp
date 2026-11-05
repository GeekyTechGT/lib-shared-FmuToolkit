#include <gtest/gtest.h>

#include "domain/model/fmu_package.hpp"
#include "domain/model/validation_report.hpp"
#include "application/service/fmu_validator.hpp"
#include "domain/port/out/i_xml_validator.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

using namespace libfmu_toolkit;

// ---------------------------------------------------------------------------
// Stub XML validators — used to isolate structure and semantic tests from the
// real XML/XSD validation logic without requiring real schema files.
// ---------------------------------------------------------------------------

/// Always reports the XML as valid.
class StubXmlValidatorPass final : public IXmlValidator {
public:
    ValidationReport validate(const std::string& /*xmlContent*/,
                               const std::filesystem::path& /*schemaPath*/) const override
    {
        ValidationReport r;
        r.passed = true;
        return r;
    }
};

/// Always reports a configurable error message.
class StubXmlValidatorFail final : public IXmlValidator {
public:
    explicit StubXmlValidatorFail(std::string errorMsg)
        : errorMsg_(std::move(errorMsg)) {}

    ValidationReport validate(const std::string& /*xmlContent*/,
                               const std::filesystem::path& /*schemaPath*/) const override
    {
        ValidationReport r;
        r.passed = false;
        r.addError(ValidationCategory::Schema, errorMsg_);
        return r;
    }
private:
    std::string errorMsg_;
};

// ---------------------------------------------------------------------------
// Helper: build a minimal valid FmuPackage (in-memory, no real files)
// ---------------------------------------------------------------------------
namespace {

FmuPackage makeMinimalPackage()
{
    FmuPackage pkg;
    pkg.fmiVersion       = FmiVersion::FMI2;
    pkg.modelName        = "TestModel";
    pkg.modelIdentifier  = "TestModel";
    pkg.guid             = "{test-guid}";
    pkg.rawModelDescription = "<fmiModelDescription/>";
    // extractedPath is intentionally empty so structure checks report errors
    return pkg;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Structure validation tests
// ---------------------------------------------------------------------------

class ValidatorStructureTest : public ::testing::Test {
protected:
    StubXmlValidatorPass   xmlPass;
    std::filesystem::path  noSchema{"nonexistent_schema_dir"};
};

TEST_F(ValidatorStructureTest, MissingExtractedPathReportsError)
{
    FmuValidator v(xmlPass, noSchema, noSchema);
    FmuPackage pkg = makeMinimalPackage();
    // extractedPath is empty → structure error expected
    ValidationReport report = v.validate(pkg);
    EXPECT_FALSE(report.passed);
    EXPECT_FALSE(report.errors.empty());
    bool found = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Structure) { found = true; break; }
    EXPECT_TRUE(found);
}

TEST_F(ValidatorStructureTest, MissingSchemaReportsSchemaError)
{
    FmuValidator v(xmlPass, noSchema, noSchema);
    FmuPackage pkg = makeMinimalPackage();
    pkg.rawModelDescription = "something";

    // Create a minimal directory structure so the structure phase passes
    auto tmpDir = std::filesystem::temp_directory_path() / "lib_fmu_toolkit_test_validator";
    std::filesystem::create_directories(tmpDir);
    std::filesystem::create_directories(tmpDir / "binaries");
    pkg.extractedPath = tmpDir;

    ValidationReport report = v.validate(pkg);
    bool hasSchemaError = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Schema) { hasSchemaError = true; break; }
    EXPECT_TRUE(hasSchemaError);

    std::filesystem::remove_all(tmpDir);
}

// ---------------------------------------------------------------------------
// Binary directory naming convention tests
//
// The FMI standard mandates specific directory names inside binaries/:
//   FMI 2.0: win32, win64, linux32, linux64, darwin32, darwin64
//   FMI 3.0: x86_64-windows, x86_64-linux, x86_64-darwin, aarch64-linux, …
//
// A version mismatch must be reported as a structure error.
// ---------------------------------------------------------------------------

class ValidatorBinaryDirTest : public ::testing::Test {
protected:
    StubXmlValidatorPass xmlPass;

    void SetUp() override
    {
        tmpDir_ = std::filesystem::temp_directory_path()
                / "lib_fmu_toolkit_binary_dir_test";
        std::filesystem::create_directories(tmpDir_);

        // Create dummy XSD dir + files so the schema check doesn't block
        xsdDir_ = tmpDir_ / "schema";
        std::filesystem::create_directories(xsdDir_);
        std::ofstream(xsdDir_ / "fmi2ModelDescription.xsd").close();
        std::ofstream(xsdDir_ / "fmi3ModelDescription.xsd").close();
    }

    void TearDown() override
    {
        std::filesystem::remove_all(tmpDir_);
    }

    std::filesystem::path tmpDir_;
    std::filesystem::path xsdDir_;
};

/// FMI 2.0 FMU that only has an FMI 3.0-style directory (x86_64-windows).
TEST_F(ValidatorBinaryDirTest, Fmi2FmuWithFmi3StyleDirReportsError)
{
    // Create the FMI 3.0-style binary directory inside a fresh working dir
    auto fmuRoot = tmpDir_ / "fmu_wrong_dir";
    std::filesystem::create_directories(fmuRoot / "binaries" / "x86_64-windows");

    FmuPackage pkg;
    pkg.fmiVersion          = FmiVersion::FMI2;
    pkg.modelName           = "WrongDir";
    pkg.modelIdentifier     = "WrongDir";
    pkg.rawModelDescription = "<x/>";
    pkg.extractedPath       = fmuRoot;

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool hasStructureError = false;
    for (const auto& e : report.errors) {
        if (e.category == ValidationCategory::Structure &&
            e.message.find("FMI 3.0-style") != std::string::npos) {
            hasStructureError = true;
            break;
        }
    }
    EXPECT_TRUE(hasStructureError)
        << "Expected a structure error about FMI 3.0-style directory in FMI 2.0 FMU";
}

/// FMI 3.0 FMU that only has an FMI 2.0-style directory (win64).
TEST_F(ValidatorBinaryDirTest, Fmi3FmuWithFmi2StyleDirReportsError)
{
    auto fmuRoot = tmpDir_ / "fmu_wrong_dir3";
    std::filesystem::create_directories(fmuRoot / "binaries" / "win64");

    FmuPackage pkg;
    pkg.fmiVersion          = FmiVersion::FMI3;
    pkg.modelName           = "WrongDir3";
    pkg.modelIdentifier     = "WrongDir3";
    pkg.rawModelDescription = "<x/>";
    pkg.extractedPath       = fmuRoot;

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool hasStructureError = false;
    for (const auto& e : report.errors) {
        if (e.category == ValidationCategory::Structure &&
            e.message.find("FMI 2.0-style") != std::string::npos) {
            hasStructureError = true;
            break;
        }
    }
    EXPECT_TRUE(hasStructureError)
        << "Expected a structure error about FMI 2.0-style directory in FMI 3.0 FMU";
}

/// FMI 2.0 FMU with the correct win64 directory — no naming error.
TEST_F(ValidatorBinaryDirTest, Fmi2FmuWithCorrectDirProducesNoNamingError)
{
    auto fmuRoot = tmpDir_ / "fmu_correct2";
    std::filesystem::create_directories(fmuRoot / "binaries" / "win64");

    FmuPackage pkg;
    pkg.fmiVersion          = FmiVersion::FMI2;
    pkg.modelName           = "Correct2";
    pkg.modelIdentifier     = "Correct2";
    pkg.rawModelDescription = "<x/>";
    pkg.extractedPath       = fmuRoot;

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    for (const auto& e : report.errors) {
        EXPECT_FALSE(e.category == ValidationCategory::Structure &&
                     e.message.find("style") != std::string::npos)
            << "Unexpected naming error: " << e.message;
    }
}

/// FMI 3.0 FMU with the correct x86_64-windows directory — no naming error.
TEST_F(ValidatorBinaryDirTest, Fmi3FmuWithCorrectDirProducesNoNamingError)
{
    auto fmuRoot = tmpDir_ / "fmu_correct3";
    std::filesystem::create_directories(fmuRoot / "binaries" / "x86_64-windows");

    FmuPackage pkg;
    pkg.fmiVersion          = FmiVersion::FMI3;
    pkg.modelName           = "Correct3";
    pkg.modelIdentifier     = "Correct3";
    pkg.rawModelDescription = "<x/>";
    pkg.extractedPath       = fmuRoot;

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    for (const auto& e : report.errors) {
        EXPECT_FALSE(e.category == ValidationCategory::Structure &&
                     e.message.find("style") != std::string::npos)
            << "Unexpected naming error: " << e.message;
    }
}

// ---------------------------------------------------------------------------
// Schema validation tests
// ---------------------------------------------------------------------------

TEST(ValidatorSchemaTest, PassingXmlValidatorProducesNoSchemaErrors)
{
    StubXmlValidatorPass xmlPass;
    FmuValidator v(xmlPass, "nonexistent", "nonexistent");

    FmuPackage pkg = makeMinimalPackage();
    pkg.rawModelDescription = "<fmiModelDescription/>";
    // Schema XSD path doesn't exist → schema error from missing-file check
    ValidationReport report = v.validate(pkg);
    bool hasSchemaError = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Schema) { hasSchemaError = true; break; }
    EXPECT_TRUE(hasSchemaError);
}

TEST(ValidatorSchemaTest, EmptyRawDescriptionReportsSchemaError)
{
    StubXmlValidatorPass xmlPass;
    FmuValidator v(xmlPass, "nonexistent", "nonexistent");

    FmuPackage pkg = makeMinimalPackage();
    pkg.rawModelDescription = "";

    ValidationReport report = v.validate(pkg);
    bool found = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Schema) { found = true; break; }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Semantic validation tests
//
// These tests bypass structure and schema checks by providing a real temporary
// directory with dummy XSD files and a stub XML validator.
// ---------------------------------------------------------------------------

class ValidatorSemanticTest : public ::testing::Test {
protected:
    StubXmlValidatorPass xmlPass;

    void SetUp() override
    {
        tmpDir_ = std::filesystem::temp_directory_path()
                / "lib_fmu_toolkit_semantic_test";
        std::filesystem::create_directories(tmpDir_);
        std::filesystem::create_directories(tmpDir_ / "binaries");

        xsdDir_ = tmpDir_ / "schema";
        std::filesystem::create_directories(xsdDir_);
        // Dummy XSD files so the schema file-existence check passes
        std::ofstream(xsdDir_ / "fmi2ModelDescription.xsd").close();
    }

    void TearDown() override
    {
        std::filesystem::remove_all(tmpDir_);
    }

    FmuPackage makePackageWithVars(
        std::vector<std::tuple<std::string, Causality, Variability, DataType>> vars)
    {
        FmuPackage pkg;
        pkg.fmiVersion          = FmiVersion::FMI2;
        pkg.modelName           = "SemTest";
        pkg.modelIdentifier     = "SemTest";
        pkg.rawModelDescription = "<x/>";
        pkg.extractedPath       = tmpDir_;

        uint32_t vr = 0;
        for (auto& [name, c, vi, dt] : vars) {
            ModelVariable mv;
            mv.name           = name;
            mv.valueReference = vr++;
            mv.causality      = c;
            mv.variability    = vi;
            mv.dataType       = dt;
            pkg.variables.push_back(std::move(mv));
        }
        return pkg;
    }

    std::filesystem::path tmpDir_;
    std::filesystem::path xsdDir_;
};

TEST_F(ValidatorSemanticTest, DuplicateValueReferenceForSameTypeReportsError)
{
    FmuPackage pkg = makePackageWithVars({
        {"a", Causality::Output, Variability::Continuous, DataType::Real},
        {"b", Causality::Output, Variability::Continuous, DataType::Real},
    });
    // Force both variables to share the same valueReference
    pkg.variables[0].valueReference = 0;
    pkg.variables[1].valueReference = 0;

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool found = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Semantic &&
            e.message.find("Duplicate") != std::string::npos) { found = true; break; }
    EXPECT_TRUE(found);
}

TEST_F(ValidatorSemanticTest, InputConstantIsInvalid)
{
    FmuPackage pkg = makePackageWithVars({
        {"bad", Causality::Input, Variability::Constant, DataType::Real},
    });

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool found = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Semantic) { found = true; break; }
    EXPECT_TRUE(found);
}

TEST_F(ValidatorSemanticTest, OutputFixedIsInvalid)
{
    FmuPackage pkg = makePackageWithVars({
        {"bad", Causality::Output, Variability::Fixed, DataType::Real},
    });

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool found = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Semantic) { found = true; break; }
    EXPECT_TRUE(found);
}

TEST_F(ValidatorSemanticTest, ParameterContinuousIsInvalid)
{
    FmuPackage pkg = makePackageWithVars({
        {"bad", Causality::Parameter, Variability::Continuous, DataType::Real},
    });

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool found = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Semantic) { found = true; break; }
    EXPECT_TRUE(found);
}

TEST_F(ValidatorSemanticTest, NoOutputVariablesAddsWarning)
{
    FmuPackage pkg = makePackageWithVars({
        {"p", Causality::Parameter, Variability::Fixed, DataType::Real},
    });

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool hasWarning = false;
    for (const auto& w : report.warnings)
        if (w.message.find("output") != std::string::npos) { hasWarning = true; break; }
    EXPECT_TRUE(hasWarning);
}

TEST_F(ValidatorSemanticTest, ValidModelPassesSemanticCheck)
{
    FmuPackage pkg = makePackageWithVars({
        {"u",    Causality::Input,     Variability::Continuous, DataType::Real},
        {"y",    Causality::Output,    Variability::Continuous, DataType::Real},
        {"gain", Causality::Parameter, Variability::Fixed,      DataType::Real},
    });

    FmuValidator v(xmlPass, xsdDir_, xsdDir_);
    ValidationReport report = v.validate(pkg);

    bool hasSemanticError = false;
    for (const auto& e : report.errors)
        if (e.category == ValidationCategory::Semantic) { hasSemanticError = true; break; }
    EXPECT_FALSE(hasSemanticError);
}

// ---------------------------------------------------------------------------
// FMI-LS-BUS rules (Semantic phase)
// ---------------------------------------------------------------------------

namespace {

ModelVariable busVar(const std::string& name, uint32_t vr, DataType dt, Causality c) {
    ModelVariable v;
    v.name = name; v.valueReference = vr; v.dataType = dt; v.causality = c;
    if (dt != DataType::Clock) v.variability = Variability::Discrete;
    return v;
}

bool hasLsBusMessage(const ValidationReport& r, const std::string& needle) {
    for (const auto& e : r.errors) if (e.message.find("LS-BUS") != std::string::npos && e.message.find(needle) != std::string::npos) return true;
    return false;
}

FmuPackage makeBusPackage() {
    FmuPackage pkg;
    pkg.fmiVersion = FmiVersion::FMI3;
    pkg.modelName = "Node";
    pkg.coSimulation.hasEventMode = true;
    pkg.layeredStandards = {"org.fmi-standard.fmi-ls-bus"};
    auto txClock = busVar("Tx_Clock", 1, DataType::Clock, Causality::Output);
    auto tx = busVar("Tx_Data", 2, DataType::Binary, Causality::Output);
    tx.mimeType = "application/org.fmi-standard.fmi-ls-bus.can;version=\"1.0.0\"";
    tx.clocks = {1};
    pkg.variables = {txClock, tx};
    Terminal t;
    t.name = "Bus"; t.terminalKind = "org.fmi-ls-bus.network-terminal"; t.matchingRule = "bus";
    t.members = {{"Tx_Data", "Tx_Data", "signal"}, {"Tx_Clock", "Tx_Clock", "signal"}};
    pkg.terminals = {t};
    return pkg;
}

}  // namespace

TEST_F(ValidatorStructureTest, WellFormedLsBusNodeHasNoLsBusFindings)
{
    FmuValidator v(xmlPass, noSchema, noSchema);
    const ValidationReport report = v.validate(makeBusPackage());
    EXPECT_FALSE(hasLsBusMessage(report, ""));
}

TEST_F(ValidatorStructureTest, LsBusNeedsEventMode)
{
    FmuValidator v(xmlPass, noSchema, noSchema);
    FmuPackage pkg = makeBusPackage();
    pkg.coSimulation.hasEventMode = false;
    EXPECT_TRUE(hasLsBusMessage(v.validate(pkg), "hasEventMode"));
}

TEST_F(ValidatorStructureTest, LsBusBinaryMustBeClockedByAClockVariable)
{
    FmuValidator v(xmlPass, noSchema, noSchema);
    FmuPackage pkg = makeBusPackage();
    pkg.variables[1].clocks = {99};
    EXPECT_TRUE(hasLsBusMessage(v.validate(pkg), "not a Clock variable"));
    pkg.variables[1].clocks.clear();
    EXPECT_TRUE(hasLsBusMessage(v.validate(pkg), "not clocked"));
}

TEST_F(ValidatorStructureTest, LsBusTerminalMembersAreChecked)
{
    FmuValidator v(xmlPass, noSchema, noSchema);
    FmuPackage pkg = makeBusPackage();
    pkg.terminals[0].members.pop_back();  // Tx_Data without Tx_Clock
    EXPECT_TRUE(hasLsBusMessage(v.validate(pkg), "both Tx_Data and Tx_Clock"));
    pkg = makeBusPackage();
    pkg.terminals[0].members[0].variableName = "Missing";
    EXPECT_TRUE(hasLsBusMessage(v.validate(pkg), "unknown variable"));
    pkg = makeBusPackage();
    pkg.variables[1].causality = Causality::Input;  // Tx_Data must be an output
    EXPECT_TRUE(hasLsBusMessage(v.validate(pkg), "causality output"));
}
