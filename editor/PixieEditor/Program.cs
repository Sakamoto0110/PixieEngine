using System.Globalization;
using PixieEditor.Native;
using PixieEditor.Ui;

namespace PixieEditor;

// What the command line asks for.
internal sealed record EditorOptions
{
    // A game built in code, by name or library path; or a project folder; with neither, the start
    // screen.
    public string? Game { get; init; }
    public string? Project { get; init; }
    public ulong Seed { get; init; } = 1;
    // The entity, by name, selected at the start.
    public string? Select { get; init; }
    // Edits of the selected entity, applied through its inspectors after EditAfter seconds.
    public List<(string Type, string Property, string Value)> Edits { get; } = [];
    public double EditAfter { get; init; } = 1.0;
    public double QuitAfter { get; init; }
    public string? Screenshot { get; init; }
    public double? TimeScale { get; init; }
    // What the mouse does at the start; Game for a game, Select for a project, which opens in edit mode.
    public ViewportMode? Mode { get; init; }
    public bool Paused { get; init; }
    // An earlier editor to wait for, so a game library it held can be built again.
    public int? After { get; init; }
    // When a project is saved without a hand on the keyboard, after the edits.
    public double? SaveAfter { get; init; }
    // Whether the project goes into the user's recent projects; off for scripted runs.
    public bool Recent { get; init; } = true;
    // Editing commands run one per frame after the --set edits, as the editor's gestures would.
    public List<string> Commands { get; } = [];
}

internal static class Program
{
    private const string Usage = """
        usage: PixieEditor [options]
          With neither --game nor --project, the editor opens its start screen: new project, open project, recent ones.
          --project <folder>       open the project in this folder, in edit mode; its code, if any, is built first
          --game <name|path>       open a game built in code: boids, flocks, particles, or a path to its capi library
          --seed <n>               seed of the World (default 1)
          --select <name>          select the entity with this name at the start
          --set <Type.Property=v>  after --edit-after seconds, set a property of the selected entity through its inspector; repeatable
          --edit-after <seconds>   when the --set edits happen (default 1)
          --quit-after <seconds>   close after this long; 0 means never (default 0)
          --screenshot <file.png>  with --quit-after, save the last frame of the window
          --time-scale <x>         start with simulated time at this speed (default 1)
          --mode <game|select>     what the mouse does in the viewport at the start (default game; select for a project)
          --paused <on|off>        start with time paused (default off)
          --save-after <seconds>   save the project after this long (after the --set edits)
          --recent <on|off>        add the project to the recent projects (default on; off for scripted runs)
          --do <command>           after the --set edits, one per frame: create, collider, character, dynamic, kinematic, sensor, world,
                                  compoundbox, compound, addshape, removeshape=<index>, shape=<index>:<field>:<value>, wait=<seconds>,
                                   duplicate, delete, undo, redo, colliderhandles, physicsdebug=<0..127>,
                                   colliderdrag=<shape>:<resize|rotate|height|cancel|cancelheight>:<worldX>,<worldY> (shape -1 is the root),
                                   select=<name>, select+=<name>, rename=<name>, move=<dx>,<dy>,
                                   add=<Type>, remove=<Type>, save, play, stop,
                                   drop=<file>[;<file>...]@<x>,<y> (as files dropped at a point of the view, in pixels),
                                   image=<file> (as chosen for the selected sprites' image),
                                   font=<file> (as chosen for the selected texts' font),
                                   addpoint=<Type.field>, removepoint=<Type.field>@<index> (as the inspector's
                                   buttons on a list of points); repeatable
        """;

    [STAThread]
    private static int Main(string[] args)
    {
        if (!TryParse(args, out var options))
        {
            Console.Error.WriteLine(Usage);
            return 2;
        }

        try
        {
            return new EditorWindow(options).Run();
        }
        catch (Exception e) when (e is EngineException or FileNotFoundException or InvalidOperationException or DllNotFoundException)
        {
            Console.Error.WriteLine($"[error] {e.Message}");
            return 1;
        }
    }

    // A game's name, from its name or its library's path.
    public static string GameName(string game) =>
        File.Exists(game) ? Path.GetFileNameWithoutExtension(game).Replace("-module", "") : game;

    private static bool TryParse(string[] args, out EditorOptions options)
    {
        options = new EditorOptions();
        for (var i = 0; i < args.Length; i++)
        {
            if (i + 1 >= args.Length)
                return false;

            var value = args[++i];
            switch (args[i - 1])
            {
                case "--game" when options.Project == null:
                    options = options with { Game = value };
                    break;
                case "--project" when options.Game == null:
                    options = options with { Project = value };
                    break;
                case "--seed" when ulong.TryParse(value, CultureInfo.InvariantCulture, out var seed):
                    options = options with { Seed = seed };
                    break;
                case "--select":
                    options = options with { Select = value };
                    break;
                case "--set" when ParseEdit(value) is { } edit:
                    options.Edits.Add(edit);
                    break;
                case "--edit-after" when double.TryParse(value, CultureInfo.InvariantCulture, out var after):
                    options = options with { EditAfter = after };
                    break;
                case "--quit-after" when double.TryParse(value, CultureInfo.InvariantCulture, out var quit):
                    options = options with { QuitAfter = quit };
                    break;
                case "--time-scale" when double.TryParse(value, CultureInfo.InvariantCulture, out var scale) && scale > 0:
                    options = options with { TimeScale = scale };
                    break;
                case "--mode" when Enum.TryParse<ViewportMode>(value, ignoreCase: true, out var mode):
                    options = options with { Mode = mode };
                    break;
                case "--paused" when value is "on" or "off":
                    options = options with { Paused = value == "on" };
                    break;
                case "--save-after" when double.TryParse(value, CultureInfo.InvariantCulture, out var save):
                    options = options with { SaveAfter = save };
                    break;
                case "--do":
                    options.Commands.Add(value);
                    break;
                case "--recent" when value is "on" or "off":
                    options = options with { Recent = value == "on" };
                    break;
                case "--after" when int.TryParse(value, CultureInfo.InvariantCulture, out var pid):
                    options = options with { After = pid };
                    break;
                case "--screenshot":
                    options = options with { Screenshot = value };
                    break;
                default:
                    return false;
            }
        }

        return true;
    }

    private static (string, string, string)? ParseEdit(string text)
    {
        var equals = text.IndexOf('=');
        var dot = text.IndexOf('.');
        if (dot <= 0 || equals <= dot + 1)
            return null;
        return (text[..dot], text[(dot + 1)..equals], text[(equals + 1)..]);
    }
}
