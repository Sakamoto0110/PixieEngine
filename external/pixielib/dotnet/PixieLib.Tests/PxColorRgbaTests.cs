namespace PixieLib.Tests;

internal static class PxColorRgbaTests
{
    public static void Run()
    {
        var c = new PxColorRgba(0x11, 0x22, 0x33, 0x44);
        Check.That(new PxColorRgba(1, 2, 3) == new PxColorRgba(1, 2, 3, 255));
        Check.That(PxColorRgba.Empty.IsEmpty);
        Check.That(!c.IsEmpty);

        // The hex number as it is written, 0xRRGGBBAA, whatever the order in memory.
        Check.That(c.ToHex() == 0x11223344u);
        Check.That(PxColorRgba.FromHex(0x11223344u) == c);
        Check.That(PxColorRgba.FromHex(0xFF000080u) == new PxColorRgba(255, 0, 0, 128));

        // The 0xAARRGGBB of System.Drawing.
        Check.That(c.ToArgb() == 0x44112233);
        Check.That(PxColorRgba.FromArgb(0x44112233) == c);
        Check.That(new PxColorRgba(1, 2, 3, 200).ToArgb() == unchecked((int)0xC8010203));
    }
}
