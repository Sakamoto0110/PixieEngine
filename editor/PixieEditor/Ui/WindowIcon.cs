using System.Buffers.Binary;
using Hexa.NET.GLFW;

namespace PixieEditor.Ui;

// The editor's icon on its window (title bar, task bar, Alt + Tab), from the same .ico the executable
// carries, embedded in the assembly. GLFW takes RGBA pixels on every platform, so the icon's 32-bit
// frames are decoded here rather than loaded through Win32. Frames stored as PNG are skipped.
internal static unsafe class WindowIcon
{
    private const string Resource = "PixieEditor.assets.neko-purple-light.ico";

    public static void Apply(GLFWwindowPtr window)
    {
        using var stream = typeof(WindowIcon).Assembly.GetManifestResourceStream(Resource);
        if (stream == null)
            return;

        using var memory = new MemoryStream();
        stream.CopyTo(memory);
        var frames = Decode(memory.ToArray());
        if (frames.Count == 0)
            return;

        var handles = frames.Select(f => System.Runtime.InteropServices.GCHandle.Alloc(f.Pixels, System.Runtime.InteropServices.GCHandleType.Pinned)).ToArray();
        try
        {
            var images = stackalloc GLFWimage[frames.Count];
            for (var i = 0; i < frames.Count; i++)
            {
                images[i].Width = frames[i].Size;
                images[i].Height = frames[i].Size;
                images[i].Pixels = (byte*)handles[i].AddrOfPinnedObject();
            }

            GLFW.SetWindowIcon(window, frames.Count, images);
        }
        finally
        {
            foreach (var handle in handles)
                handle.Free();
        }
    }

    // The square frames stored as 32-bit bitmaps, as RGBA rows from the top.
    private static List<(int Size, byte[] Pixels)> Decode(byte[] ico)
    {
        var frames = new List<(int, byte[])>();
        var count = BinaryPrimitives.ReadUInt16LittleEndian(ico.AsSpan(4));
        for (var i = 0; i < count; i++)
        {
            var entry = ico.AsSpan(6 + 16 * i);
            var size = entry[0] == 0 ? 256 : entry[0];
            var offset = (int)BinaryPrimitives.ReadUInt32LittleEndian(entry[12..]);
            var bitmap = ico.AsSpan(offset);
            // A PNG frame starts with its signature; a bitmap with its header's size.
            if (bitmap[0] == 0x89)
                continue;
            var headerSize = (int)BinaryPrimitives.ReadUInt32LittleEndian(bitmap);
            var bits = BinaryPrimitives.ReadUInt16LittleEndian(bitmap[14..]);
            if (bits != 32)
                continue;

            var pixels = new byte[size * size * 4];
            var source = bitmap[headerSize..];
            for (var y = 0; y < size; y++)
            {
                // Bitmap rows run from the bottom; BGRA becomes RGBA.
                var row = source.Slice((size - 1 - y) * size * 4, size * 4);
                for (var x = 0; x < size; x++)
                {
                    var to = (y * size + x) * 4;
                    pixels[to] = row[x * 4 + 2];
                    pixels[to + 1] = row[x * 4 + 1];
                    pixels[to + 2] = row[x * 4];
                    pixels[to + 3] = row[x * 4 + 3];
                }
            }

            frames.Add((size, pixels));
        }

        return frames;
    }
}
