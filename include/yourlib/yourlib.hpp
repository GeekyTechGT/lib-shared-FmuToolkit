#pragma once

#include <yourlib/export.hpp>
#include <yourlib/version.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace yourlib {

class YOUR_LIBRARY_API LibraryInfo final {
public:
    explicit LibraryInfo(std::string library_name = "your_library");
    ~LibraryInfo();

    LibraryInfo(LibraryInfo&&) noexcept;
    LibraryInfo& operator=(LibraryInfo&&) noexcept;

    LibraryInfo(const LibraryInfo&);
    LibraryInfo& operator=(const LibraryInfo&);

    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string version() const;
    [[nodiscard]] std::string describe(std::string_view consumer) const;
    void set_name(std::string library_name);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace yourlib
