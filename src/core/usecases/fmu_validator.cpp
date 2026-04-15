#include "fmu_validator.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// FMI 2.0 standard binary directory names (FMI 2.0 spec §4.1)
//
// Legacy "flat" naming: <os><bitwidth>
//   win32, win64, linux32, linux64, darwin32, darwin64
// ---------------------------------------------------------------------------
static const std::vector<std::string> kFmi2BinaryDirs = {
    "win32",   "win64",
    "linux32", "linux64",
    "darwin32","darwin64"
};

// ---------------------------------------------------------------------------
// FMI 3.0 standard binary directory names (FMI 3.0 spec §4.1)
//
// New "<arch>-<os>" naming, e.g. x86_64-windows, aarch64-linux
// Covers all architectures listed in the FMI 3.0 specification.
// ---------------------------------------------------------------------------
static const std::vector<std::string> kFmi3BinaryDirs = {
    "x86-windows",    "x86_64-windows",  "aarch64-windows",
    "x86-linux",      "x86_64-linux",    "aarch64-linux",
    "x86_64-darwin",  "aarch64-darwin"
};

// ---------------------------------------------------------------------------

FmuValidator::FmuValidator(IXmlValidator&        xmlValidator,
                           std::filesystem::path fmi2SchemaDir,
                           std::filesystem::path fmi3SchemaDir)
    : xmlValidator_(xmlValidator)
    , fmi2SchemaDir_(std::move(fmi2SchemaDir))
    , fmi3SchemaDir_(std::move(fmi3SchemaDir))
{}

// ---------------------------------------------------------------------------
// checkBinaryNamingConvention
//
// FMI 2.0 mandates the legacy naming: binaries/<win32|win64|linux32|linux64|…>/
// FMI 3.0 mandates the new naming:    binaries/<arch>-<os>/
//
// A version mismatch (e.g. an FMI 2.0 FMU that only contains
// "binaries/x86_64-windows/") is reported as an error because tool integrators
// rely on the naming convention to locate the correct shared library for the
// running platform. If BOTH styles are present, a warning is issued.
// ---------------------------------------------------------------------------
void FmuValidator::checkBinaryNamingConvention(
    const FmuPackage& pkg, ValidationReport& report) const
{
    namespace fs = std::filesystem;

    const fs::path binDir = pkg.extractedPath / "binaries";
    if (!fs::is_directory(binDir)) return; // Already reported by checkStructure

    // Walk binaries/ and classify each subdirectory
    std::vector<std::string> foundFmi2;
    std::vector<std::string> foundFmi3;

    for (const auto& entry : fs::directory_iterator(binDir)) {
        if (!entry.is_directory()) continue;
        const std::string name = entry.path().filename().string();

        if (std::find(kFmi2BinaryDirs.begin(), kFmi2BinaryDirs.end(), name)
                != kFmi2BinaryDirs.end()) {
            foundFmi2.push_back(name);
        }
        if (std::find(kFmi3BinaryDirs.begin(), kFmi3BinaryDirs.end(), name)
                != kFmi3BinaryDirs.end()) {
            foundFmi3.push_back(name);
        }
    }

    if (pkg.fmiVersion == FmiVersion::FMI2) {
        // FMI 2.0 FMU must not contain exclusively FMI 3.0-style directories
        if (!foundFmi3.empty() && foundFmi2.empty()) {
            report.addError(
                ValidationCategory::Structure,
                "FMI 2.0 FMU contains FMI 3.0-style binary directory '"
                + foundFmi3.front()
                + "'. FMI 2.0 requires one of: win32, win64, linux32, linux64, "
                  "darwin32, darwin64");
        } else if (!foundFmi3.empty()) {
            // Both styles present — non-standard for FMI 2.0 but not fatal
            report.addWarning(
                ValidationCategory::Structure,
                "FMI 2.0 FMU contains FMI 3.0-style binary directory '"
                + foundFmi3.front()
                + "' alongside FMI 2.0 directories. This is non-standard.");
        }
    } else {
        // FMI 3.0 FMU must not contain exclusively FMI 2.0-style directories
        if (!foundFmi2.empty() && foundFmi3.empty()) {
            report.addError(
                ValidationCategory::Structure,
                "FMI 3.0 FMU contains FMI 2.0-style binary directory '"
                + foundFmi2.front()
                + "'. FMI 3.0 requires one of: x86_64-windows, x86_64-linux, "
                  "x86_64-darwin, aarch64-linux, aarch64-darwin");
        } else if (!foundFmi2.empty()) {
            report.addWarning(
                ValidationCategory::Structure,
                "FMI 3.0 FMU contains FMI 2.0-style binary directory '"
                + foundFmi2.front()
                + "' alongside FMI 3.0 directories. This is non-standard.");
        }
    }
}

