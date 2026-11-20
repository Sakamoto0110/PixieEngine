#include <cstdio>

#include "check.hpp"

void TestSize();
void TestPoint();
void TestRect();
void TestRegion();
void TestPadding();
void TestColorRgba();
void TestColorHsl();
void TestToString();
#ifdef PIXIE_TEST_MATH
void TestVec();
void TestMat();
void TestSegment();
#endif
#ifdef PIXIE_TEST_EVENTS
void TestCallback();
void TestEventHandler();
void TestSubscription();
void TestEvent();

#endif

int main() {
    TestSize();
    TestPoint();
    TestRect();
    TestRegion();
    TestPadding();
    TestColorRgba();
    TestColorHsl();
    TestToString();
#ifdef PIXIE_TEST_MATH
    TestVec();
    TestMat();
    TestSegment();
#endif
#ifdef PIXIE_TEST_EVENTS
    TestCallback();
    TestEventHandler();
    TestSubscription();
    TestEvent();

#endif

    if (pxCheckFailures == 0)
        std::printf("all checks passed\n");
    else
        std::printf("%d check(s) failed\n", pxCheckFailures);
    return pxCheckFailures == 0 ? 0 : 1;
}
