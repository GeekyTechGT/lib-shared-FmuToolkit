#pragma once

#include "core/domain/fmu_package.hpp"
#include "ports/i_fmu_loader.hpp"

#include <filesystem>

namespace libfmu_toolkit {

class FmuImporter {
public:
    explicit FmuImporter(IFmuLoader& loader) noexcept;
    ~FmuImporter() = default;

    FmuImporter(const FmuImporter&)            = delete;
    FmuImporter& operator=(const FmuImporter&) = delete;
    FmuImporter(FmuImporter&&)                 = delete;
    FmuImporter& operator=(FmuImporter&&)      = delete;

    [[nodiscard]] FmuPackage import(const std::filesystem::path& fmuPath);

private:
    IFmuLoader& loader_;
};

} // namespace libfmu_toolkit