// ---------------------------------------------------------------------------
// checkLibraryIdentifier
//
// Per FMI spec, the shared library inside the platform directory must be named
// <modelIdentifier>.<platform-extension>  (e.g. MyModel.dll on Windows).
// A mismatch produces a warning because tools that rely on this convention —
// including this library's ZipLoaderAdapter — will fail to locate the binary.
// ---------------------------------------------------------------------------
void FmuValidator::checkLibraryIdentifier(
    const FmuPackage& pkg, ValidationReport& report) const
{
    namespace fs = std::filesystem;

    if (pkg.modelIdentifier.empty()) return;
    if (pkg.sharedLibraryPath.empty() || !fs::exists(pkg.sharedLibraryPath)) return;

    const std::string libStem = pkg.sharedLibraryPath.stem().string();
    if (libStem != pkg.modelIdentifier) {
        report.addWarning(
            ValidationCategory::Structure,
            "Shared library filename '" + libStem
            + "' does not match modelIdentifier '" + pkg.modelIdentifier
            + "'. Per FMI standard the library must be named <modelIdentifier>.<ext>.");
    }
}

// ---------------------------------------------------------------------------
// checkStructure — Phase 1
//
// Verifies the mandatory FMU archive layout:
//   1. The extracted root directory exists.
//   2. modelDescription.xml is present.
//   3. binaries/ directory is present.
//   4. A platform-specific subdirectory exists under binaries/ for the current
//      host (we accept both FMI 2.0 and FMI 3.0 naming here to support either
//      version running on the current host).
//   5. Binary directory names match the FMI version convention.
//   6. The shared library filename matches modelIdentifier.
// ---------------------------------------------------------------------------
void FmuValidator::checkStructure(const FmuPackage& pkg, ValidationReport& report) const
{
    namespace fs = std::filesystem;

    // 1. Root directory must exist
    if (pkg.extractedPath.empty() || !fs::exists(pkg.extractedPath)) {
        report.addError(ValidationCategory::Structure, "Extracted path not found");
        return; // Nothing further can be checked without an extracted directory
    }

    // 2. modelDescription.xml must be present at the archive root
    const fs::path modelDescPath = pkg.extractedPath / "modelDescription.xml";
    if (!fs::exists(modelDescPath)) {
        report.addError(ValidationCategory::Structure,
                        "modelDescription.xml not found at archive root");
    }

    // 3. binaries/ directory must be present
    const fs::path binariesDir = pkg.extractedPath / "binaries";
    if (!fs::exists(binariesDir) || !fs::is_directory(binariesDir)) {
        report.addError(
            ValidationCategory::Structure,
            "binaries/ directory not found. FMI requires a binaries/ subdirectory "
            "containing platform-specific shared libraries.");
    } else {
        // 4. Check that the current host platform has a binary directory.
        //    Accept both FMI 2.0 and FMI 3.0 naming so this check works for either version.
        std::vector<std::string> expectedDirs;
#if defined(_WIN32)
        expectedDirs = {"win64", "x86_64-windows"};
#elif defined(__APPLE__)
        expectedDirs = {"darwin64", "x86_64-darwin"};
#else
        expectedDirs = {"linux64", "x86_64-linux"};
#endif
        bool foundPlatformDir = false;
        for (const auto& dirName : expectedDirs) {
            if (fs::exists(binariesDir / dirName)) {
                foundPlatformDir = true;
                break;
            }
        }
        if (!foundPlatformDir) {
            report.addWarning(
                ValidationCategory::Structure,
                "No binary directory found for the current host platform. Checked: "
                + expectedDirs.front() + ", " + expectedDirs.back());
        }

        // 5. Verify naming convention matches the FMI version
        checkBinaryNamingConvention(pkg, report);
    }

    // 6. Shared library must exist at the resolved path (if it was resolved)
    if (!pkg.sharedLibraryPath.empty() && !fs::exists(pkg.sharedLibraryPath)) {
        report.addError(ValidationCategory::Structure,
                        "Shared library not found: "
                        + pkg.sharedLibraryPath.string());
    }

    // 7. Library filename must match modelIdentifier
    checkLibraryIdentifier(pkg, report);
}

