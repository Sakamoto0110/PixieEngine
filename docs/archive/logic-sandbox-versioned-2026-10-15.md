# The Logic Sandbox Under Version Control

**Document ID:** ARCHIVE-LOGIC-SANDBOX-VERSIONED-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the LogicSandbox game put under version control in its own repository, at the owner's request

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-059

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** with this record (parent `f425cb6ea559d4d992593327b36d4fa839f625eb`)

**Current state:** the game's repository, [`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox) (private)

At the promotion of PIXIE-PROPOSAL-009 ([PIXIE-ARCHIVE-057](logic-sandbox-promotion-2026-10-15.md)),
Claude was not to put the game under version control; the owner would. After PIXIE-TODO-032
completed ([PIXIE-ARCHIVE-058](todo-032-logic-sandbox-stage-1-2026-10-15.md)), the owner asked
Claude to do it.

The game, `C:\Users\Sakamoto\dev\LogicSandbox`, is now its own git repository on `main`, pushed to the
private GitHub repository [`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox)
with the owner's approval. Its first commit, `069df175902d0c636482b17e8d4fc6295776467c`, holds stage 1
as PIXIE-TODO-032 delivered it, with `assets/` (the copies from the shared resources) so that a clone
builds without the store; `build/` and `bin/` stay out. Its README records the engine commit it was
last built and tested against, `f425cb6`, and its `CLAUDE.md` says how to build and test it, and that
anything the game needs from the engine is first recorded as a proposal here.

Nothing changes in this repository's boundaries: the engine still never depends on the game, and the
game's commits are not this repository's. PIXIE-TODO-033 is unchanged; its work goes to the game's
repository.
