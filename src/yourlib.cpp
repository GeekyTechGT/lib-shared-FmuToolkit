#include <yourlib/yourlib.hpp>

#include "yourlib_impl.hpp"

#include <utility>

namespace yourlib {

LibraryInfo::Impl::Impl(std::string library_name)
    : library_name_(std::move(library_name)) {}

std::string LibraryInfo::Impl::name() const {
    return library_name_;
}

std::string LibraryInfo::Impl::version() const {
    return YOUR_LIBRARY_VERSION;
}

std::string LibraryInfo::Impl::describe(std::string_view consumer) const {
    return library_name_ + " " + YOUR_LIBRARY_VERSION + " ready for " + std::string{consumer};
}

void LibraryInfo::Impl::set_name(std::string library_name) {
    library_name_ = std::move(library_name);
}

LibraryInfo::LibraryInfo(std::string library_name)
    : impl_(std::make_unique<Impl>(std::move(library_name))) {}

LibraryInfo::~LibraryInfo() = default;
LibraryInfo::LibraryInfo(LibraryInfo&&) noexcept = default;
LibraryInfo& LibraryInfo::operator=(LibraryInfo&&) noexcept = default;

LibraryInfo::LibraryInfo(const LibraryInfo& other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}

LibraryInfo& LibraryInfo::operator=(const LibraryInfo& other) {
    if (this != &other) {
        impl_ = std::make_unique<Impl>(*other.impl_);
    }
    return *this;
}

std::string LibraryInfo::name() const {
    return impl_->name();
}

std::string LibraryInfo::version() const {
    return impl_->version();
}

std::string LibraryInfo::describe(std::string_view consumer) const {
    return impl_->describe(consumer);
}

void LibraryInfo::set_name(std::string library_name) {
    impl_->set_name(std::move(library_name));
}

} // namespace yourlib
