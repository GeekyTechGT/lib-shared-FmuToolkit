#pragma once

/// @file zip_loader_adapter.hpp
/// @brief libzip-backed adapter implementing the IFmuLoader port.
/// @internal

#include "domain/port/out/i_fmu_loader.hpp"
#include <filesystem>
#include <vector>

namespace libfmu_toolkit {

/// @brief Loads an FMU by extracting its ZIP archive and resolving FMI symbols.
///
/// Concrete IFmuLoader implementation backed by libzip. Each instance owns the
/// extraction directory and the OS shared library handles it opens, releasing
/// them on destruction.
///
/// @par Requirements
/// Implements FR-01.1, FR-01.3, FR-01.4, FR-01.5 (archive extraction, shared
/// library loading, FMI 2.0/3.0 symbol resolution, error reporting).
/// @req FR-01.1
class ZipLoaderAdapter final : public IFmuLoader {
public:
    /// @brief Constructs the adapter.
    /// @param extractionRoot Directory under which archives are extracted; a
    ///        temporary directory is used when empty.
    /// @req FR-01.1
    explicit ZipLoaderAdapter(std::filesystem::path extractionRoot = {});
    ~ZipLoaderAdapter() override;

    /// @brief Extracts and parses the FMU at @p fmuPath (IFmuLoader port).
    /// @returns A FmuPackage with metadata and resolved FMI function pointers.
    /// @throws FmuLoadException on extraction, library, or symbol resolution failure.
    /// @req FR-01.1
    [[nodiscard]] FmuPackage load(const std::filesystem::path& fmuPath) override;

private:
    std::filesystem::path extractionRoot_;   ///< Root directory for extracted archives.
    std::vector<void*> loadedHandles_;       ///< OS library handles owned by this adapter.

    /// @brief Extracts the archive and returns the extraction directory.
    /// @req FR-01.1
    std::filesystem::path extractFmu(const std::filesystem::path& fmuPath);
    /// @brief Locates the platform shared library inside an extracted FMU.
    /// @req FR-01.3
    std::filesystem::path findSharedLibrary(const std::filesystem::path& extractedDir,
                                            const std::string& modelName);
    /// @brief Opens an OS shared library and tracks its handle for cleanup.
    /// @req FR-01.3
    void* loadSharedLibrary(const std::filesystem::path& libPath);
    /// @brief Resolves the FMI 2.0 function pointers into @p pkg.
    /// @req FR-01.3
    void  loadFmi2FunctionPointers(FmuPackage& pkg);
    /// @brief Resolves the FMI 3.0 function pointers into @p pkg.
    /// @req FR-01.3
    void  loadFmi3FunctionPointers(FmuPackage& pkg);
    /// @brief Resolves a single named symbol from an OS library handle.
    /// @req FR-01.3
    static void* resolveSymbol(void* handle, const char* name);
};

} // namespace libfmu_toolkit
