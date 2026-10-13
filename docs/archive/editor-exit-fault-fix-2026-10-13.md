# Editor Exit Fault Fixed

**Document ID:** ARCHIVE-EDITOR-EXIT-FAULT-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the editor's fault at process exit, its cause in the ImGui backends' own context, and the fix

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-024

**Type:** fix

**Origin:** PIXIE-ARCHIVE-023

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `74771b8fe32372617530824e9d126115c36f1574`

**Current state:** `editor/PixieEditor/Ui/EditorWindow.cs`

## The fault

The completion record of PIXIE-TODO-011
([PIXIE-ARCHIVE-023](todo-011-imgui-editor-2026-10-13.md)) left one issue open:
the editor faulted with `0xC0000005` while exiting in about half the runs,
after releasing everything, with or without an engine session, as
PixieInspector's own ImGui host did. At the owner's suggestion, it was reported
to the session working on PixieInspector.

## The cause

Found in PixieInspector (pull request `Sakamoto0110/PixieInspector#6`, commit
`2abd0f8`, not merged on this date), as reported back:

- The Application log names the faulting module: `ImGuiImplGLFW.dll`.
- `ImGuiImplGLFW.dll` and `ImGuiImplOpenGL3.dll` compile in the ImGui functions
  they use and keep their own copy of the current context, the one their
  `SetCurrentContext` sets. `ImGui.DestroyContext` frees the context and clears
  only cimgui's copy.
- At process exit, the GLFW backend frees a static vector through
  `ImGui::MemFree`, which reads the stale context to update its debug
  allocation log; whether the freed page is still mapped decides whether the
  process faults.
- Not the cause: the AMD driver, `opengl32.dll`, the 4.5 core context, or the
  engine's library.

## The fix

After the backends' `Shutdown` and before `ImGui.DestroyContext`,
`ImGuiImplOpenGL3.SetCurrentContext(default)` and
`ImGuiImplGLFW.SetCurrentContext(default)`.

## Verification

Same machine as PIXIE-ARCHIVE-023. The Release editor run 15 times (boids,
flocks, and particles, five times each, closing after 2 seconds with
`--quit-after`): 0 faults, against about half before. The editor builds with
zero warnings. The known issue is removed from `editor/README.md`.
