#pragma once
#include "ports/i_fmu_loader.hpp"
#include <filesystem>
#include <vector>

namespace libfmu_toolkit {

class ZipLoaderAdapter final : public IFmuLoader {
public:
    explicit ZipLoaderAdapter(std::filesystem::path extractionRoot = {});
    ~ZipLoaderAdapter() override;

    [[nodiscard]] FmuPackage load(const std::filesystem::path& fmuPath) override;

private:
    std::filesystem::path extractionRoot_;
    std::vector<void*> loadedHandles_;

    std::filesystem::path extractFmu(const std::filesystem::path& fmuPath);
    std::filesystem::path findSharedLibrary(const std::filesystem::path& extractedDir,
                                            const std::string& modelName);
    void* loadSharedLibrary(const std::filesystem::path& libPath);
    void  loadFmi2FunctionPointers(FmuPackage& pkg);
    void  loadFmi3FunctionPointers(FmuPackage& pkg);
    static void* resolveSymbol(void* handle, const char* name);
};

} // namespace libfmu_toolkit
