"""Qualify real exports and deliberately broken scratch copies; never edit source projects."""
import argparse
import copy
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time

# Actual selected shipping runtime provenance, populated from its SDK profile.
PRODUCT_VERSION = None


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


def pe_imports(path):
    """The DLL names a PE file imports, directly or delay-loaded, as the exporter's closure reads them."""
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    sections, optional = struct.unpack_from("<H12xH", data, pe + 6)
    magic = struct.unpack_from("<H", data, pe + 24)[0]
    directories = pe + 24 + (112 if magic == 0x20B else 96)
    table = pe + 24 + optional

    def offset(rva):
        for i in range(sections):
            size, address, _, raw = struct.unpack_from("<IIII", data, table + 40 * i + 8)
            if address <= rva < address + size:
                return rva - address + raw
        raise ValueError(f"{path.name}: RVA {rva:#x} is in no section")

    def name(rva):
        start = offset(rva)
        return data[start:data.index(b"\0", start)].decode("ascii")

    names = []
    for index, size, field in ((1, 20, 12), (13, 32, 4)):
        rva = struct.unpack_from("<I", data, directories + 8 * index)[0]
        while rva:
            entry = offset(rva)
            target = struct.unpack_from("<I", data, entry + field)[0]
            if not target:
                break
            names.append(name(target))
            rva += size
    return names


def opengl_closure(provider):
    """The provider's opengl32.dll and the DLLs it needs from the same folder, not every DLL there."""
    available = {file.name.lower(): file for file in provider.glob("*.dll")}
    needed, pending = {}, ["opengl32.dll"]
    while pending:
        key = pending.pop().lower()
        if key in needed or key not in available:
            continue
        needed[key] = available[key]
        pending.extend(pe_imports(available[key]))
    return sorted(needed.values())


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


