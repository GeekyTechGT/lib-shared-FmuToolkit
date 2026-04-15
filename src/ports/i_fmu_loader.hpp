#pragma once

#include "core/domain/fmu_package.hpp"

#include <filesystem>

namespace libfmu_toolkit {

/// @brief Port interface for loading FMU archives.
///
/// Implemented by ZipLoaderAdapter. One instance is created per FmuEngine::load()
/// call and stored in FmuEngine::Impl::Entry so the OS library handle lifetime
/// is tied to the FmuPackage lifetime.
class IFmuLoader {
public:
    virtual ~IFmuLoader() = default;

    /// @brief Loads an FMU from a ZIP archive.
    ///
    /// Extracts the archive, parses `modelDescription.xml`, loads the platform
    /// shared library, and resolves FMI symbols.
    ///
    /// @param fmuPath  Path to the `.fmu` file.
    /// @returns        Populated FmuPackage including function-pointer tables.
    ///
    /// @throws FmuLoadException on any I/O, parse, or symbol-resolution error.
    [[nodiscard]] virtual FmuPackage load(const std::filesystem::path& fmuPath) = 0;

protected:
    IFmuLoader() = default;
};

} // namespace libfmu_toolkit
