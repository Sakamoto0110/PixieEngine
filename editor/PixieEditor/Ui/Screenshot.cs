using System.Buffers.Binary;
using System.IO.Compression;

namespace PixieEditor.Ui;

// Saves the window's framebuffer as an RGB PNG, top row first: for checking a run without watching it.
internal static class Screenshot
{
    public static unsafe void Save(string path, int width, int height)
    {
        var pixels = new byte[width * height * 4];
        Gl.PixelStorei(Gl.PackAlignment, 1);
        fixed (byte* p = pixels)
        {
            Gl.ReadPixels(0, 0, width, height, Gl.Rgba, Gl.UnsignedByte, p);
        }

        // Rows bottom to top in GL; each PNG row starts with its filter type (0, none).
        var raw = new byte[height * (width * 3 + 1)];
        for (var row = 0; row < height; row++)
        {
            var source = (height - 1 - row) * width * 4;
            var target = row * (width * 3 + 1) + 1;
            for (var x = 0; x < width; x++)
            {
                raw[target + x * 3] = pixels[source + x * 4];
                raw[target + x * 3 + 1] = pixels[source + x * 4 + 1];
                raw[target + x * 3 + 2] = pixels[source + x * 4 + 2];
            }
        }

        using var compressed = new MemoryStream();
        using (var zlib = new ZLibStream(compressed, CompressionLevel.Fastest, leaveOpen: true))
        {
            zlib.Write(raw);
        }

        using var file = File.Create(path);
        file.Write([0x89, (byte)'P', (byte)'N', (byte)'G', 0x0D, 0x0A, 0x1A, 0x0A]);
        var header = new byte[13];
        BinaryPrimitives.WriteInt32BigEndian(header, width);
        BinaryPrimitives.WriteInt32BigEndian(header.AsSpan(4), height);
        header[8] = 8; // bits per channel
        header[9] = 2; // RGB
        Chunk(file, "IHDR", header);
        Chunk(file, "IDAT", compressed.ToArray());
        Chunk(file, "IEND", []);
    }

    private static void Chunk(Stream stream, string type, byte[] data)
    {
        Span<byte> length = stackalloc byte[4];
        BinaryPrimitives.WriteInt32BigEndian(length, data.Length);
        stream.Write(length);
        var typeBytes = System.Text.Encoding.ASCII.GetBytes(type);
        stream.Write(typeBytes);
        stream.Write(data);
        Span<byte> sum = stackalloc byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(sum, ~Crc(Crc(0xFFFFFFFFu, typeBytes), data));
        stream.Write(sum);
    }

    private static readonly uint[] CrcTable = Enumerable.Range(0, 256).Select(n =>
    {
        var c = (uint)n;
        for (var k = 0; k < 8; k++)
            c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        return c;
    }).ToArray();

    private static uint Crc(uint crc, ReadOnlySpan<byte> data)
    {
        foreach (var b in data)
            crc = CrcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        return crc;
    }
}
