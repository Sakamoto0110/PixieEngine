namespace PixieEditor.Editing;

// The images a project's sprites use are files inside its folder, named by their path relative to
// it, with '/'. An image from elsewhere is copied into the project's images/ folder first, so the
// project stays whole when it is opened on another machine.
internal static class ProjectImages
{
    public const string Folder = "images";

    // Whether `file` is an image a sprite can draw: a PNG.
    public static bool IsImage(string file) => Path.GetExtension(file).Equals(".png", StringComparison.OrdinalIgnoreCase);

    // The path a sprite names for `file`, and the file it copied into the project to get it, if it
    // did. A file already inside the project is used where it is; one outside is copied into images/,
    // under its own name, or a numbered one when another file has it, and an identical file already
    // there is used instead of a second copy. Without a project, the file's full path.
    public static (string Path, string? CopiedTo) Import(string? projectFolder, string file)
    {
        var full = System.IO.Path.GetFullPath(file);
        if (projectFolder == null)
            return (full.Replace('\\', '/'), null);

        var project = System.IO.Path.GetFullPath(projectFolder);
        if (Inside(project, full))
            return (Relative(project, full), null);

        var images = System.IO.Path.Combine(project, Folder);
        Directory.CreateDirectory(images);
        var name = System.IO.Path.GetFileNameWithoutExtension(full);
        var extension = System.IO.Path.GetExtension(full).ToLowerInvariant();
        var target = System.IO.Path.Combine(images, name + extension);
        for (var n = 2; File.Exists(target) && !SameContent(target, full); n++)
            target = System.IO.Path.Combine(images, $"{name} {n}{extension}");

        if (File.Exists(target))
            return (Relative(project, target), null);
        File.Copy(full, target);
        return (Relative(project, target), Relative(project, target));
    }

    // The folder an image chooser starts in: the project's images/ when it exists, else the project.
    public static string? StartFolder(string? projectFolder)
    {
        if (projectFolder == null)
            return null;
        var images = System.IO.Path.Combine(projectFolder, Folder);
        return Directory.Exists(images) ? images : projectFolder;
    }

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
