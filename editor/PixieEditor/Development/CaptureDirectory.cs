namespace PixieEditor.Development;

internal sealed class CaptureDirectory
{
    public string Root { get; }
    public CaptureDirectory(string path)
    {
        // Refuse traversal before GetFullPath can hide a junction followed by '..'.
        if (path.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar).Contains(".."))
            throw new ArgumentException("Capture root cannot contain parent traversal.");
        Root = Path.GetFullPath(path);
        Check();
        Directory.CreateDirectory(Root);
        Check();
    }
    public string Resolve(string name)
    {
        if (name.EndsWith(".png", StringComparison.Ordinal)) name = name[..^4];
        if (name.Length is < 1 or > 64 || name.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-' && c != '_'))
            throw new ArgumentException("Capture name must be a bounded ASCII basename.");
        var upper = name.ToUpperInvariant();
        if (upper is "CON" or "PRN" or "AUX" or "NUL" ||
            upper.Length == 4 && (upper.StartsWith("COM", StringComparison.Ordinal) || upper.StartsWith("LPT", StringComparison.Ordinal)) && upper[3] is >= '1' and <= '9')
            throw new ArgumentException("Reserved capture basename.");
        Check();
        var path = Path.Combine(Root, name + ".png");
        if (File.Exists(path) || Directory.Exists(path)) throw new IOException("Capture already exists; choose a fresh basename.");
        return path;
    }
    public void Check()
    {
        var path = Path.GetPathRoot(Root)!;
        foreach (var part in Root[path.Length..].Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar))
        {
            if (part.Length == 0) continue;
            path = Path.Combine(path, part);
            if ((File.Exists(path) || Directory.Exists(path)) && (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Capture root contains a reparse point.");
        }
    }
}
