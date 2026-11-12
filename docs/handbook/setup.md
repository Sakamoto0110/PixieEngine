# Setup and compatibility

**Document ID:** GLOBAL-CONSUMER-SETUP
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** installed SDK setup for game consumers
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

[Handbook](README.md)

## Obtain the inputs

C++ authoring requires the public SDK. It supplies headers, import libraries,
CMake support, authoring profiles and, when declared, paired export tooling.
The Runtime-only ZIP supplies the `shipping/Release` generic player and DLLs
to run an existing saved no-code project. Hand players the complete exported
game folder when distributing your game.

You need the complete Editor application directory and a compatible **public
SDK** directory. Before the first public release, obtain those artifacts and
their compatibility information from the maintainer. A public release URL and
an automatic SDK download flow are not supplied by this guide. Do not download
an arbitrary `pixie.dll` and assume it is the SDK.

If supplied as ZIPs, extract each **whole archive** into a separate directory.
Keep the DLLs and other files alongside the application where the archive puts
them. An additional outer ZIP folder is harmless: what matters is locating the
actual executable and actual SDK root.

For example, you can choose this layout; these are example locations, not paths
that must already exist:

```text
C:/PixieTools/Editor/       PixieEditor.exe and all accompanying files
C:/PixieTools/SDK/          sdk.json, include/, profiles/, cmake/, lib/, tools/ ...
C:/Games/MyFirstGame/       your .pixie file, scenes/, images/, fonts/, src/ ...
C:/Games/MyFirstGame-out/   a future export; initially absent or empty
```

Do not create projects, caches or personal notes inside the SDK. Its inventory
is checked for changed, missing **and extra** files. Keep the supplied SDK
immutable and store different SDKs in different directories.

## Prerequisites by activity

| Activity | Inputs |
|---|---|
| Run the Editor and edit a project without C++ | Complete Editor application, compatible public SDK, Windows x64, graphics driver exposing OpenGL 4.5 core; .NET Runtime 10 x64 for the current framework-dependent application |
| Add/build game C++ code | Above, plus the matching MSVC x64 compiler and Windows SDK, and CMake available through PATH or Visual Studio |
| Export a project without code | Public SDK with `features.pairedExport` and its paired exporter/shipping payload; no game compilation is needed |
| Export a project with code | Same paired SDK plus the matching C++ build toolchain |
| Build the Editor itself | Separate maintainer workflow using .NET SDK and verified binding inputs; unnecessary when you already have the application |
| Run the exported game | Complete export directory and its declared OS/graphics requirements; no Editor or C++ compiler required |

The SDK also records an AVX2/FMA/F16C CPU baseline. A VM must expose the required
CPU features and graphics context; installing Windows alone does not establish
either. Do not switch to the experimental GL 4.3 path to satisfy this handbook.
The current Editor's native backend also imports `vulkan-1.dll` despite using
OpenGL. Before beginning a clean-machine walkthrough, launch the complete
Editor application and confirm its start screen appears. If startup reports
that DLL missing, the native dependency set is incomplete; obtain the supported
64-bit loader prerequisite/installer from the application maintainer. Do not
copy an arbitrary DLL from a download site or install a different renderer.

