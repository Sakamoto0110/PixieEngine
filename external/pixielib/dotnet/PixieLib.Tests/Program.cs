using PixieLib.Tests;

LayoutTests.Run();
PxSizeTests.Run();
PxPointTests.Run();
PxRectTests.Run();
PxRegionTests.Run();
PxPaddingTests.Run();
PxColorRgbaTests.Run();
PxColorHslTests.Run();
ToStringTests.Run();
PxVecTests.Run();
PxMatTests.Run();
PxSegmentTests.Run();
SystemDrawingTests.Run();

Console.WriteLine(Check.Failures == 0 ? "all checks passed" : $"{Check.Failures} check(s) failed");
return Check.Failures == 0 ? 0 : 1;
