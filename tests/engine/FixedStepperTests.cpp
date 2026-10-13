#include "Check.hpp"

#include "pixie/FixedStepper.hpp"

#include <cmath>
#include <vector>

using pixie::FixedStepper;
using pixie::FrameTime;

TEST(StepsComeOutAtTheFixedRateWithTheirIndexAndTime) {
    FixedStepper stepper(60.0);
    std::vector<FrameTime> steps;
    const auto record = [&](const FrameTime& t) { steps.push_back(t); };
    stepper.Advance(0.010, record); // less than a step: nothing yet
    CHECK(steps.empty());
    stepper.Advance(0.030, record); // 40 ms in all: two steps
    CHECK(steps.size() == 2);
    CHECK(steps[0].index == 0 && steps[1].index == 1);
    CHECK(steps[1].elapsed == steps[0].delta);
    CHECK(std::abs(steps[0].delta - 1.0 / 60.0) < 1e-15);
    CHECK(stepper.Steps() == 2);
}

TEST(AlphaIsTheLeftoverFractionOfAStep) {
    FixedStepper stepper(10.0);
    stepper.Advance(0.25, [](const FrameTime&) {});
    const FrameTime frame = stepper.RenderTime(0.25, 7);
    CHECK(std::abs(frame.alpha - 0.5) < 1e-9);
    CHECK(frame.index == 7 && frame.delta == 0.25);
}

TEST(LongFramesAreClampedAndDropForgetsTheLeftover) {
    FixedStepper stepper(60.0);
    int count = 0;
    stepper.Advance(5.0, [&](const FrameTime&) { ++count; });
    CHECK(count == 15); // 0.25 s at most
    stepper.Advance(0.01, [&](const FrameTime&) { ++count; });
    stepper.Drop();
    CHECK(stepper.RenderTime(0.0, 0).alpha == 0.0);
}
