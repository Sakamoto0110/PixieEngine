using System.Runtime.InteropServices;
using Hexa.NET.GLFW;
using PixieEditor.Native;
using PixieEditor.Ui;

static class Program
{
    static void Check(bool condition, string name)
    {
        if (!condition) throw new InvalidOperationException(name);
        Console.WriteLine($"PASS {name}");
    }

    static unsafe int Main(string[] args)
    {
        if (args.Length != 1) return 2;
        var engine = Path.GetFullPath(args[0]);
        NativeLibrary.SetDllImportResolver(typeof(EngineSession).Assembly,
            (name, _, _) => name == "pixie_capi" ? NativeLibrary.Load(engine) : 0);
        var root = Path.Combine(Path.GetTempPath(), "pixie-start-screen-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            var browser = new FolderBrowser(root);
            Check(browser.CreateFolder("Project locations"), "create a folder with spaces");
            Check(browser.Current == Path.Combine(root, "Project locations"), "navigate into the new folder");
            Check(!browser.CreateFolder(" ") && !browser.CreateFolder(".."), "reject empty and parent names");
            if (OperatingSystem.IsWindows())
                Check(!browser.CreateFolder("trailing."), "reject Windows trailing-dot aliases");
            Check(!browser.CreateFolder("../outside") && !Directory.Exists(Path.Combine(root, "outside")), "reject traversal");
            browser.Go(root);
            Check(!browser.CreateFolder("Project locations"), "refuse an existing folder");
            File.WriteAllText(Path.Combine(root, "keep.txt"), "preserve");
            Check(!browser.CreateFolder("keep.txt") && File.ReadAllText(Path.Combine(root, "keep.txt")) == "preserve", "preserve an existing file");

            var project = Path.Combine(root, "New game");
            EngineSession.CreateProject(project);
            Check(File.Exists(Path.Combine(project, "New game.pixie")), "create project descriptor");
            Check(Directory.GetFiles(Path.Combine(project, "scenes")).Length > 0, "create starting scene");
            Check(FolderBrowser.IsProject(project), "recognize created project in browser");
            if (GLFW.Init() == 0) throw new InvalidOperationException("GLFW initialization failed");
            GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
            GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4);
            GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
            GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
            var window = GLFW.CreateWindow(320, 240, "Project creation test", default, default);
            if (window.Handle == null) throw new InvalidOperationException("OpenGL window creation failed");
            try
            {
                GLFW.MakeContextCurrent(window);
                using var session = EngineSession.OpenProject(320, 240, project);
                Check(session.Editing && session.Entities().Length > 0, "open new project in edit mode with its starting entity");
            }
            finally { GLFW.DestroyWindow(window); GLFW.Terminate(); }
            return 0;
        }
        finally { Directory.Delete(root, recursive: true); }
    }
}
