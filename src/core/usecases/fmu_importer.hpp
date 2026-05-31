#pragma once

/// @file fmu_importer.hpp
/// @brief Use case: import an FMU archive into a parsed FmuPackage.
/// @internal

#include "core/domain/fmu_package.hpp"
#include "ports/i_fmu_loader.hpp"

#include <filesystem>

namespace libfmu_toolkit {

/// @brief Orchestrates loading an FMU archive into an in-memory FmuPackage.
///
/// Delegates the platform-specific work (ZIP extraction, XML parsing, shared
/// library loading, FMI symbol resolution) to an injected IFmuLoader port, so
/// the use case itself stays free of any libzip/libxml2 dependency.
///
/// @par Requirements
/// Implements FR-01.1 … FR-01.6 (archive loading, metadata parsing, symbol
/// resolution, multi-FMU support) and NFR-03.1, NFR-03.2 (hexagonal layering,
/// constructor injection).
/// @req FR-01.1
class FmuImporter {
public:
    /// @brief Constructs the importer with an injected loader port.
    /// @param loader Loader adapter used to extract and parse the archive.
    /// @req FR-01.1
    explicit FmuImporter(IFmuLoader& loader) noexcept;
    ~FmuImporter() = default;

    FmuImporter(const FmuImporter&)            = delete;
    FmuImporter& operator=(const FmuImporter&) = delete;
    FmuImporter(FmuImporter&&)                 = delete;
    FmuImporter& operator=(FmuImporter&&)      = delete;

    /// @brief Imports an FMU from a file path into a fully populated FmuPackage.
    /// @param fmuPath Path to the `.fmu` archive.
    /// @returns The parsed package with metadata and resolved FMI symbols.
    /// @throws FmuLoadException if the archive, library, or symbols cannot be resolved.
    /// @req FR-01.1
    [[nodiscard]] FmuPackage import(const std::filesystem::path& fmuPath);

private:
    IFmuLoader& loader_; ///< Injected loader port (not owned).
};

} // namespace libfmu_toolkit
