"""Qualify real exports and deliberately broken scratch copies; never edit source projects."""
import argparse
import copy
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def loaded_modules(process):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD,
                                         ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
    psapi.GetModuleFileNameExW.argtypes = [wintypes.HANDLE, wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
    handle = kernel.OpenProcess(0x410, False, process.pid)
    if not handle:
        return {}
    try:
        modules = (wintypes.HMODULE * 2048)()
        needed = wintypes.DWORD()
        if not psapi.EnumProcessModulesEx(handle, modules, ctypes.sizeof(modules), ctypes.byref(needed), 3):
            return {}
        result = {}
        for module in modules[:min(needed.value // ctypes.sizeof(wintypes.HMODULE), len(modules))]:
            text = ctypes.create_unicode_buffer(32768)
            if psapi.GetModuleFileNameExW(handle, module, text, len(text)):
                result[Path(text.value).name.lower()] = str(Path(text.value).resolve())
        return result
    finally:
        kernel.CloseHandle(handle)


def junction(link, target):
    """Create a directory junction in our scratch tree, without a shell or elevation."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p,
                                  wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.DeviceIoControl.argtypes = [wintypes.HANDLE, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD,
                                      ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    link.mkdir()
    handle = kernel.CreateFileW(str(link), 0x40000000, 7, None, 3, 0x02200000, None)
    assert handle != ctypes.c_void_p(-1).value, ctypes.get_last_error()
    try:
        substitute = ("\\??\\" + str(target.resolve())).encode("utf-16-le")
        display = str(target.resolve()).encode("utf-16-le")
        paths = substitute + b"\0\0" + display + b"\0\0"
        data = struct.pack("<IHHHHHH", 0xA0000003, 8 + len(paths), 0, 0, len(substitute),
                           len(substitute) + 2, len(display)) + paths
        buffer = ctypes.create_string_buffer(data)
        returned = wintypes.DWORD()
        assert kernel.DeviceIoControl(handle, 0x900A4, buffer, len(data), None, 0, ctypes.byref(returned), None), ctypes.get_last_error()
    finally:
        kernel.CloseHandle(handle)


def main():
    parser = argparse.ArgumentParser()
    for key in ("exporter", "engine", "work", "cmake", "generator"):
        parser.add_argument("--" + key, required=True)
    parser.add_argument("--graphics-runtime", default="")
    args = parser.parse_args()
    engine, work = Path(args.engine).resolve(), Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="run-", dir=work)).resolve()
    assert run.is_relative_to(work) and run != work
    records = []

    def record(value):
        records.append(value)
        (run / "results.json").write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding="utf-8")

    def command(name, arguments, failure=None):
        result = subprocess.run([str(x) for x in arguments], cwd=run, text=True, encoding="utf-8",
                                errors="replace", stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
        (run / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        if failure is None:
            assert result.returncode == 0, (name, result.returncode, result.stdout[-5000:])
        else:
            assert result.returncode != 0 and failure in result.stdout, (name, result.returncode, result.stdout[-5000:])
        record({"case": name, "exit": result.returncode, "expected_error": failure})
        return result

    def write_manifest(name, manifest):
        path = run / (name + ".json")
        path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
        return path

    def replace_source(manifest, destination, source):
        for item in manifest["files"]:
            if item["destination"] == destination:
                item["source"], item["sha256"] = str(source), sha(source)
                return
        raise AssertionError(destination)

    def clone(name, original):
        destination = run / name
        shutil.copytree(original, destination)
        return destination

    manifests, packages = {}, {}
    for name in ("boids", "sprites"):
        # Spaces and Unicode qualify the complete CLI -> native -> CMake argument-vector path.
        project = run / (name + " source é")
        shutil.copytree(engine / "samples" / name, project, ignore=shutil.ignore_patterns("build", "bin"))
        project_file = next(project.glob("*.pixie"))
        data = json.loads(project_file.read_text(encoding="utf-8"))
        data["future_export_fixture"] = {"preserve": [1, "unknown", {"data": True}]}
        project_file.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
        output = run / (name + " export é")
        command(name + "-export", [args.exporter, project, output])
        manifest = project / "build/pixie-export/export-manifest.json"
        manifests[name], packages[name] = manifest, output
        command(name + "-verify", [args.exporter, "--verify", manifest, output])
        assert (output / project_file.name).read_bytes() == project_file.read_bytes()
        product_files = list((project / "build/pixie-export/pixie-export/Release").glob("*.json"))
        assert len(product_files) == 1
        product = json.loads(product_files[0].read_text(encoding="utf-8"))
        assert not product["development"] and not product["dev_channel"]
        data = json.loads(manifest.read_text(encoding="utf-8"))
        assert all("-module" not in x["destination"] and "-capi" not in x["destination"] for x in data["files"])
        runnable = output
        if args.graphics_runtime:
            # The raw default export remains untouched. CI may qualify a second explicit manifest
            # with its pinned software OpenGL provider, instead of relying on a GPU driver.
            graphics = copy.deepcopy(data)
            destinations = {x["destination"].lower() for x in graphics["files"]}
            provider = Path(args.graphics_runtime).resolve()
            assert (provider / "opengl32.dll").is_file()
            for file in sorted(provider.glob("*.dll")):
                if file.name.lower() not in destinations:
                    graphics["files"].append({"source": str(file), "destination": file.name, "role": "library", "sha256": sha(file)})
                    destinations.add(file.name.lower())
            runnable = run / (name + " graphics")
            command(name + "-graphics-runtime", [args.exporter, "--from-manifest", write_manifest(name + "-graphics", graphics), runnable])
        screenshot = run / (name + ".png")
        with (run / (name + "-run.log")).open("w", encoding="utf-8") as log:
            process = subprocess.Popen([str(runnable / data["player"]), "--quit-after", "2", "--screenshot", str(screenshot)],
                                       cwd=run, stdout=log, stderr=subprocess.STDOUT)
            observed = {}
            deadline = time.monotonic() + 12
            while process.poll() is None and time.monotonic() < deadline:
                observed.update(loaded_modules(process))
                if all(x in observed for x in ("pixie.dll", "glfw3.dll", "vcruntime140.dll", "msvcp140.dll")):
                    break
                time.sleep(0.05)
            assert process.wait(timeout=30) == 0
        for dll in ("pixie.dll", "glfw3.dll", "vcruntime140.dll", "msvcp140.dll"):
            assert Path(observed[dll]).parent == runnable.resolve(), (dll, observed)
        assert screenshot.read_bytes()[:8] == b"\x89PNG\r\n\x1a\n"
        assert all(x > 0 for x in struct.unpack(">II", screenshot.read_bytes()[16:24]))
        record({"case": name + "-run", "exit": 0, "loaded_modules": observed, "capture_sha256": sha(screenshot),
                "graphics_runtime": args.graphics_runtime or "installed GPU driver"})

    base = json.loads(manifests["boids"].read_text(encoding="utf-8"))
    package = packages["boids"]
    for name in ("boids", "sprites"):
        shipping = packages[name]
        shipping_manifest = json.loads(manifests[name].read_text(encoding="utf-8"))
        assert b"--quit-after-steps" not in (shipping / "pixie.dll").read_bytes()
        rejected = command(name + "-capture-option-excluded",
                           [shipping / shipping_manifest["player"], "--quit-after-steps", "0"], "usage:")
        assert rejected.returncode == 2
    # Barrier 1: a development-enabled engine in a falsely declared shipping manifest.
    broken = copy.deepcopy(base)
    replace_source(broken, "pixie.dll", Path(args.exporter).resolve().parent / "pixie.dll")
    path = write_manifest("development-engine", broken)
    command("barrier1-development-engine", [args.exporter, "--from-manifest", path, run / "bad-engine"], "development code")
    broken["development"] = True
    command("barrier1-development-configuration", [args.exporter, "--from-manifest", write_manifest("dev-config", broken),
                                                run / "bad-config"], "development-free shipping manifest")

    # Build an actual module-loading executable and an export-forwarder pair in the scratch workspace.
    probe = run / "probe"
    probe.mkdir()
    (probe / "loader.c").write_text('''#include <windows.h>
int main(void) { HMODULE m=LoadLibraryA("bad-module.dll"); if(!m)return 0;
FARPROC p=GetProcAddress(m,"pixie_game_create"); FreeLibrary(m); return p?7:0; }
''', encoding="utf-8")
    (probe / "dependency.c").write_text('__declspec(dllexport) int Answer(void) { return 42; }\n', encoding="utf-8")
    (probe / "empty.c").write_text('#pragma comment(linker, "/export:Probe=private_dependency.Answer")\nint placeholder;\n', encoding="utf-8")
    (probe / "forwarder.def").write_text('LIBRARY forwarder\n', encoding="utf-8")
    (probe / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.25)
project(ExportProbes C)
add_executable(loader loader.c)
add_library(private_dependency SHARED dependency.c)
add_library(forwarder SHARED empty.c forwarder.def)
''', encoding="utf-8")
    configure = [args.cmake, "-S", probe, "-B", probe / "build", "-G", args.generator]
    if args.generator.startswith("Visual Studio"):
        configure += ["-A", "x64"]
    command("probe-configure", configure)
    command("probe-build", [args.cmake, "--build", probe / "build", "--config", "Release", "--parallel", "4"])
    binaries = probe / "build/Release"
    broken = copy.deepcopy(base)
    replace_source(broken, broken["player"], binaries / "loader.exe")
    command("barrier2-player-module-loader", [args.exporter, "--from-manifest", write_manifest("module-loader", broken),
                                             run / "bad-loader"], "development code")

    contaminated = clone("unlisted-file", package)
    (contaminated / "unlisted.txt").write_text("not in the game manifest", encoding="utf-8")
    command("barrier3-unlisted-file", [args.exporter, "--verify", manifests["boids"], contaminated], "unlisted file")
    contaminated = clone("unlisted-directory", package)
    (contaminated / "tools").mkdir()
    command("barrier3-unlisted-directory", [args.exporter, "--verify", manifests["boids"], contaminated], "unlisted directory")
    for dll in ("glfw3.dll", "vcruntime140.dll"):
        missing = clone("missing-" + dll, package)
        (missing / dll).unlink()
        broken = copy.deepcopy(base)
        broken["files"] = [x for x in broken["files"] if x["destination"] != dll]
        command("barrier4-closure-" + dll, [args.exporter, "--verify", write_manifest("missing-" + dll, broken), missing],
                "unresolved native import")
    broken = copy.deepcopy(base)
    forwarder = binaries / "forwarder.dll"
    broken["files"].append({"source": str(forwarder), "destination": "forwarder.dll", "role": "library", "sha256": sha(forwarder)})
    command("forwarded-dependency-missing", [args.exporter, "--from-manifest", write_manifest("forwarder", broken),
                                           run / "bad-forwarder"], "unresolved native import: private_dependency.dll")
    dependency = binaries / "private_dependency.dll"
    broken["files"].append({"source": str(dependency), "destination": "private_dependency.dll", "role": "library", "sha256": sha(dependency)})
    command("forwarded-dependency-complete", [args.exporter, "--from-manifest", write_manifest("forwarder-complete", broken),
                                            run / "good-forwarder"])

    # A path-shaped import must not be treated as Windows or joined to an asset/dependency directory.
    malformed = run / "path-import.exe"
    image = bytearray((package / base["player"]).read_bytes())
    marker = image.lower().find(b"kernel32.dll\0")
    assert marker >= 0
    image[marker:marker + 13] = b"../evil.dll\0\0"
    malformed.write_bytes(image)
    broken = copy.deepcopy(base)
    replace_source(broken, broken["player"], malformed)
    command("import-basename-traversal", [args.exporter, "--from-manifest", write_manifest("path-import", broken),
                                         run / "bad-import"], "library basename")
    broken = copy.deepcopy(base)
    broken["files"][0]["destination"] = "../escape.exe"
    command("manifest-path-traversal", [args.exporter, "--from-manifest", write_manifest("path", broken),
                                       run / "bad-path"], "portable relative path")
    command("existing-output-preserved", [args.exporter, "--from-manifest", manifests["boids"], package], "empty or absent")
    target = run / "junction-target"
    target.mkdir()
    sentinel = target / "preserve.txt"
    sentinel.write_text("preserve", encoding="utf-8")
    link = run / "junction"
    junction(link, target)
    project = manifests["boids"].parents[2]
    command("original-output-junction", [args.exporter, project, link / "out"], "reparse")
    assert sentinel.read_text(encoding="utf-8") == "preserve" and not (target / "out").exists()
    command("restored-clean-boids", [args.exporter, "--verify", manifests["boids"], package])
    command("restored-clean-sprites", [args.exporter, "--verify", manifests["sprites"], packages["sprites"]])
    (run / "results.json").write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"PASS: {len(records)} export cases. Evidence: {run}")


if __name__ == "__main__":
    main()
