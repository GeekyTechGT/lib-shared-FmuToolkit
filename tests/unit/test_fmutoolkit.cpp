// Tests go through the public header only - they double as API/ABI
// regression tests for the PIMPL interface.
#include "fmutoolkit/fmutoolkit.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace fmutoolkit {
namespace {

static_assert(std::is_nothrow_move_constructible_v<FmuToolkitService>);
static_assert(std::is_nothrow_move_assignable_v<FmuToolkitService>);
static_assert(std::is_copy_constructible_v<FmuToolkitService>);

TEST(FmuToolkitServiceTest, GreetsTheLibraryWhenNoRecipientIsGiven) {
    const FmuToolkitService service;
    EXPECT_EQ(service.greeting(), "Hello from FmuToolkit");
}

TEST(FmuToolkitServiceTest, GreetsTheGivenRecipient) {
    const FmuToolkitService service;
    EXPECT_EQ(service.greeting("Ada"), "Hello from FmuToolkit, Ada");
}

TEST(FmuToolkitServiceTest, CountsGreetings) {
    const FmuToolkitService service;
    (void)service.greeting();
    (void)service.greeting("Ada");
    EXPECT_EQ(service.greetingCount(), 2U);
}

TEST(FmuToolkitServiceTest, CopiesAreDeepAndIndependent) {
    const FmuToolkitService original;
    (void)FmuToolkitService{original}.greeting();  // mutate a temporary copy only

    FmuToolkitService copy{original};
    (void)copy.greeting();

    EXPECT_EQ(original.greetingCount(), 0U);
    EXPECT_EQ(copy.greetingCount(), 1U);
}

TEST(FmuToolkitServiceTest, CopyAssignmentReplacesState) {
    FmuToolkitService source;
    (void)source.greeting();
    FmuToolkitService target;
    target = source;
    EXPECT_EQ(target.greetingCount(), 1U);
}

TEST(FmuToolkitServiceTest, MoveTransfersTheImplementation) {
    FmuToolkitService source;
    (void)source.greeting();

    FmuToolkitService moved{std::move(source)};
    EXPECT_EQ(moved.greetingCount(), 1U);
    EXPECT_EQ(moved.greeting("Ada"), "Hello from FmuToolkit, Ada");

    source = FmuToolkitService{};  // a moved-from object can be re-assigned
    EXPECT_EQ(source.greetingCount(), 0U);
}

} // namespace
} // namespace fmutoolkit