def producer_refusals():
    """CLI/schema-entry checks only; never claim actual paired SDK/export/G3 evidence."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--producer-refusals", action="store_true")
    parser.add_argument("--exporter", required=True)
    parser.add_argument("--work", required=True)
    args = parser.parse_args()
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="producer-", dir=work))
    empty = run / "not-an-sdk"
    empty.mkdir()
    (empty / "sdk.json").write_text("{}", encoding="utf-8")
    records = []
    cases = [
        ("missing-sdk", ["project", "output"], 2),
        ("identity-requires-sdk", ["--identity"], 2),
        ("duplicate-sdk", ["--sdk", str(empty), "--sdk", str(empty), "--identity"], 2),
        ("conflicting-operations", ["--sdk", str(empty), "--identity", "--verify"], 2),
        ("unknown-option", ["--sdk", str(empty), "--unknown"], 2),
        ("verify-work-forbidden", ["--sdk", str(empty), "--work", str(run / "cache"), "--verify", "manifest", "out"], 2),
        ("absent-selected-root", ["--sdk", str(run / "absent"), "--identity"], 1),
        ("invalid-selected-metadata", ["--sdk", str(empty), "--identity"], 1),
    ]
    for name, arguments, expected in cases:
        environment = dict(os.environ)
        environment["PIXIE_SDK_ROOT"] = str(empty)  # Tool never uses a second ambient SDK selector.
        command = [str(Path(args.exporter).resolve()), *arguments]
        result = subprocess.run(command, cwd=run, env=environment, text=True, encoding="utf-8", errors="replace",
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        (run / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        assert result.returncode == expected, (name, result.returncode, result.stdout)
        records.append({"case": name, "command": command, "exit": result.returncode})
    (run / "results.json").write_text(json.dumps({"scope": "producer CLI/refusal only; no G3", "cases": records}, indent=2), encoding="utf-8")
    print(f"PASS: {len(records)} producer CLI refusals. Evidence: {run}")


def paired_sdk():
    """Actual paired-tool/no-code/replay and binary sensitivity checks; code export is separate."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--paired-sdk", action="store_true")
    parser.add_argument("--default-project-lengths", nargs="+", type=int, default=[])
    for key in ("sdk", "work", "cmake", "generator"):
        parser.add_argument("--" + key, required=True)
    args = parser.parse_args()
    sdk = Path(args.sdk).resolve()
    exporter = sdk / "tools/export/Release/pixie-export.exe"
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="paired-", dir=work))
    records = []

    def command(name, arguments, failure=None, compiler=False):
        argv = [str(x) for x in arguments]
        environment = dict(os.environ)
        environment.pop("PIXIE_SDK_ROOT", None)
        if not compiler:
            environment["PATH"] = ""
            argv = [str(exporter), "--sdk", str(sdk), *argv]
        result = subprocess.run(argv, cwd=run, env=environment, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, encoding="utf-8", errors="replace", timeout=300)
        (run / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        records.append(dict(case=name, command=argv, exit=result.returncode, expected_error=failure,
                            path="inherited for probe compiler" if compiler else "empty"))
        (run / "results.json").write_text(json.dumps(dict(scope="bounded actual paired SDK; not full G3", cases=records),
                                                    indent=2, ensure_ascii=False), encoding="utf-8")
        assert (result.returncode == 0 if failure is None else result.returncode == 1 and failure in result.stdout), (name, result.stdout)
        return result.stdout

    def write(name, value):
        path = run / (name + ".json")
        path.write_text(json.dumps(value, indent=2, ensure_ascii=False), encoding="utf-8")
        return path

    identity = json.loads(command("actual-identity", ["--identity"]))
    profile = json.loads((sdk / "profiles/shipping/Release/profile.json").read_text(encoding="utf-8"))
    assert identity["sdkId"] == profile["sdkId"] and identity["sdkBuildId"] == profile["sdkBuildId"]
    project = run / "No code project Å"
    (project / "scenes").mkdir(parents=True)
    (project / "assets").mkdir()
    # Asset closure is tested as exact bytes; these fixtures are never rendered or decoded as media.
    (project / "assets/sprite.png").write_bytes(b"opaque sprite asset fixture")
    (project / "assets/font.ttf").write_bytes(b"opaque font asset fixture")
    (project / "Test.pixie").write_text(json.dumps(dict(format="pixie-project", version=1, name="Export Å", startScene="scenes/main.scene")))
    (project / "scenes/main.scene").write_text('{"format":"pixie-scene","version":1,"seed":123,"entities":[]}\n')
    authored = dict(format="pixie-scene", version=2, seed=987,
        presentation=dict(schemaVersion=1, lightingState="disabled", camera="11", futureRoot=17,
                          environment=dict(ambientIntensity=0.3, futureEnvironment=dict(keep=True)),
                          view=dict(exposure=2, futureView=13)),
        sceneScopes=[dict(id="7", name="room", camera="11", futureScope=23,
                          environment=dict(mode="inherit", value=dict(ambientIntensity=0.1, futureValue=43))),
                     dict(id="8", name="empty")],
        entities=[dict(id="11", sceneScope="7", components=dict(Transform2D=dict(position=[20, 30]), Camera2D=dict(zoom=2),
                                                              Sprite=dict(image="assets/sprite.png"), Text=dict(font="assets/font.ttf"),
                                                              FutureOpaque=dict(keep="unchanged")))],
        futureDocument=dict(keep="retained"))
    (project / "scenes/complete-v2.scene").write_text(json.dumps(authored, indent=2) + "\n", encoding="utf-8")
    before = {p.relative_to(project).as_posix(): sha(p) for p in project.rglob("*") if p.is_file()}
    cache, package = run / "cache Å", run / "Game Å"
    command("no-code-no-compiler", ["--work", cache, project, package])
    manifest = cache / "export-manifest.json"
    base = json.loads(manifest.read_text(encoding="utf-8"))
    assert base["version"] == 2 and base["engine_version"] == profile["productVersion"]
    for relative, digest in before.items():
        assert sha(project / relative) == digest and sha(package / relative) == digest
    assert not (project / ".pixie-code").exists()
    default_package = run / "Default cache game Å"
    command("default-work-relocated-project", [project, default_package])
    default_plans = list((project / ".local/pixie-export").rglob("export-manifest.json"))
    assert len(default_plans) == 1 and sha(default_package / "scenes/complete-v2.scene") == before["scenes/complete-v2.scene"]
    for length in args.default_project_lengths:
        padding = length - len(str(run)) - len("\\P Å")
        assert padding >= 0, f"Use a shorter fixture work root to test an exact {length}-character path"
        long_project = run / ("P" + "x" * padding + " Å")
        assert len(str(long_project)) == length
        shutil.copytree(project, long_project, ignore=shutil.ignore_patterns(".local"))
        relocated = run / ("default-game-" + str(length))
        logs = command(f"default-work-{length}", [long_project, relocated])
        plan_path = Path(next(line.removeprefix("Export manifest: ") for line in logs.splitlines() if line.startswith("Export manifest: ")))
        assert plan_path.is_relative_to(long_project) and plan_path.parent.name == sha(sdk / "sdk.json")
        # Python needs explicit extended I/O too when the default cache exceeds MAX_PATH.
        plan_io = Path("\\\\?\\" + str(plan_path)) if len(str(plan_path)) >= 240 else plan_path
        assert json.loads(plan_io.read_text(encoding="utf-8"))["sdkBuildId"] == profile["sdkBuildId"]
        assert sha(relocated / "scenes/complete-v2.scene") == before["scenes/complete-v2.scene"]
        command(f"verify-default-work-{length}", ["--verify", plan_path, relocated])
    command("verify-original", ["--verify", manifest, package])
    dead_sources = copy.deepcopy(base)
    for item in dead_sources["files"]:
        item["source"] = str(run / "absent sources" / item["destination"])
    command("verify-does-not-open-sources", ["--verify", write("dead-sources", dead_sources), package])
    command("replay-original", ["--from-manifest", manifest, run / "Replay Å"])
    legacy = copy.deepcopy(base)
    legacy["version"] = 1
    command("legacy-manifest-refused", ["--from-manifest", write("legacy", legacy), run / "bad-legacy"], "fresh export")
    for key in ("sdkId", "sdkBuildId"):
        broken = copy.deepcopy(base)
        broken[key] = "0" * 64
        command("mixed-" + key, ["--from-manifest", write("mixed-" + key, broken), run / ("bad-" + key)], "mixed selected")
    for name in ("glfw3.dll", "msvcp140_atomic_wait.dll", "vcruntime140.dll"):
        broken = copy.deepcopy(base)
        broken["files"] = [x for x in broken["files"] if x["destination"].lower() != name]
        missing = run / ("missing-" + name)
        shutil.copytree(package, missing)
        (missing / name).unlink()  # Deliberate removal in this newly generated negative fixture only.
        command("omitted-" + name, ["--verify", write("omitted-" + name, broken), missing], "omits mandatory")
    changed = copy.deepcopy(base)
    changed["exporter"]["sha256"] = "0" * 64
    command("mixed-tool-provenance", ["--from-manifest", write("mixed-tool", changed), run / "bad-tool"], "mixed exporter")
    command("existing-output-preserved", ["--from-manifest", manifest, package], "empty or absent")
    probe = run / "probe"
    probe.mkdir()
    markers = ["pixie_game_api_version", "pixie_game_create", "pixie_game_candidate_contract", "pixie_game_sdk_identity"]
    cmake = ['cmake_minimum_required(VERSION 3.25)', 'project(ExportMarkerProbes C)']
    for index, marker in enumerate(markers):
        (probe / f"export{index}.c").write_text(f'__declspec(dllexport) int {marker}(void) {{ return 1; }}\n')
        (probe / f"import{index}.c").write_text(f'__declspec(dllimport) int {marker}(void);\n__declspec(dllexport) int Probe(void) {{ return {marker}(); }}\n')
        cmake.extend([f'add_library(export{index} SHARED export{index}.c)', f'add_library(import{index} SHARED import{index}.c)',
                      f'target_link_libraries(import{index} PRIVATE export{index})'])
    (probe / "CMakeLists.txt").write_text("\n".join(cmake) + "\n")
    configure = [args.cmake, "-S", probe, "-B", probe / "build", "-G", args.generator]
    if args.generator.startswith("Visual Studio"):
        configure += ["-A", "x64"]
    command("probe-configure", configure, compiler=True)
    command("probe-build", [args.cmake, "--build", probe / "build", "--config", "Release", "--parallel", "4"], compiler=True)
    for index, marker in enumerate(markers):
        for table in ("export", "import"):
            binary = probe / "build/Release" / f"{table}{index}.dll"
            changed = copy.deepcopy(base)
            changed["files"].append(dict(source=str(binary), destination=binary.name, role="library", sha256=sha(binary)))
            name = table + "-" + marker
            command(name, ["--from-manifest", write(name, changed), run / ("bad-" + name)], "development code")
    command("restored-clean", ["--verify", manifest, package])
    print(f"PASS: {len(records)} actual paired-SDK cases. Evidence: {run}")


def installed_code():
    """Build and launch one actual installed SDK game, without using an Engine source project."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--installed-code", action="store_true")
    for key in ("sdk", "work", "cmake"):
        parser.add_argument("--" + key, required=True)
    parser.add_argument("--generator")
    args = parser.parse_args()
    sdk, work, cmake = Path(args.sdk).resolve(), Path(args.work).resolve(), Path(args.cmake).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="code-", dir=work))
    records = []

    def record(value):
        records.append(value)
        (run / "results.json").write_text(json.dumps(dict(scope="focused installed C3 code export; no G3 or source-unavailable proof",
            sdkManifestSha256=sha(sdk / "sdk.json"), cases=records), indent=2, ensure_ascii=False), encoding="utf-8")

    def command(name, arguments, compile_code=False, failure=None):
        env = dict(os.environ, PATH="")
        for key in ("PIXIE_SDK_ROOT", "PIXIE_EXPORT_CMAKE", "CMAKE_GENERATOR", "CMAKE_GENERATOR_PLATFORM"):
            env.pop(key, None)
        if compile_code:
            # CMake itself is selected explicitly. Let MSBuild retain the host
            # Windows utilities it needs; only no-code/replay require empty PATH.
            env["PATH"] = os.environ.get("PATH", "")
            env["PIXIE_EXPORT_CMAKE"] = str(cmake)
            env["MSBUILDDISABLENODEREUSE"] = "1"
            if args.generator:
                env["CMAKE_GENERATOR"] = args.generator
                if args.generator.startswith("Visual Studio"):
                    env["CMAKE_GENERATOR_PLATFORM"] = "x64"
        argv = [str(x) for x in arguments]
        log = run / (name + ".log")
        with log.open("wb") as stream:
            result = subprocess.run(argv, cwd=run, env=env, stdin=subprocess.DEVNULL, stdout=stream, stderr=subprocess.STDOUT, timeout=180)
        stdout = log.read_text(encoding="utf-8", errors="replace")
        record(dict(case=name, command=argv, exit=result.returncode, expectedError=failure, consumerCMake=env.get("PIXIE_EXPORT_CMAKE")))
        assert (result.returncode == 0 if failure is None else result.returncode == 1 and failure in stdout), (name, stdout)
        return stdout

    exporter = sdk / "tools/export/Release/pixie-export.exe"
    project = run / "Project Å"
    (project / "scenes").mkdir(parents=True)
    (project / "assets").mkdir()
    (project / "Installed.pixie").write_text('{"format":"pixie-project","version":1,"name":"Installed code","startScene":"scenes/main.scene"}')
    (project / "scenes/main.scene").write_text('{"format":"pixie-scene","version":1,"seed":5,"entities":[]}')
    (project / "scenes/complete-v2.scene").write_text('{"format":"pixie-scene","version":2,"seed":7,"sceneScopes":[{"id":"8","name":"empty"}],"entities":[],"future":{"keep":true}}')
    (project / "assets/explicit.txt").write_bytes(b"Game-owned explicit asset bytes\x00\n")
    command("installed-generator", [cmake, "-DPIXIE_PROJECT_DIR=" + str(project), "-DPIXIE_SDK_ROOT=" + str(sdk),
            "-DPIXIE_SDK_PROFILE=authoring", "-DPIXIE_ENGINE_MODE=PACKAGE", "-DPIXIE_PROJECT_BUILD_ROLE=authoring",
            "-DPIXIE_PROJECT_ADOPT_SDK=ON", "-DPIXIE_GENERATED_CANDIDATE_AUTHORING=OFF", "-P", sdk / "cmake/PixieAddCode.cmake"])
    registry = json.loads((project / ".pixie-code/manifest.json").read_text(encoding="utf-8"))
    assert registry["generatorFamily"] == "sdk-v1"
    target = registry["projectId"]
    cmakelists = project / "CMakeLists.txt"
    text = cmakelists.read_text(encoding="utf-8")
    sources = f"    SOURCES src/{target}Project.cpp\n"
    assert text.count(sources) == 1
    cmakelists.write_text(text.replace(sources, sources + "    ASSETS assets/explicit.txt\n") +
                          '\nadd_custom_target(unrelated_default_target ALL COMMAND "${CMAKE_COMMAND}" -E false)\n', encoding="utf-8")
    before = {p.relative_to(project).as_posix(): sha(p) for p in project.rglob("*") if p.is_file()}
    cache, output = run / "work", run / "Game Å"
    code_source = project / "src" / (target + "Project.cpp")
    source_bytes = code_source.read_bytes()
    failed_output = run / "Failed build"
    try:
        code_source.write_bytes(b'#error "C3 export retry fixture"\n' + source_bytes)
        failure = command("failed-build", [exporter, "--sdk", sdk, "--work", cache, project, failed_output],
                          compile_code=True, failure="could not build the declared shipping game target")
        assert "C3 export retry fixture" in failure and not failed_output.exists()
    finally:
        code_source.write_bytes(source_bytes)
    owner = cache / "build/pixie-export-owner.json"
    owner_hash = sha(owner)
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p,
                                  wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = kernel.CreateFileW(str(cache / ".pixie-export.lock"), 0xC0000000, 0, None, 3, 0x80, None)
    assert handle != ctypes.c_void_p(-1).value, ctypes.get_last_error()
    try:
        command("concurrent-work-refused", [exporter, "--sdk", sdk, "--work", cache, project, run / "Concurrent"],
                compile_code=True, failure="another export owns this work directory")
        assert not (run / "Concurrent").exists()
    finally:
        kernel.CloseHandle(handle)
    logs = command("actual-code-export", [exporter, "--sdk", sdk, "--work", cache, project, output], compile_code=True)
    assert sha(owner) == owner_hash
    assert f"Building installed shipping target: {target}" in logs and "unrelated_default_target" not in logs
    assert before == {p.relative_to(project).as_posix(): sha(p) for p in project.rglob("*") if p.is_file()}
    assert not (project / "bin").exists() and not (project / "build").exists()
    manifest = cache / "export-manifest.json"
    plan = json.loads(manifest.read_text(encoding="utf-8"))
    profile = json.loads((sdk / "profiles/shipping/Release/profile.json").read_text(encoding="utf-8"))
    product = json.loads((cache / "build/pixie-export/Release" / (target + ".json")).read_text(encoding="utf-8"))
    assert plan["version"] == 2 and product["version"] == 2 and plan["sdkBuildId"] == profile["sdkBuildId"] == product["sdkBuildId"]
    assert product["assets"] == ["assets/explicit.txt"] and not product["development"] and not product["dev_channel"]
    assert sha(output / "assets/explicit.txt") == before["assets/explicit.txt"]
    for relative in ("Installed.pixie", "scenes/main.scene", "scenes/complete-v2.scene"):
        assert sha(output / relative) == before[relative]
    cache_text = (cache / "build/CMakeCache.txt").read_text(encoding="utf-8")
    cache_fields = {line.split(":", 1)[0]: line.split("=", 1)[1] for line in cache_text.splitlines()
                    if ":" in line and "=" in line and not line.startswith(("//", "#"))}
    for field, value in {"PIXIE_PROJECT_BUILD_ROLE": "shipping-export", "PIXIE_SDK_PROFILE": "shipping",
                         "CMAKE_CONFIGURATION_TYPES": "Release", "PIXIE_PROJECT_ADOPT_SDK": "OFF"}.items():
        assert cache_fields[field] == value, field
    assert not list(output.glob("*-module.dll")) and not (output / "sdk.json").exists()
    command("verify-code-game", [exporter, "--sdk", sdk, "--verify", manifest, output])
    replay = run / "Replay Å"
    command("replay-code-game", [exporter, "--sdk", sdk, "--from-manifest", manifest, replay])
    assert {p.relative_to(replay).as_posix(): sha(p) for p in replay.rglob("*") if p.is_file()} == {p["destination"]: p["sha256"] for p in plan["files"]}
    screenshot = run / "frame.png"
    env = dict(os.environ, PATH="")
    env.pop("PIXIE_SDK_ROOT", None)
    argv = [str(replay / plan["player"]), "--quit-after", "3", "--screenshot", str(screenshot)]
    observed = {}
    with (run / "game.log").open("wb") as log:
        process = subprocess.Popen(argv, cwd=run, env=env, stdout=log, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic() + 10
            while process.poll() is None and time.monotonic() < deadline:
                observed.update(loaded_modules(process))
                if all(key in observed for key in ("pixie.dll", "glfw3.dll", "msvcp140.dll", "vcruntime140.dll")):
                    break
                time.sleep(0.04)
            exit_code = process.wait(timeout=30)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    assert exit_code == 0 and screenshot.is_file(), (exit_code, (run / "game.log").read_text(errors="replace"))
    modules = {key: {"path": observed[key], "sha256": sha(Path(observed[key]))} for key in ("pixie.dll", "glfw3.dll", "msvcp140.dll", "vcruntime140.dll")}
    assert all(Path(value["path"]).parent == replay for value in modules.values())
    assert modules["pixie.dll"]["sha256"] == profile["runtime"]["sha256"]
    record(dict(case="launch-exported-code-game", command=argv, exit=exit_code, loadedModules=modules, screenshotSha256=sha(screenshot)))
    # The target still configures, but its product declaration is deliberately
    # removed in this scratch project. The prior successful descriptor must not win.
    original_cmake = cmakelists.read_bytes()
    original_manifest = sha(manifest)
    try:
        cmakelists.write_text(cmakelists.read_text(encoding="utf-8").replace(
            "pixie_game(", "function(pixie_export_product)\nendfunction()\n\npixie_game(", 1), encoding="utf-8")
        stale_output = run / "Stale declaration"
        stale = command("removed-product-declaration", [exporter, "--sdk", sdk, "--work", cache, project, stale_output],
                        compile_code=True, failure="configured project did not declare an installed shipping export product")
        assert "Generating done" in stale and "Building installed shipping target" not in stale
        assert not stale_output.exists() and sha(manifest) == original_manifest
        assert list((cache / "build/previous-export-products").rglob(target + ".json"))
    finally:
        cmakelists.write_bytes(original_cmake)
    assert before == {p.relative_to(project).as_posix(): sha(p) for p in project.rglob("*") if p.is_file()}
    print(f"PASS: {len(records)} actual installed code-export cases. Evidence: {run}")


def code_preflight():
    """Check code-export refusals before executing any CMake project or compiler."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--code-preflight", action="store_true")
    for key in ("sdk", "work", "cmake"):
        parser.add_argument("--" + key, required=True)
    args = parser.parse_args()
    sdk = Path(args.sdk).resolve()
    profile = json.loads((sdk / "profiles/shipping/Release/profile.json").read_text(encoding="utf-8"))
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="preflight-", dir=work))
    exe = sdk / "tools/export/Release/pixie-export.exe"
    cmake = Path(args.cmake).resolve()
    records = []
    requirement = {key: profile[key] for key in ("sdkId", "platform", "architecture", "compilerVersion", "cpuFeatures")}
    requirement.update(schemaVersion=1, profile="authoring", requiredCapabilities=["moduleLoading"])

    def fixture(name, code=True, requirement_data=None):
        root = run / name
        (root / "scenes").mkdir(parents=True)
        (root / "Test.pixie").write_text('{"format":"pixie-project","version":1,"name":"Code Probe","startScene":"scenes/main.scene"}')
        (root / "scenes/main.scene").write_text('{"format":"pixie-scene","version":1,"seed":5,"entities":[]}')
        if code:
            (root / "CMakeLists.txt").write_text('message(FATAL_ERROR "PRECONDITION REGRESSION: CMake should never execute this fixture")\n')
        if requirement_data is not None:
            (root / ".pixie-code").mkdir()
            (root / ".pixie-code/sdk.json").write_text(json.dumps(requirement_data))
        return root

    def check(name, project, diagnostic, override=None, include_path=False, existing_cache=None):
        cache, output = existing_cache or run / (name + "-cache"), run / (name + "-output")
        before = {p.relative_to(project).as_posix(): sha(p) for p in project.rglob("*") if p.is_file()}
        cached = {p.relative_to(cache).as_posix(): sha(p) for p in cache.rglob("*") if p.is_file()}
        env = dict(os.environ, PATH=str(cmake.parent) if include_path else "")
        for key in ("PIXIE_SDK_ROOT", "PIXIE_EXPORT_CMAKE"):
            env.pop(key, None)
        if override is not None:
            env["PIXIE_EXPORT_CMAKE"] = str(override)
        command = [str(exe), "--sdk", str(sdk), "--work", str(cache), str(project), str(output)]
        result = subprocess.run(command, cwd=run, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                encoding="utf-8", errors="replace", timeout=90)
        (run / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        records.append(dict(case=name, command=command, exit=result.returncode, expectedError=diagnostic,
                            path=env["PATH"], cmakeOverride=env.get("PIXIE_EXPORT_CMAKE")))
        (run / "results.json").write_text(json.dumps(dict(scope="consumer preflight only; no code build or G3", cases=records), indent=2), encoding="utf-8")
        assert before == {p.relative_to(project).as_posix(): sha(p) for p in project.rglob("*") if p.is_file()}
        if diagnostic is None:
            assert result.returncode == 0 and (output / "pixie-player.exe").is_file(), (name, result.stdout)
        else:
            assert result.returncode == 1 and diagnostic in result.stdout, (name, result.stdout)
            assert not output.exists() and "PRECONDITION REGRESSION" not in result.stdout
            assert cached == {p.relative_to(cache).as_posix(): sha(p) for p in cache.rglob("*") if p.is_file()}
            if existing_cache is None:
                assert not cache.exists()

    check("missing-portable-requirement", fixture("legacy"), "compatible portable authoring SDK requirement")
    check("mixed-portable-requirement", fixture("mixed", requirement_data={**requirement, "sdkId": "0" * 64}), "requirement mismatch")
    check("private-requirement", fixture("private", requirement_data={**requirement, "profile": "local-development"}), "never private/shipping fallback")
    project = fixture("admitted", requirement_data=requirement)
    check("missing-consumer-cmake", project, "consumer's CMake")
    check("c3-cpu-order-admitted", fixture("sorted-cpu", requirement_data={**requirement, "cpuFeatures": sorted(requirement["cpuFeatures"])}),
          "consumer's CMake")
    for name, cpu, diagnostic in (("duplicate", requirement["cpuFeatures"] + ["avx2"], "duplicate portable CPU feature"),
                                  ("missing", ["avx2", "fma"], "requirement mismatch: cpuFeatures"),
                                  ("extra", requirement["cpuFeatures"] + ["sse2"], "requirement mismatch: cpuFeatures"),
                                  ("case", ["AVX2", "fma", "f16c"], "requirement mismatch: cpuFeatures")):
        check("cpu-" + name, fixture("cpu-" + name, requirement_data={**requirement, "cpuFeatures": cpu}), diagnostic)
    check("invalid-explicit-cmake-no-fallback", project, "must be the absolute path", override="cmake.exe", include_path=True)
    unowned = run / "unowned"
    (unowned / "build").mkdir(parents=True)
    (unowned / "build/CMakeCache.txt").write_text("Unowned cache fixture; preserve bytes")
    check("unowned-build-cache", project, "unowned export build cache", override=cmake, existing_cache=unowned)
    owned = run / "different-owner"
    (owned / "build").mkdir(parents=True)
    (owned / "build/pixie-export-owner.json").write_text('{"schemaVersion":1,"project":"another-project"}')
    check("mixed-build-cache", project, "belongs to another project", override=cmake, existing_cache=owned)
    check("no-code-ignores-invalid-cmake", fixture("no-code", code=False), None, override="cmake.exe")
    print(f"PASS: {len(records)} consumer preflight cases. Evidence: {run}")


def main():
    global PRODUCT_VERSION
    parser = argparse.ArgumentParser()
    for key in ("exporter", "engine", "work", "cmake", "generator", "sdk"):
        parser.add_argument("--" + key, required=True)
    parser.add_argument("--graphics-runtime", default="")
    args = parser.parse_args()
    args.exporter = str(Path(args.exporter).resolve())
    sdk = Path(args.sdk).resolve()
    authoring = json.loads((sdk / "profiles/authoring/Release/profile.json").read_text(encoding="utf-8"))
    PRODUCT_VERSION = json.loads((sdk / "profiles/shipping/Release/profile.json").read_text(encoding="utf-8"))["productVersion"]
    engine, work = Path(args.engine).resolve(), Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="run-", dir=work)).resolve()
    assert run.is_relative_to(work) and run != work
    records = []

    def record(value):
        records.append(value)
        (run / "results.json").write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding="utf-8")

    def command(name, arguments, failure=None):
        arguments = [str(x) for x in arguments]
        environment = dict(os.environ)
        environment.pop("PIXIE_SDK_ROOT", None)
        environment.pop("PIXIE_EXPORT_CMAKE", None)
        if arguments[0] == args.exporter:
            if len(arguments) == 3 and not arguments[1].startswith("--"):
                project = Path(arguments[1])
                arguments.extend(["--work", str(run / (project.name + " work"))])
                if (project / "CMakeLists.txt").is_file():
                    environment["PIXIE_EXPORT_CMAKE"] = str(Path(args.cmake).resolve())
                    environment["CMAKE_GENERATOR"] = args.generator
                    if args.generator.startswith("Visual Studio"):
                        environment["CMAKE_GENERATOR_PLATFORM"] = "x64"
                    environment["MSBUILDDISABLENODEREUSE"] = "1"
                else:
                    environment["PATH"] = ""
            arguments[1:1] = ["--sdk", str(sdk)]
        result = subprocess.run(arguments, cwd=run, env=environment, text=True, encoding="utf-8",
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

    manifests, packages, projects = {}, {}, {}
    for name in ("boids", "sprites"):
        # Spaces and Unicode qualify the complete CLI -> native -> CMake argument-vector path.
        project = run / (name + " source é")
        shutil.copytree(engine / "samples" / name, project, ignore=shutil.ignore_patterns("build", "bin", ".local", ".pixie-code"))
        projects[name] = project
        cache = run / (project.name + " work")
        if name == "boids":
            # Adapt only this copied sample to the installed package contract.
            # The engine tree supplies fixture inputs, never a build graph.
            (project / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.25)
project(BoidsExport LANGUAGES C CXX)
set(PIXIE_ENGINE_MODE PACKAGE)
include("${PIXIE_SDK_ROOT}/cmake/PixieProject.cmake")
pixie_game(boids SOURCES src/game/Boid.cpp src/game/BoidsProject.cpp src/game/BoidsScene.cpp INCLUDE_DIRS src/game)
''', encoding="utf-8")
            requirement = {key: authoring[key] for key in ("sdkId", "profile", "platform", "architecture", "compilerVersion", "cpuFeatures")}
            (project / ".pixie-code").mkdir()
            (project / ".pixie-code/sdk.json").write_text(json.dumps({"schemaVersion": 1, **requirement}), encoding="utf-8")
        project_file = next(project.glob("*.pixie"))
        data = json.loads(project_file.read_text(encoding="utf-8"))
        data["future_export_fixture"] = {"preserve": [1, "unknown", {"data": True}]}
        project_file.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
        output = run / (name + " export é")
        command(name + "-export", [args.exporter, project, output])
        manifest = cache / "export-manifest.json"
        manifests[name], packages[name] = manifest, output
        command(name + "-verify", [args.exporter, "--verify", manifest, output])
        assert (output / project_file.name).read_bytes() == project_file.read_bytes()
        product_files = list(((cache / "build" if name == "boids" else cache) / "pixie-export/Release").glob("*.json"))
        assert len(product_files) == 1
        product = json.loads(product_files[0].read_text(encoding="utf-8"))
        assert not product["development"] and not product["dev_channel"]
        data = json.loads(manifest.read_text(encoding="utf-8"))
        assert all("-module" not in x["destination"] and "-capi" not in x["destination"] for x in data["files"])
        assert data["engine_version"] == PRODUCT_VERSION, data.get("engine_version")
        runnable = output
        if args.graphics_runtime:
            # The raw default export remains untouched. CI may qualify a second explicit manifest
            # with its pinned software OpenGL provider, instead of relying on a GPU driver.
            graphics = copy.deepcopy(data)
            destinations = {x["destination"].lower() for x in graphics["files"]}
            provider = Path(args.graphics_runtime).resolve()
            assert (provider / "opengl32.dll").is_file()
            for file in opengl_closure(provider):
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
        assert ("PixieEngine " + PRODUCT_VERSION) in (run / (name + "-run.log")).read_text(encoding="utf-8", errors="replace")
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
        engine_bytes = (shipping / "pixie.dll").read_bytes()
        assert b"--quit-after-steps" not in engine_bytes
        # The local development channel, its dispatcher and the MCP bridge stay out of every export.
        for marker in (b"LocalCommandChannel", b"PIXIE_DEVELOPMENT_CHANNEL_V1", b"pixie_dev_dispatch", b"world.hello"):
            assert marker not in engine_bytes, (name, marker)
        for pattern in ("*PixieMcp*", "*ModelContextProtocol*", "*pixie-mcp*", "*channel-write-probe*", "*devchannel*", "*-module.dll"):
            assert not list(shipping.rglob(pattern)), (name, pattern)
        rejected = command(name + "-capture-option-excluded",
                           [shipping / shipping_manifest["player"], "--quit-after-steps", "0"], "usage:")
        assert rejected.returncode == 2
        for flag, value in (("--dev-channel", "forbidden"), ("--dev-capture-dir", "captures"), ("--dev-hidden", "on")):
            rejected = command(name + "-" + flag[2:] + "-excluded",
                               [shipping / shipping_manifest["player"], flag, value], "usage:")
            assert rejected.returncode == 2
    # Barrier 1: a development-enabled engine in a falsely declared shipping manifest.
    broken = copy.deepcopy(base)
    replace_source(broken, "pixie.dll", sdk / authoring["runtime"]["path"])
    path = write_manifest("development-engine", broken)
    command("barrier1-development-engine", [args.exporter, "--from-manifest", path, run / "bad-engine"], "mandatory selected shipping")
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
project(ExportProbes C CXX)
add_executable(loader loader.c)
add_library(private_dependency SHARED dependency.c)
add_library(forwarder SHARED empty.c forwarder.def)
''' + f'''
add_library(local_channel SHARED "{engine.as_posix()}/engine/platform/src/LocalCommandChannel.cpp")
target_include_directories(local_channel PRIVATE "{engine.as_posix()}/engine/platform/include")
target_compile_definitions(local_channel PRIVATE PIXIE_PLATFORM_BUILD)
target_link_libraries(local_channel PRIVATE advapi32)
set_target_properties(local_channel PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
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
    transport_only = binaries / "local_channel.dll"
    transport_bytes = transport_only.read_bytes()
    assert b"pixie_dev_dispatch" not in transport_bytes and b"pixie_module_load" not in transport_bytes
    assert b"LocalCommandChannel" in transport_bytes and b"PIXIE_DEVELOPMENT_CHANNEL_V1" in transport_bytes
    broken = copy.deepcopy(base)
    broken["files"].append(dict(source=str(transport_only), destination=transport_only.name, role="library", sha256=sha(transport_only)))
    command("barrier2-transport-without-capi", [args.exporter, "--from-manifest", write_manifest("transport-only", broken),
                                              run / "bad-transport"], "development code")
    marker_only = probe / "marker-only.dll"
    marker_only.write_bytes((package / "pixie.dll").read_bytes() + b"PIXIE_DEVELOPMENT_CHANNEL_V1\0")
    broken = copy.deepcopy(base)
    broken["files"].append(dict(source=str(marker_only), destination=marker_only.name, role="library", sha256=sha(marker_only)))
    command("barrier2-channel-marker-without-export", [args.exporter, "--from-manifest", write_manifest("marker-only", broken),
                                                     run / "bad-channel-marker"], "development code")

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
                "omits mandatory")
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
    project = projects["boids"]
    command("original-output-junction", [args.exporter, project, link / "out"], "reparse")
    assert sentinel.read_text(encoding="utf-8") == "preserve" and not (target / "out").exists()
    command("restored-clean-boids", [args.exporter, "--verify", manifests["boids"], package])
    command("restored-clean-sprites", [args.exporter, "--verify", manifests["sprites"], packages["sprites"]])
    (run / "results.json").write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"PASS: {len(records)} export cases. Evidence: {run}")


if __name__ == "__main__":
    if "--producer-refusals" in sys.argv:
        producer_refusals()
    elif "--paired-sdk" in sys.argv:
        paired_sdk()
    elif "--code-preflight" in sys.argv:
        code_preflight()
    elif "--installed-code" in sys.argv:
        installed_code()
    else:
        main()