// ---------------------------------------------------------------------------
// checkSchema — Phase 2
//
// Validates the raw modelDescription.xml against the FMI XSD schema.
// The schema directory and filename are determined by the FMI version.
// Errors and warnings from the XML validator are merged into the report.
// ---------------------------------------------------------------------------
void FmuValidator::checkSchema(const FmuPackage& pkg, ValidationReport& report) const
{
    namespace fs = std::filesystem;

    // XML content must be non-empty before we attempt schema validation
    if (pkg.rawModelDescription.empty()) {
        report.addError(ValidationCategory::Schema,
                        "modelDescription.xml content is empty");
        return;
    }

    // Select the appropriate schema file for the FMI version
    fs::path    schemaDir;
    std::string xsdFilename;

    if (pkg.fmiVersion == FmiVersion::FMI2) {
        schemaDir   = fmi2SchemaDir_;
        xsdFilename = "fmi2ModelDescription.xsd";
    } else {
        schemaDir   = fmi3SchemaDir_;
        xsdFilename = "fmi3ModelDescription.xsd";
    }

    const fs::path xsdPath = schemaDir / xsdFilename;

    if (!fs::exists(xsdPath)) {
        report.addError(ValidationCategory::Schema,
                        "XSD schema file not found: " + xsdPath.string());
        return;
    }

    // Delegate actual XML/XSD validation to the injected validator
    ValidationReport schemaReport = xmlValidator_.validate(
        pkg.rawModelDescription, xsdPath);

    for (auto& err  : schemaReport.errors)   report.errors.push_back(std::move(err));
    for (auto& warn : schemaReport.warnings)  report.warnings.push_back(std::move(warn));
}

// ---------------------------------------------------------------------------
// checkSemantics — Phase 3
//
// Checks model-variable invariants that XSD cannot express:
//   - No duplicate valueReference within the same DataType.
//   - Invalid causality/variability combinations (per FMI 2.0 §2.2.7 /
//     FMI 3.0 §2.4.7 tables).
//   - At least one output variable should exist (warning only, not an error).
// ---------------------------------------------------------------------------
void FmuValidator::checkSemantics(const FmuPackage& pkg, ValidationReport& report) const
{
    // --- Duplicate valueReference check ---
    // Within a single DataType, each valueReference must be unique.
    {
        std::map<DataType, std::set<uint32_t>> seenVrByType;
        for (const auto& var : pkg.variables) {
            auto& seenSet = seenVrByType[var.dataType];
            if (!seenSet.insert(var.valueReference).second) {
                report.addError(
                    ValidationCategory::Semantic,
                    "Duplicate valueReference " +
                        std::to_string(var.valueReference) +
                        " for type " +
                        std::string(to_string(var.dataType)));
            }
        }
    }

    // --- Causality / variability combination checks ---
    // Invalid combinations produce errors (not warnings) because they represent
    // structural violations of the FMI variable model.
    for (const auto& var : pkg.variables) {
        const Causality   c = var.causality;
        const Variability v = var.variability;

        if (c == Causality::Input && v == Variability::Constant) {
            report.addError(
                ValidationCategory::Semantic,
                "Variable '" + var.name +
                    "': causality=input combined with variability=constant is invalid");
        } else if (c == Causality::Output &&
                   (v == Variability::Fixed || v == Variability::Tunable)) {
            report.addError(
                ValidationCategory::Semantic,
                "Variable '" + var.name +
                    "': causality=output combined with variability=" +
                    std::string(to_string(v)) + " is invalid");
        } else if (c == Causality::Parameter &&
                   (v == Variability::Discrete || v == Variability::Continuous)) {
            report.addError(
                ValidationCategory::Semantic,
                "Variable '" + var.name +
                    "': causality=parameter combined with variability=" +
                    std::string(to_string(v)) + " is invalid");
        }
    }

    // --- Output variable presence ---
    // An FMU with no outputs is technically valid but practically useless.
    if (pkg.outputs().empty()) {
        report.addWarning(ValidationCategory::Semantic,
                          "FMU has no output variables");
    }
}

// ---------------------------------------------------------------------------
// validate — entry point
// ---------------------------------------------------------------------------
ValidationReport FmuValidator::validate(const FmuPackage& pkg) const
{
    ValidationReport report;

    // All three phases run unconditionally to produce a complete diagnostic set
    checkStructure(pkg, report);
    checkSchema(pkg, report);
    checkSemantics(pkg, report);

    // passed = true only when no errors were found across all phases
    report.passed = !report.hasErrors();
    return report;
}

} // namespace libfmu_toolkit
