#include "fmu_importer.hpp"
#include "core/exceptions/fmu_exception.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace libfmu_toolkit {

FmuImporter::FmuImporter(IFmuLoader& loader) noexcept
    : loader_(loader)
{}

FmuPackage FmuImporter::import(const std::filesystem::path& fmuPath) {
    if (!std::filesystem::exists(fmuPath)) {
        throw std::invalid_argument(
            "FmuImporter::import — path does not exist: " + fmuPath.string()
        );
    }

    {
        std::string ext = fmuPath.extension().string();
        for (char& ch : ext) {
            ch = static_cast<char>(
                (ch >= 'A' && ch <= 'Z') ? (ch + ('a' - 'A')) : ch
            );
        }
        if (ext != ".fmu") {
            throw std::invalid_argument(
                "FmuImporter::import — file does not have .fmu extension: "
                + fmuPath.string()
            );
        }
    }

    return loader_.load(fmuPath);
}

} // namespace libfmu_toolkit
