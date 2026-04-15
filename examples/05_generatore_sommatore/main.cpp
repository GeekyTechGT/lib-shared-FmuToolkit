// =============================================================================
// Example 05 — Generatore + Sommatore (FMI 3.0)
//
// Demonstrates:
//   - Loading two FMI 3.0 FMUs in a single engine instance
//   - Step-by-step orchestration: reading output from one FMU and
//     feeding it as input to another
//   - Metadata inspection: DefaultExperiment, variable listings
//   - Result export to CSV and JSON
//
// Scenario:
//   Generatore produces a ramp:    y_G = t * 5.0
//   Sommatore sums two addends:    y_S = u1 + u2
//   We feed y_G into u1 and a
//   constant 10 into u2, so:       y_S = t * 5.0 + 10.0
//
// Usage:
//   example_05_generatore_sommatore data/fmi3/Generatore.fmu data/fmi3/Sommatore.fmu
// =============================================================================

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <variant>

// Extract a double from any OutputValue variant
static double toDouble(const libfmu_toolkit::OutputValue& v) {
    return std::visit([](auto&& x) -> double {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double>)  return x;
        if constexpr (std::is_same_v<T, int64_t>) return static_cast<double>(x);
        if constexpr (std::is_same_v<T, bool>)    return x ? 1.0 : 0.0;
        return 0.0;
    }, v);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <path/to/Generatore.fmu> <path/to/Sommatore.fmu>\n";
        return EXIT_FAILURE;
    }

    const std::string genPath = argv[1];
    const std::string sumPath = argv[2];

    try {
        libfmu_toolkit::FmuEngine engine;

        // -----------------------------------------------------------------------
        // Load both FMUs
        // -----------------------------------------------------------------------
        std::cout << "Loading Generatore... ";
        const uint64_t hGen = engine.load(genPath);
        std::cout << "OK\n";

        std::cout << "Loading Sommatore...  ";
        const uint64_t hSum = engine.load(sumPath);
        std::cout << "OK\n\n";

        // -----------------------------------------------------------------------
        // Inspect metadata
        // -----------------------------------------------------------------------
        const auto infoGen = engine.info(hGen);
        const auto infoSum = engine.info(hSum);

        auto printInfo = [](const libfmu_toolkit::FmuInfo& info) {
            std::cout << "  Model      : " << info.modelName << "\n"
                      << "  Identifier : " << info.modelIdentifier << "\n"
                      << "  FMI        : " << (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2
                                               ? "2.0" : "3.0") << "\n"
                      << "  Description: " << info.description << "\n";
            if (info.defaultExperiment.hasStopTime)
                std::cout << "  DefaultStop: " << info.defaultExperiment.stopTime << " s\n";
            std::cout << "  Variables  :";
            for (const auto& v : info.variables)
                std::cout << " " << v.name
                          << "(" << (v.causality == libfmu_toolkit::Causality::Output ? "out" :
                                     v.causality == libfmu_toolkit::Causality::Input  ? "in"  :
                                     v.causality == libfmu_toolkit::Causality::Independent ? "t" : "?")
                          << ")";
            std::cout << "\n";
        };

        std::cout << "[Generatore]\n";  printInfo(infoGen);
        std::cout << "\n[Sommatore]\n"; printInfo(infoSum);

        // -----------------------------------------------------------------------
        // Validate both FMUs
        // -----------------------------------------------------------------------
        std::cout << "\nValidating...\n";
        for (auto [label, h] : std::initializer_list<std::pair<const char*, uint64_t>>{
                {"Generatore", hGen}, {"Sommatore", hSum}}) {
            const auto r = engine.validate(h);
            std::cout << "  " << label << ": " << (r.passed ? "PASSED" : "FAILED");
            if (r.hasWarnings())
                std::cout << " (" << r.warnings.size() << " warning(s))";
            std::cout << "\n";
            for (const auto& e : r.errors)
                std::cout << "    [ERROR] " << e.message << "\n";
        }

        // -----------------------------------------------------------------------
        // Co-simulation setup
        // -----------------------------------------------------------------------
        constexpr double startTime = 0.0;
        constexpr double stopTime  = 5.0;
        constexpr double stepSize  = 1.0;
        constexpr double u2Fixed   = 10.0; // constant second addend fed to Sommatore

        libfmu_toolkit::SimConfig cfgGen;
        cfgGen.startTime = startTime;
        cfgGen.stopTime  = stopTime;
        cfgGen.stepSize  = stepSize;

        libfmu_toolkit::SimConfig cfgSum;
        cfgSum.startTime = startTime;
        cfgSum.stopTime  = stopTime;
        cfgSum.stepSize  = stepSize;
        cfgSum.initialParameters["u2"] = u2Fixed;

        libfmu_toolkit::FmuSession genSession = engine.openSession(hGen, cfgGen);
        libfmu_toolkit::FmuSession sumSession = engine.openSession(hSum, cfgSum);

        // -----------------------------------------------------------------------
        // Step loop: read Generatore → feed to Sommatore
        // -----------------------------------------------------------------------
        std::cout << "\nOrchestrated co-simulation  [Generatore → Sommatore]\n";
        std::cout << "Scenario: y_S(t) = Generatore.y + " << u2Fixed
                  << " = t*5.0 + " << u2Fixed << "\n\n";

        std::cout << std::fixed << std::setprecision(4);
        std::cout << std::setw(8)  << "t"
                  << std::setw(12) << "Gen.y"
                  << std::setw(12) << "Sum.y"
                  << std::setw(14) << "expected\n";
        std::cout << std::string(46, '-') << "\n";

        // Storage for the final-step values for export
        libfmu_toolkit::SimResult exportResult;
        exportResult.timePoints.reserve(static_cast<size_t>(stopTime / stepSize) + 2);

        const int totalSteps = static_cast<int>((stopTime - startTime) / stepSize);
        double t = startTime;

        for (int step = 0; step <= totalSteps; ++step) {
            // 1. Read Generatore output
            const double genY = toDouble(genSession.getValue("y"));

            // 2. Forward to Sommatore input u1.
            // setValue before reading y is the correct FMI pattern: the FMU
            // picks up u1 on the next doStep (or on init for step 0).
            sumSession.setValue("u1", genY);

            // 4. Read Sommatore output
            const double sumY     = toDouble(sumSession.getValue("y"));
            const double expected = t * 5.0 + u2Fixed;

            std::cout << std::setw(8)  << t
                      << std::setw(12) << genY
                      << std::setw(12) << sumY
                      << std::setw(14) << expected << "\n";

            // Accumulate into export result
            exportResult.timePoints.push_back(t);
            exportResult.series["Gen.y"].push_back(libfmu_toolkit::OutputValue{genY});
            exportResult.series["Sum.y"].push_back(libfmu_toolkit::OutputValue{sumY});

            // 5. Advance both FMUs
            if (step < totalSteps) {
                (void)genSession.doStep(t, stepSize);
                sumSession.setValue("u1", toDouble(genSession.getValue("y")));
                (void)sumSession.doStep(t, stepSize);
                t += stepSize;
            }
        }

        genSession.terminate();
        sumSession.terminate();

        // -----------------------------------------------------------------------
        // Export combined result
        // -----------------------------------------------------------------------
        const std::string csvPath  = "results_05.csv";
        const std::string jsonPath = "results_05.json";
        libfmu_toolkit::FmuEngine::exportCsv(exportResult,  csvPath);
        libfmu_toolkit::FmuEngine::exportJson(exportResult, jsonPath);

        std::cout << "\nExported:\n";
        std::cout << "  CSV  → " << csvPath  << "\n";
        std::cout << "  JSON → " << jsonPath << "\n";

        engine.unload(hGen);
        engine.unload(hSum);
        return EXIT_SUCCESS;
    }
    catch (const libfmu_toolkit::FmuLoadError& e) {
        std::cerr << "[Load Error]       " << e.what() << "\n";
    }
    catch (const libfmu_toolkit::FmuSimulationError& e) {
        std::cerr << "[Simulation Error] " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[Error]            " << e.what() << "\n";
    }
    return EXIT_FAILURE;
}
