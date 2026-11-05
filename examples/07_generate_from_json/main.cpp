#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const std::filesystem::path jsonPath =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path(LIB_FMU_TOOLKIT_SOURCE_DIR) / "data/generation/fmi2_from_json_example.json";
    const std::filesystem::path outputDir =
        argc > 2 ? std::filesystem::path(argv[2])
                 : std::filesystem::path("generated/example_07");

    try {
        libfmu_toolkit::FmuArtifactGenerator generator;
        libfmu_toolkit::FmuArtifactGenerationOptions options;
        options.outputDirectory = outputDir;
        options.generateModelDescription = true;
        options.generateCSource = true;

        const auto paths = generator.generateFromJson(jsonPath.string(), options);

        std::cout << "JSON input             : " << jsonPath << "\n";
        std::cout << "modelDescription.xml   : " << paths.modelDescriptionXmlPath << "\n";
        std::cout << "C accessors source     : " << paths.cSourcePath << "\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}
