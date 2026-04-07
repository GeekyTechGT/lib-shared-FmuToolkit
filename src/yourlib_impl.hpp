#pragma once

#include <string>
#include <string_view>

namespace yourlib {

class LibraryInfo::Impl final {
public:
    explicit Impl(std::string library_name);

    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string version() const;
    [[nodiscard]] std::string describe(std::string_view consumer) const;
    void set_name(std::string library_name);

private:
    std::string library_name_;
};

} // namespace yourlib