For diagnosis, `Test-Path "$env:WINDIR/System32/vulkan-1.dll"` in PowerShell
checks the usual system location. A packaged application may supply a local
copy instead; file presence alone does not prove architecture or successful
loading. The successful Editor launch is the consumer smoke check. The
application's `eng/native/win-x64/release-provenance.json` records this import;
the [Editor development guide](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/development.md#rehearse-the-windows-x64-release) describes the packaging boundary. A maintainer must
supply prerequisites with a clean-machine handoff; this guide does not claim
the SDK installs them. The local handbook session used an already runnable
application and did not test installing prerequisites on a clean machine.

For a new C++ machine, install Visual Studio's **Desktop development with C++**
workload, including the toolset and Windows SDK named by your SDK profile and
the C++ CMake tools. A similarly named Visual Studio release alone is not
enough: this package initially requires exact compiler compatibility. Inspect
`profiles/authoring/Release/profile.json` for `compilerVersion` and
`windowsSdkVersion`. If that tuple is unavailable, ask for a compatible SDK;
do not bypass its checks.

## Select the SDK in the Editor

1. Launch `PixieEditor.exe` from the complete application directory.
2. On its start screen choose **Choose SDK...**.
3. In the folder browser, navigate into the directory containing **`sdk.json`**.
   You can type the full path into the browser path field and press Enter.
4. Choose **Release** under **Native configuration** for this walkthrough.
   Normal public authoring selects the `authoring` profile.
5. Choose **Use SDK**. The start screen should show the selected SDK summary
   instead of a request to choose one. An error here must be resolved before
   creating the game.

Select the SDK root, not `profiles/authoring/Release/bin`, not the `include`
directory, not the Engine source directory, and not the project directory.
Selecting a folder reads and checks its contents; it does not install/download
an SDK. The Editor remembers the chosen local path in its user settings.

### When a path works in one application but not another

An SDK folder may be visible to one tool and redirected or inaccessible to the
Editor process. Confirm that the **Editor itself** can select the SDK and show
its identity summary. For an assistant-led handoff, also record the verified
physical path in the [handoff inputs](assistant-handoff.md#handoff-inputs).

If the remembered path fails, obtain the installation's accessible physical
path and select it through **Choose SDK**. Match its `sdk.json` SHA-256/SDK ID
to the supplied artifact identity, then let the Editor validate the package.
Do not select another copy merely because its folder name or version looks
similar. Keep the SDK contents unchanged. The handoff's `LOCAL-SETUP.md` should
record the verified path for that machine; application-private cache paths are
not portable installation defaults or a requirement to install that assistant.

New projects record portable SDK requirements in `.pixie-code/sdk.json`.
That file is a requirement, not a second SDK installation. The physical SDK
path stays local to the machine. The **Record this SDK when opening a legacy
project** option is explicit adoption for a project without a recorded
requirement; it is not a repair button for incompatible existing requirements.

## Profiles and configurations are different choices

| Native profile | Use | Native configurations |
|---|---|---|
| `authoring` | Editor sessions and your development game module | Debug and Release when supplied |
| `shipping` | Standalone exported games; game code linked into the player | Release |
| `local-development` | Separate private automation/qualification distribution | Only its declared configurations |

Release authoring still supports game modules. It does **not** become shipping
merely because its configuration is Release. An exported game uses the paired
shipping profile, not your authoring DLL renamed or copied into the output.

The package's SDK ID, selected profile/build identity and native ABI matter in
addition to its human-readable version. Rebuild game code against the selected
SDK. Never mix headers/import libraries from one installation with a DLL from
another. Use a fresh build directory when changing SDK/profile/configuration.
The Editor manages its own identity-specific code caches.

Continue with the [Editor walkthrough](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/handbook.md#1-choose-the-sdk-and-create-a-project)
to create and save a project, add C++ code, Play/Stop and export.

## Optional explicit launch

The UI is sufficient for normal use. For a repeatable launch from PowerShell,
replace these example paths with your real locations:

```powershell
$editor = 'C:\PixieTools\Editor\PixieEditor.exe'
$sdk = 'C:\PixieTools\SDK'
$game = 'C:\Games\MyFirstGame'
& $editor --sdk $sdk --native-configuration Release --project $game
```

`--sdk` overrides the remembered selection and locks that choice for the launch.
The `PIXIE_SDK_ROOT` environment variable is another optional override. Clear
an obsolete override if you intend to choose through the UI. Do not put these
machine paths in the game's `.pixie` document or portable requirement.

## CMake consumption without rebuilding the Engine

The generated game already includes the SDK's `cmake/PixieProject.cmake` and
uses `pixie_game`. Keep that route for an Editor-managed game. Its targets are
`<Id>-game` (game code), `<Id>-module` (authoring DLL), and `<Id>` (player).
`<Id>` comes from `.pixie-code/manifest.json`, not necessarily the raw folder name.

For a separate native utility that does not use the Editor's project registry,
the installed package also supports this minimal CMake file:

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyUtility LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
find_package(PixieEngine CONFIG REQUIRED
    PATHS "${PIXIE_SDK_ROOT}/lib/cmake/PixieEngine" NO_DEFAULT_PATH)
add_executable(MyUtility main.cpp)
target_link_libraries(MyUtility PRIVATE Pixie::Runtime)
target_compile_options(MyUtility PRIVATE /arch:AVX2)
pixie_stage_runtime(MyUtility)
```

`main.cpp` can begin with a headless smoke, without a window or GL context:

```cpp
#include "pixie/runtime/World.hpp"
#include <iostream>

int main() {
    pixie::World world(1);
    const auto entity = world.Create("hello");
    std::cout << (entity.IsAlive() ? "Pixie is ready\n" : "Entity creation failed\n");
    return entity.IsAlive() ? 0 : 1;
}
```

From a terminal with CMake available, select the installed matching Visual
Studio generator (the inspected installation uses the name below):

```powershell
cmake -S . -B build-authoring -G "Visual Studio 18 2026" -A x64 `
  -DPIXIE_SDK_ROOT=C:/PixieTools/SDK -DPIXIE_SDK_PROFILE=authoring `
  -DCMAKE_CONFIGURATION_TYPES=Release '-DCMAKE_SYSTEM_VERSION=10.0.26100.0'
cmake --build build-authoring --config Release
& .\build-authoring\Release\MyUtility.exe
```

The Windows SDK version above is the inspected baseline, not a replacement
for reading your profile. Expected output is `Pixie is ready`. CMake stages the
declared runtime dependencies beside the executable. Keep that directory
together. This utility is not an Editor-loadable game module or a shipping export.
