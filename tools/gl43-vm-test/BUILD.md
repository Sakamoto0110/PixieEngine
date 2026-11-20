# Build the OpenGL 4.3 VM test

Use an x64 Visual Studio developer shell with CMake, Ninja, Python and the
.NET SDK pinned by the standalone Editor. Run from the Engine root and use
a fresh work directory for each build. The hook attaches the compatibility
bindings only to the VM build and includes their hashes in its native SDK
identity.

```powershell
$work = Join-Path $PWD '.local/gl43-vm-test'
$hook = (Resolve-Path tools/gl43-vm-test/Enable.cmake).Path.Replace('\', '/')
$python = (python -c 'import sys; print(sys.executable)').Trim().Replace('\', '/')
foreach ($profile in 'authoring', 'shipping') {
    $development = if ($profile -eq 'authoring') { 'ON' } else { 'OFF' }
    cmake -S . -B "$work/b-$profile" -G 'Visual Studio 18 2026' -A x64 `
        "-DCMAKE_PROJECT_PixieEngine_INCLUDE=$hook" "-DPIXIE_OUTPUT_ROOT=$work/bin-$profile" `
        "-DPIXIE_SDK_PROFILE=$profile" "-DPIXIE_DEVELOPMENT=$development" `
        -DPIXIE_DEV_CHANNEL=OFF -DPIXIE_BUILD_SAMPLES=OFF -DPIXIE_ENGINE_BUILD_TESTS=OFF `
        -DPIXIE_BUILD_TOOLS=OFF -DPIXIE_SDK_INSTALL=ON "-DPython3_EXECUTABLE=$python"
    if ($LASTEXITCODE) { throw "Could not configure $profile" }
    cmake --build "$work/b-$profile" --config Release --parallel --target pixie-player pixie_shared pixie_math glfw
    if ($LASTEXITCODE) { throw "Could not build $profile" }
    cmake --install "$work/b-$profile" --config Release --prefix "$work/u-$profile" --component PixieSDK
    if ($LASTEXITCODE) { throw "Could not install $profile" }
}
& $python -B tools/gl43-vm-test/prepare.py --work $work --out "$work/package" --editor ../PixieEditor
```

The ZIP contains a launcher and the guest instructions. The packaged Editor
is built from a temporary copy with its own settings location. The ZIP uses
the Windows guest's OpenGL driver.
