# PixieInspector Consumed from nuget.org

**Document ID:** ARCHIVE-PIXIEINSPECTOR-NUGET-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** PixieInspector 0.1.0 published on nuget.org and recorded as a package dependency of the C# editor

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-012

**Type:** dependency

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** with this record (parent `83aad39359a52011ba3131d4e3e6692f86232565`)

**Current state:** [`DEPENDENCIES.md`](../../DEPENDENCIES.md#own-projects)

The PixieInspector session reported that PixieInspector is now distributed on
nuget.org: version 0.1.0, published on 2026-10-13 with the tag `v0.1.0` and a
GitHub release in `Sakamoto0110/PixieInspector`. It asked for the release to be
recorded here when the owner asked, and later reviewed the plan for doing so;
the owner had it sent.

Confirmed on nuget.org on 2026-10-13: `PixieInspector`, `PixieInspector.ImGui`,
`PixieInspector.Avalonia`, `PixieInspector.WinForms`, and `PixieInspector.Wpf`,
all at 0.1.0, under the MIT license. `PixieInspector.ImGui` depends on the core
at exactly `[0.1.0]` and on Hexa.NET.ImGui 2.2.9; `PixieInspector.Avalonia` on
the core at `[0.1.0]` and on Avalonia 12.1.3; the core on PixieLib 0.2.0.

Recorded in `DEPENDENCIES.md`:

- PixieInspector is consumed only as a package, never by project reference,
  copied code, or commit. The ImGui editor references `PixieInspector.ImGui`
  alone; each view requires the core at the same exact version.
- The editor's host uses the binding and version its view package brings; those
  are dependencies of the editor project, not of the engine, and a change of
  binding is PixieInspector's decision.
- The editor receives PixieLib 0.2.0 through the inspector; a direct reference
  must be 0.2.0 or newer.
- The local checkout is for developing the inspector, not how the engine
  consumes it.

Nothing in this repository consumes PixieInspector yet: the C# editor does not
exist, so the status is `not-installed`.
