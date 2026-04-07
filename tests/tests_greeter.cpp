#include <yourlib/yourlib.hpp>

#include <gtest/gtest.h>

TEST(LibraryInfoTest, UsesDefaultName) {
    const yourlib::LibraryInfo info;
    EXPECT_EQ(info.name(), "your_library");
}

TEST(LibraryInfoTest, ReportsGeneratedVersion) {
    const yourlib::LibraryInfo info;
    EXPECT_FALSE(info.version().empty());
}

TEST(LibraryInfoTest, DescribesConsumer) {
    const yourlib::LibraryInfo info{"demo_lib"};
    EXPECT_EQ(info.describe("integration-test"), "demo_lib " YOUR_LIBRARY_VERSION " ready for integration-test");
}
