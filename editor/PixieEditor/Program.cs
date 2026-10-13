using System.Globalization;
using PixieEditor.Native;
using PixieEditor.Ui;

namespace PixieEditor;

// What the command line asks for.
internal sealed record EditorOptions
{
    public string Game { get; init; } = "boids";
    public ulong Seed { get; init; } = 1;
    // The entity, by name, selected at the start.
    public string? Select { get; init; }
    // Edits of the selected entity, applied through its inspectors after EditAfter seconds.
    public List<(string Type, string Property, string Value)> Edits { get; } = [];
    public double EditAfter { get; init; } = 1.0;
    public double QuitAfter { get; init; }
    public string? Screenshot { get; init; }
}

internal static class Program
{
    private const string Usage = """
        usage: PixieEditor [options]
          --game <name|path>       the game: boids, flocks, particles, or a path to its capi library (default boids)
          --seed <n>               seed of the World (default 1)
          --select <name>          select the entity with this name at the start
          --set <Type.Property=v>  after --edit-after seconds, set a property of the selected entity through its inspector; repeatable
          --edit-after <seconds>   when the --set edits happen (default 1)
          --quit-after <seconds>   close after this long; 0 means never (default 0)
          --screenshot <file.png>  with --quit-after, save the last frame of the window
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
            GameLibrary.Use(GameLibrary.Find(options.Game));
            return new EditorWindow(options with { Game = GameName(options.Game) }).Run();
        }
        catch (Exception e) when (e is EngineException or FileNotFoundException or InvalidOperationException or DllNotFoundException)
        {
            Console.Error.WriteLine($"[error] {e.Message}");
            return 1;
        }
    }

    private static string GameName(string game) =>
        File.Exists(game) ? Path.GetFileNameWithoutExtension(game).Replace("-capi", "") : game;

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
                case "--game":
                    options = options with { Game = value };
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
