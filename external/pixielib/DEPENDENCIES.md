# PixieLib Dependencies

**Document ID:** GLOBAL-DEPENDENCIES

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** every dependency, its version, status, and candidates

**Surface:** dependency-registry

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

**Last checked:** 2026-10-12

This is the master list of what PixieLib depends on: toolchain, libraries, and
packages. It owns which dependency is used, why, and its current status. The
build and project files own the wiring: which target or project uses which
dependency, at which version.

## Status

| Status | Meaning |
|---|---|
| `ok` | Present where it is consumed, at a current or deliberately pinned version |
| `outdated` | Present, but a newer release exists and the gap matters |
| `not-installed` | Not present yet |

"Latest" is the newest stable release on the `Last checked` date: on nuget.org
for packages, and the newest tag for GLM. Statuses are observations from that
date, not guarantees. Recheck before a version change.

## Toolchain

| Dependency | Used for | In use | Latest | Status | Notes |
|---|---|---|---|---|---|
| C++20 compiler | The C++ side | g++ 13, clang++ 18, and MSVC in CI | — | `ok` | The tests compile with `-Wall -Wextra -Wpedantic -Wconversion -Werror -ffp-contract=off` on GCC and Clang, and `/W4 /WX /permissive-` on MSVC. |
| CMake | Building the C++ side, and `FetchContent` for GLM and for consumers | 3.20 or newer (`cmake_minimum_required`) | — | `ok` | |
| .NET SDK 10 | Building the C# side, its tests, and the package; C# `latest` | 10.0.x (10.0.401 on 2026-10-12) | — | `ok` | Builds the `net481` target on any system through the reference assemblies the SDK brings; `net481` runs only on Windows, in CI. |
| .NET Framework 4.8.1 | The `net481` target, for NekoLib | Reference assemblies at build time; the runtime on Windows | — | `ok` | `System.Drawing` is referenced explicitly there, since on the .NET Framework it is a separate assembly. |

## Libraries and packages

| Dependency | Used for | In use | Latest | Status | Notes |
|---|---|---|---|---|---|
| GLM | The vectors and matrices of `pixie::math` | 1.0.3, pinned by commit in `cpp/CMakeLists.txt` (`8d1fd52e5ab5590e2c81768ace50c72bae28f2ed`), headers only | 1.0.3 | `ok` | Adopted 2026-10-09 (6.1). `PIXIE_SYSTEM_GLM` takes it from `find_package` instead, untested; `PIXIE_MATH=OFF` leaves it out. |
| Microsoft.CodeAnalysis.CSharp | The source generator, `dotnet/PixieLib.Generators` | 4.8.0, private, build time only | 5.9.0 | `ok` | Not shipped: it runs inside the compiler while PixieLib builds. A newer version would raise the oldest compiler that can run the generator; no decision about it is recorded. |
| System.Memory | The layout checks of the tests on `net481` (`Unsafe`, `MemoryMarshal`) | 4.6.3, in `dotnet/PixieLib.Tests` only | 4.6.3 | `ok` | PixieLib itself depends on nothing. |

## Consumers

Not dependencies, listed so that a breaking change is weighed against them:
PixieEngine (C++), PixieInspector (C#, from nuget.org, at an exact version), and
NekoLib (C#, also on the .NET Framework).

## Candidates

None.
