// =============================================================================
// Example 03 — FMU Validation
//
// Demonstrates the three-phase validation pipeline:
//
//   Phase 1: Structure  — checks the FMU archive layout defined by the FMI
//             standard:
//               • modelDescription.xml must be present at the archive root
//               • binaries/ directory must be present
//               • binary subdirectories must use the naming convention that
//                 matches the FMI version:
//                   FMI 2.0 → win32, win64, linux32, linux64, darwin32, darwin64
//                   FMI 3.0 → x86_64-windows, x86_64-linux, x86_64-darwin, …
//               • the shared library must be named <modelIdentifier>.<ext>
//
//   Phase 2: Schema     — validates modelDescription.xml against the official
//             FMI XSD schema (fmi2ModelDescription.xsd / fmi3ModelDescription.xsd).
//             Line- and column-accurate errors are reported.
//
//   Phase 3: Semantics  — checks variable rules beyond what XSD covers:
//               • no duplicate valueReference within a DataType
//               • invalid causality/variability combinations
//               • at least one output variable (warning only)
//
// All three phases run even if a previous phase found errors, so a single
// validate() call returns the complete diagnostic picture.
//
// Exit code:
//   0 — validation passed (warnings may still be present)
//   1 — validation failed or load error
//
// Usage:
//   example_03_validate_fmu path/to/model.fmu
//
// Example with the bundled test data:
//   example_03_validate_fmu data/fmi3/Generatore.fmu
//   example_03_validate_fmu data/fmi2/SimpleAdder.fmu
//   example_03_validate_fmu data/fmi2/corrupted/wrong_platform_dir.fmu
// =============================================================================

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Print a validation category label
// ---------------------------------------------------------------------------
static const char* categoryLabel(libfmu_toolkit::ValidationCategory cat)
{
    using libfmu_toolkit::ValidationCategory;
    switch (cat) {
        case ValidationCategory::Structure: return "Structure";
        case ValidationCategory::Schema:    return "Schema";
        case ValidationCategory::Semantic:  return "Semantic";
    }
    return "Unknown";
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-fmu>\n\n"
                  << "Examples:\n"
                  << "  " << argv[0] << " data/fmi3/Generatore.fmu\n"
                  << "  " << argv[0] << " data/fmi2/SimpleAdder.fmu\n"
                  << "  " << argv[0] << " data/fmi2/corrupted/wrong_platform_dir.fmu\n";
        return EXIT_FAILURE;
    }

    const std::string fmuPath = argv[1];

    try {
        libfmu_toolkit::FmuEngine engine;
        const uint64_t handle = engine.load(fmuPath);

        // -----------------------------------------------------------------------
        // 1. Inspect metadata before validation
        // -----------------------------------------------------------------------
        const libfmu_toolkit::FmuInfo info = engine.info(handle);

        std::cout << "=============================================================\n";
        std::cout << "FMU: " << fmuPath << "\n";
        std::cout << "=============================================================\n\n";

        std::cout << "Metadata:\n";
        std::cout << "  Model name    : " << info.modelName << "\n";
        std::cout << "  Identifier    : " << info.modelIdentifier << "\n";
        std::cout << "  FMI version   : "
                  << (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2 ? "2.0" : "3.0") << "\n";
        std::cout << "  GUID          : " << info.guid << "\n";
        if (!info.description.empty())
            std::cout << "  Description   : " << info.description << "\n";
        if (!info.generationTool.empty())
            std::cout << "  Generator     : " << info.generationTool << "\n";

        // Unit definitions
        if (!info.unitDefinitions.empty()) {
            std::cout << "\n  Unit definitions (" << info.unitDefinitions.size() << "):\n";
            for (const auto& u : info.unitDefinitions) {
                std::cout << "    " << u.name;
                if (u.factor != 1.0 || u.offset != 0.0)
                    std::cout << "  factor=" << u.factor << "  offset=" << u.offset;
                std::cout << "\n";
            }
        }

        // Type definitions
        if (!info.typeDefinitions.empty()) {
            std::cout << "\n  Type definitions (" << info.typeDefinitions.size() << "):\n";
            for (const auto& t : info.typeDefinitions) {
                std::cout << "    [" << t.name << "]";
                if (!t.unit.empty())     std::cout << "  unit=" << t.unit;
                if (!t.quantity.empty()) std::cout << "  quantity=" << t.quantity;
                std::cout << "\n";
            }
        }

        // Variables
        std::cout << "\n  Variables (" << info.variables.size() << "):\n";
        const auto inputs  = info.inputs();
        const auto outputs = info.outputs();
        const auto params  = info.parameters();
        std::cout << "    Inputs (" << inputs.size() << "):";
        for (const auto* v : inputs)  std::cout << " " << v->name;
        std::cout << "\n";
        std::cout << "    Outputs (" << outputs.size() << "):";
        for (const auto* v : outputs) std::cout << " " << v->name;
        std::cout << "\n";
        std::cout << "    Parameters (" << params.size() << "):";
        for (const auto* v : params)  std::cout << " " << v->name;
        std::cout << "\n";

        // -----------------------------------------------------------------------
        // 2. Run three-phase validation
        // -----------------------------------------------------------------------
        std::cout << "\n-------------------------------------------------------------\n";
        std::cout << "Running validation...\n";
        std::cout << "-------------------------------------------------------------\n\n";

        const libfmu_toolkit::ValidationResult result = engine.validate(handle);

        if (result.passed) {
            std::cout << "RESULT: PASSED";
            if (!result.warnings.empty())
                std::cout << "  (with " << result.warnings.size() << " warning(s))";
            std::cout << "\n";
        } else {
            std::cout << "RESULT: FAILED\n";
        }

        // Print errors grouped by phase
        if (!result.errors.empty()) {
            std::cout << "\nErrors (" << result.errors.size() << "):\n";
            for (const auto& err : result.errors) {
                std::cout << "  [" << categoryLabel(err.category) << "]";
                if (err.line > 0)
                    std::cout << " (line " << err.line;
                if (err.column > 0)
                    std::cout << ":" << err.column;
                if (err.line > 0)
                    std::cout << ")";
                std::cout << " " << err.message << "\n";
            }
        }

        if (!result.warnings.empty()) {
            std::cout << "\nWarnings (" << result.warnings.size() << "):\n";
            for (const auto& warn : result.warnings) {
                std::cout << "  [" << categoryLabel(warn.category) << "] "
                          << warn.message << "\n";
            }
        }

        engine.unload(handle);
        return result.passed ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch (const libfmu_toolkit::FmuLoadError& e) {
        std::cerr << "[Load Error] " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << "\n";
    }
    return EXIT_FAILURE;
}
