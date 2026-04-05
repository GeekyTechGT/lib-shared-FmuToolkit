#include "yourlib/yourlib.hpp"

#include <gtest/gtest.h>

TEST(GreeterTests, UsesPrefixAndName) {
    const yourlib::Greeter greeter{"Hi"};

    EXPECT_EQ(greeter.greet("Codex"), "Hi, Codex!");
}

TEST(GreeterTests, AllowsPrefixMutation) {
    yourlib::Greeter greeter;
    greeter.set_prefix("Welcome");

    EXPECT_EQ(greeter.greet("team"), "Welcome, team!");
}
