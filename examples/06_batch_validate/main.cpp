// =============================================================================
// Example 06 — Batch Validation: Valid and Corrupted FMUs
//
// Demonstrates the enhanced three-phase structure validation by running the
// validator over both valid and intentionally corrupted FMU archives from the
// test data directory (data/fmi2/ and data/fmi3/).
//
// For each FMU the output shows which validation phase detected the problem
// and what the precise error message is. This is the recommended pattern for
// building FMU health-check pipelines or CI validation scripts.
//
// Usage:
//   example_06_batch_validate <data-dir>
//
//   where <data-dir> is the path to the repository's data/ directory.
//
// Example:
//   example_06_batch_validate data
//
// The expected structure under <data-dir>:
//   data/
//   ├── fmi2/
//   │   ├── SimpleAdder.fmu                (valid FMI 2.0)
//   │   └── corrupted/
//   │       ├── corrupted_xml.fmu          (malformed XML)
//   │       ├── missing_binaries.fmu       (no binaries/ directory)
//   │       ├── missing_model_desc.fmu     (no modelDescription.xml)
//   │       └── wrong_platform_dir.fmu     (FMI 3.0-style dir in FMI 2.0 FMU)
//   └── fmi3/
//       ├── Generatore.fmu                 (valid FMI 3.0, real binary)
//       ├── Sommatore.fmu                  (valid FMI 3.0, real binary)
//       ├── SimpleRamp.fmu                 (valid FMI 3.0 stub)
//       └── corrupted/
//           ├── corrupted_xml.fmu          (malformed XML)
//           ├── missing_binaries.fmu       (no binaries/ directory)
//           ├── missing_model_desc.fmu     (no modelDescription.xml)
//           └── wrong_platform_dir.fmu     (FMI 2.0-style dir in FMI 3.0 FMU)
// =============================================================================

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Convert a ValidationCategory to a readable label
// ---------------------------------------------------------------------------
static const char* categoryLabel(libfmu_toolkit::ValidationCategory cat)
{
    using libfmu_toolkit::ValidationCategory;
    switch (cat) {
        case ValidationCategory::Structure: return "Structure";
        case ValidationCategory::Schema:    return "Schema";
        case ValidationCategory::Semantic:  return "Semantic";
    }
    return "?";
}

// ---------------------------------------------------------------------------
// Validate a single FMU and print a compact report
// ---------------------------------------------------------------------------
static bool validateOne(libfmu_toolkit::FmuEngine& engine, const fs::path& fmuPath)
{
    const std::string relPath = fmuPath.filename().string();
    std::cout << "  " << relPath << "\n";

    uint64_t handle = 0;

    // Attempt to load (extraction + XML parsing + library loading)
    try {
        handle = engine.load(fmuPath.string());
    }
    catch (const libfmu_toolkit::FmuLoadError& ex) {
        // Some corrupted FMUs (e.g. missing modelDescription.xml) fail at load
        std::cout << "    [Load Error] " << ex.what() << "\n\n";
        return false;
    }

    const libfmu_toolkit::ValidationResult result = engine.validate(handle);

    if (result.passed) {
        const libfmu_toolkit::FmuInfo info = engine.info(handle);
        const char* ver = (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2) ? "2.0" : "3.0";
        std::cout << "    PASSED  [FMI " << ver << "  " << info.modelName << "]";
        if (!result.warnings.empty())
            std::cout << "  (" << result.warnings.size() << " warning(s))";
        std::cout << "\n";
    } else {
        std::cout << "    FAILED\n";
        for (const auto& err : result.errors)
            std::cout << "    [" << categoryLabel(err.category) << "] " << err.message << "\n";
    }

    for (const auto& warn : result.warnings)
        std::cout << "    [WARN/" << categoryLabel(warn.category) << "] " << warn.message << "\n";

    std::cout << "\n";
    engine.unload(handle);
    return result.passed;
}

// ---------------------------------------------------------------------------
// Collect all .fmu files under a directory (non-recursive)
// ---------------------------------------------------------------------------
static std::vector<fs::path> collectFmus(const fs::path& dir)
{
    std::vector<fs::path> result;
    if (!fs::is_directory(dir)) return result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".fmu")
            result.push_back(entry.path());
    }
    std::sort(result.begin(), result.end());
    return result;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <data-dir>\n\n"
                  << "Example:\n"
                  << "  " << argv[0] << " data\n";
        return EXIT_FAILURE;
    }

    const fs::path dataDir = argv[1];
    if (!fs::is_directory(dataDir)) {
        std::cerr << "[Error] Not a directory: " << dataDir << "\n";
        return EXIT_FAILURE;
    }

    libfmu_toolkit::FmuEngine engine;

    int total  = 0;
    int passed = 0;
    int failed = 0;

    // Iterate fmi2/ and fmi3/ sub-trees
    for (const char* sub : {"fmi2", "fmi3"}) {
        const fs::path subDir = dataDir / sub;
        if (!fs::is_directory(subDir)) continue;

        std::cout << "=============================================================\n";
        std::cout << "  " << sub << "/ — valid FMUs\n";
        std::cout << "=============================================================\n\n";

        for (const auto& fmuPath : collectFmus(subDir)) {
            ++total;
            if (validateOne(engine, fmuPath)) ++passed; else ++failed;
        }

        const fs::path corruptedDir = subDir / "corrupted";
        if (fs::is_directory(corruptedDir)) {
            std::cout << "-------------------------------------------------------------\n";
            std::cout << "  " << sub << "/corrupted/ — intentionally broken FMUs\n";
            std::cout << "-------------------------------------------------------------\n\n";

            for (const auto& fmuPath : collectFmus(corruptedDir)) {
                ++total;
                if (validateOne(engine, fmuPath)) ++passed; else ++failed;
            }
        }
    }

    // Summary
    std::cout << "=============================================================\n";
    std::cout << "  Summary: " << total << " FMUs checked"
              << "  |  Passed: " << passed
              << "  |  Failed: " << failed << "\n";
    std::cout << "=============================================================\n";

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
