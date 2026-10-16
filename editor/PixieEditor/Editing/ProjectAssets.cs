namespace PixieEditor.Editing;

// The files a project's components name (images, fonts) are inside its folder, named by their path
// relative to it, with '/'. A file from elsewhere is copied into the project's folder for its kind
// first (images/ for a PNG, fonts/ for a TrueType font), so the project stays whole when it is opened
// on another machine.
internal static class ProjectAssets
{
    // The folder of the project that files of a kind are copied into, by the kind's extension
    // without the dot, as a field's asset kind names it.
    public static string FolderFor(string kind) => kind.ToLowerInvariant() switch
    {
        "png" => "images",
        "ttf" => "fonts",
        _ => "assets",
    };

    // Whether `file` is of the asset kind `kind` ("png", "ttf").
    public static bool IsKind(string file, string kind) =>
        Path.GetExtension(file).Equals("." + kind, StringComparison.OrdinalIgnoreCase);

    // Whether `file` is an image a sprite can draw: a PNG.
    public static bool IsImage(string file) => IsKind(file, "png");

    // The path a component names for `file`, and the file it copied into the project to get it, if it
    // did. A file already inside the project is used where it is; one outside is copied into the
    // folder for its kind, under its own name, or a numbered one when another file has it, and an
    // identical file already there is used instead of a second copy. Without a project, the file's
    // full path.
    public static (string Path, string? CopiedTo) Import(string? projectFolder, string file)
    {
        var full = System.IO.Path.GetFullPath(file);
        if (projectFolder == null)
            return (full.Replace('\\', '/'), null);

        var project = System.IO.Path.GetFullPath(projectFolder);
        if (Inside(project, full))
            return (Relative(project, full), null);

        var kind = System.IO.Path.GetExtension(full).TrimStart('.');
        var folder = System.IO.Path.Combine(project, FolderFor(kind));
        Directory.CreateDirectory(folder);
        var name = System.IO.Path.GetFileNameWithoutExtension(full);
        var extension = System.IO.Path.GetExtension(full).ToLowerInvariant();
        var target = System.IO.Path.Combine(folder, name + extension);
        for (var n = 2; File.Exists(target) && !SameContent(target, full); n++)
            target = System.IO.Path.Combine(folder, $"{name} {n}{extension}");

        if (File.Exists(target))
            return (Relative(project, target), null);
        File.Copy(full, target);
        return (Relative(project, target), Relative(project, target));
    }

    // The folder a chooser of files of `kind` starts in: the project's folder for that kind when it
    // exists, else the project.
    public static string? StartFolder(string? projectFolder, string kind)
    {
        if (projectFolder == null)
            return null;
        var folder = System.IO.Path.Combine(projectFolder, FolderFor(kind));
        return Directory.Exists(folder) ? folder : projectFolder;
    }

    // The first file of `kind` in the project, by path, looking in its folder for that kind first, as a
    // project-relative path; null when it has none. The build's folders are not looked in.
    public static string? First(string? projectFolder, string kind)
    {
        if (projectFolder == null || !Directory.Exists(projectFolder))
            return null;
        var project = System.IO.Path.GetFullPath(projectFolder);
        var preferred = System.IO.Path.Combine(project, FolderFor(kind));
        try
        {
            var files = Directory.EnumerateFiles(project, "*." + kind, SearchOption.AllDirectories)
                .Where(f => IsKind(f, kind))
                .Select(f => Relative(project, f))
                .Where(f => !f.StartsWith("build/", StringComparison.OrdinalIgnoreCase) &&
                            !f.StartsWith("bin/", StringComparison.OrdinalIgnoreCase))
                .OrderBy(f => f.StartsWith(Relative(project, preferred) + "/", StringComparison.OrdinalIgnoreCase) ? 0 : 1)
                .ThenBy(f => f, StringComparer.OrdinalIgnoreCase);
            return files.FirstOrDefault();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    // Whether the file a project-relative `path` names exists.
    public static bool Exists(string? projectFolder, string path) =>
        File.Exists(projectFolder == null ? path : System.IO.Path.Combine(projectFolder, path));

    private static bool Inside(string folder, string file)
    {
        var relative = System.IO.Path.GetRelativePath(folder, file);
        var outside = relative == ".." || relative.StartsWith("../", StringComparison.Ordinal) ||
                      relative.StartsWith("..\\", StringComparison.Ordinal);
        return !outside && !System.IO.Path.IsPathRooted(relative);
    }

    private static string Relative(string folder, string file) => System.IO.Path.GetRelativePath(folder, file).Replace('\\', '/');

    private static bool SameContent(string a, string b) =>
        new FileInfo(a).Length == new FileInfo(b).Length && File.ReadAllBytes(a).AsSpan().SequenceEqual(File.ReadAllBytes(b));
}
