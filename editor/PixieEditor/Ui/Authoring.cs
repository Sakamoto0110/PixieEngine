using PixieEditor.Native;

namespace PixieEditor.Ui;

// Making scene entities by hand, from the entity list or the viewport's menu.
internal static class Authoring
{
    // A shape's default size is one unit: too small to see or click.
    private const double ShapeSize = 24;

    // A scene entity named "entity N", with the first N no entity has, with a transform at `at` (world
    // units; the origin when null) and a circle, chosen as the selection. Null when the engine
    // refused, after saying why.
    public static uint? CreateEntity(EngineSession session, Selection selection, (double X, double Y)? at = null)
    {
        try
        {
            var names = session.Entities().Select(session.EntityName).ToHashSet();
            var number = Enumerable.Range(1, names.Count + 1).First(n => !names.Contains($"entity {n}"));
            var entity = session.CreateEntity($"entity {number}");
            if (session.TypeIndex("Transform2D") is var transform and >= 0)
            {
                session.AddComponent(entity, transform);
                if (at is { } point)
                    session.Set(entity, transform, session.FieldIndex(transform, "position"), new PixieValue { X = point.X, Y = point.Y });
            }

            if (session.TypeIndex("Shape") is var shape and >= 0)
            {
                session.AddComponent(entity, shape);
                session.Set(entity, shape, session.FieldIndex(shape, "size"), new PixieValue { X = ShapeSize, Y = ShapeSize });
            }

            selection.Set(entity);
            return entity;
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
            return null;
        }
    }
}
