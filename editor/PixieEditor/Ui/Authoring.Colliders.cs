using PixieEditor.Editing;

namespace PixieEditor.Ui;

internal sealed record ColliderGesture(uint Entity, ulong SceneId, int Shape, int Kind, string Before);

internal sealed partial class Authoring
{
    public ColliderGesture? BeginColliderDrag(uint entity, int shape, int kind) => Guard(() =>
    {
        if (!Recording || session.SceneId(entity) is not { } id || session.ColliderHandles2D(entity, shape) == null)
            return null;
        return new ColliderGesture(entity, id, shape, kind, session.Snapshot([entity]));
    });

    public void UpdateColliderDrag(ColliderGesture gesture, double x, double y, bool snap) => Guard(() =>
    {
        if (Recording && session.IsAlive(gesture.Entity) && session.SceneId(gesture.Entity) == gesture.SceneId)
            session.DragCollider2D(gesture.Entity, gesture.Shape, gesture.Kind, x, y, snap);
        return 0;
    });

    public void EndColliderDrag(ColliderGesture gesture, bool cancel = false) => Guard(() =>
    {
        if (!Recording || !session.IsAlive(gesture.Entity) || session.SceneId(gesture.Entity) != gesture.SceneId)
            return 0;
        var after = session.Snapshot([gesture.Entity]);
        if (after == gesture.Before) return 0;
        if (cancel)
        {
            session.Destroy(gesture.Entity);
            selection.Set(session.Restore(gesture.Before, newIds: false));
        }
        else history.Record(new ReplaceStep(gesture.Kind == 0 ? "collider resize" : gesture.Kind == 1 ? "collider rotation" : "capsule height",
            [gesture.SceneId], gesture.Before, after));
        return 0;
    });
}
