#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const std::filesystem::path jsonPath =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path(LIB_FMU_TOOLKIT_SOURCE_DIR) / "data/generation/fmi3_from_json_example.json";
    const std::filesystem::path xmlDir =
        argc > 2 ? std::filesystem::path(argv[2])
                 : std::filesystem::path("generated/example_08/xml_only");
    const std::filesystem::path codeDir =
        argc > 3 ? std::filesystem::path(argv[3])
                 : std::filesystem::path("generated/example_08/code_only");

    try {
        libfmu_toolkit::ModelDescriptionGenerator xmlGenerator;
        libfmu_toolkit::FmiCCodeGenerator         codeGenerator;

        const auto xmlPath  = xmlGenerator.writeFromJson(jsonPath.string(), xmlDir);
        const auto codePath = codeGenerator.writeFromJson(jsonPath.string(), codeDir);

        std::cout << "JSON input             : " << jsonPath << "\n";
        std::cout << "XML only               : " << xmlPath << "\n";
        std::cout << "C code only            : " << codePath << "\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}
