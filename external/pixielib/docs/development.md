# Development and Validation Guide

**Document ID:** GLOBAL-DEVELOPMENT-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** building, checking, and releasing a change to PixieLib

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

This guide collects the practices needed to change PixieLib safely. It does not
replace the sources that own technical truth: source code owns behavior, the
build and project files own targets and options, the
[layout contract](layout.md) and the module `REFERENCE.md` files own the
documented contracts, and the
[documentation policy](governance/documentation-policy.md) owns the commit and
release rules.

## Working agreements

- Change the design only after the owner confirms it. When the owner asks for
  a change, apply it, verify it, and push it.
- Simplify, but keep one concept per file; do not merge files to lower the
  count.
- Redesign one thing at a time, and fix the cause rather than a symptom.
- Names in a request are examples, not specifications.
- Ask numbered questions, so the owner can answer by number ("8.1: yes"). When
  the owner asks for an opinion, give a recommendation, not a list of options,
  and its cost. Questions live in `.local/decisions/open-questions.md`, or in
  the conversation when `.local/` does not survive the session; numbers are
  never reused.
- A change to one side is a change to both: what a type does in C++ it does in
  C#, under the same name, unless the [layout contract](layout.md) records the
  difference.
- Keep one assembly: the C# package has no dependency, and a new one needs a
  stated reason.
- Before calling a change done, check that the tests catch the error: undo the
  change, or break a copy on purpose, and watch the new tests fail.

## C++

C++20 and CMake 3.20 or newer. From the repository root:

```bash
cmake -S cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
build/tests/pixie_tests
```

The tests print `all checks passed` and return 0, or list each failed check.
GLM is downloaded by `FetchContent`; `-DFETCHCONTENT_SOURCE_DIR_PIXIE_GLM=<dir>`
points it at a local checkout of GLM 1.0.3 instead. The variants CI runs, and
that are worth running before a push:

- `-DCMAKE_CXX_COMPILER=clang++`: Clang fuses multiply and add unless told not
  to; the tests pass `-ffp-contract=off`, and the bits compared with C# depend
  on it.
- `-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all"`:
  the integer arithmetic must never be undefined behavior.
- `-DPIXIE_MATH=OFF`: the primitives and the events build and pass without
  GLM.

MSVC runs only in CI.

## C#

The .NET 10 SDK builds every target on any system:

```bash
dotnet build dotnet/PixieLib.slnx -c Release
dotnet run --project dotnet/PixieLib.Tests -c Release -f net10.0 --no-build
```

A change is not done until the solution builds with 0 warnings and 0 errors and
the tests print `all checks passed`. The `net481` target is compiled here but
runs only on Windows, in CI. Without the SDK, the official script installs it
into the user profile:

```bash
curl -sSL -o dotnet-install.sh https://dot.net/v1/dotnet-install.sh
bash dotnet-install.sh --channel 10.0 --install-dir "$HOME/.dotnet"
export PATH="$HOME/.dotnet:$PATH" DOTNET_ROOT="$HOME/.dotnet"
```

The types that come in three precisions are written once, in
`dotnet/PixieLib/Templates/*.template.cs`, and the generator copies them; see
the [generator module](modules/generator/README.md).

## The same bits on both sides

The tests of each side use the same values, so a value that changes on one side
fails there. For a change to a math operation, also compare the two sides on
many inputs: a small C++ program and a small C# program read the same random
inputs from a file and print the bits of each result, and the outputs must be
identical (`cmp`). Compile the C++ side with `-ffp-contract=off`, with both g++
and clang++. Such programs live outside the repository; record the number of
cases and what was compared in the documentation commit.

## Continuous integration

`.github/workflows/ci.yml` runs on every push to `main` and on pull requests:
the C++ on g++ with the address and undefined-behavior sanitizers, on clang++,
and on MSVC; the C# on .NET 10 on Linux and Windows, where it also runs on the
.NET Framework (`net481`), and packs. After a push, check the run: a red `main`
comes before anything else, and what only MSVC or the .NET Framework shows
appears only there. When a check fails only on the .NET Framework, the text and
matrix checks print the text they got and the bits of each number.

## Releases

A new version starts by raising the `Version` of
`dotnet/PixieLib/PixieLib.csproj`. The owner publishes through
`.github/workflows/publish.yml` (Actions, Publish, Run workflow) on `main`: it
runs CI, packs, publishes to nuget.org through trusted publishing, and creates
the tag `v<Version>` and its release. A version that exists on nuget.org cannot
be replaced, only unlisted. Before the run, the `GIT_TAG` of the README's
`FetchContent` example moves to the tag the run will create, because the README
goes into the package.
