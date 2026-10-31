"""Qualify the local development channel and its MCP bridge for one configuration, on this machine.

Builds the bridge, its official-client tests and the editor (with and without channel save), then
runs, each in a disposable project under --work: the player's real pipe and MCP cases, the editor's
Inspector/history cases in both save builds, the editor's direct pipe and fault fixtures, a project
with code opened cold through MCP, and a server killed while its host runs. Nothing is published or
registered with any client. Windows only: the qualified transport is a named pipe.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import uuid

ENGINE = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", choices=("Debug", "Release"), required=True)
    parser.add_argument("--bin", default=str(ENGINE / "bin"), help="the engine's bin/ (build-ci/bin for the ci preset)")
    parser.add_argument("--work", default=str(ENGINE / "build/tests/mcp"))
    parser.add_argument("--cmake", default=shutil.which("cmake") or "cmake",
                        help="CMake that writes the cold project's code (cmake/PixieAddCode.cmake)")
    parser.add_argument("--skip-cold", action="store_true", help="leave out the cold code project, which builds the engine again")
    args = parser.parse_args()
    config = args.config
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix=f"run-{config}-", dir=work)).resolve()
    assert run.is_relative_to(work) and run != work
    records = []

    def log_run(name, command, timeout, cwd=None):
        started = time.monotonic()
        result = subprocess.run([str(x) for x in command], cwd=cwd, text=True, encoding="utf-8", errors="replace",
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        (run / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        assert result.returncode == 0, f"{name} exited {result.returncode}; see {run / (name + '.log')}"
        return result, time.monotonic() - started

    def build(name, project, *properties):
        log_run("build-" + name, ["dotnet", "build", project, "-c", config, *properties], 600)

    def suite(name, command, timeout):
        """Runs a test client; None in its command stands for the receipt it writes."""
        receipt = run / (name + ".json")
        result, seconds = log_run(name, [receipt if x is None else x for x in command], timeout)
        cases = json.loads(receipt.read_text(encoding="utf-8"))
        assert cases and all(case["passed"] for case in cases), name
        records.append({"suite": name, "cases": len(cases), "seconds": round(seconds, 2), "receipt": str(receipt),
                        "summary": result.stdout.strip().splitlines()[-1]})

    def project_copy(name, source):
        folder = run / "projetos ação" / name
        shutil.copytree(source, folder, ignore=shutil.ignore_patterns("build", "bin", "imgui.ini"))
        return folder

    bin_dir = Path(args.bin).resolve() / config
    player = bin_dir / "pixie-player.exe"
    probe = bin_dir / "channel-write-probe-module.dll"
    for native in (player, probe, bin_dir / "pixie.dll"):
        assert native.is_file(), f"build the engine's {config} configuration first: {native}"
    build("mcp", ENGINE / "tools/pixie-mcp/PixieMcp.csproj")
    build("mcp-tests", ENGINE / "tools/pixie-mcp.tests/PixieMcp.Tests.csproj")
    build("channel-tests", ENGINE / "tests/editor/channel/PixieEditor.ChannelTests.csproj")
    # The editor loads the engine from the nearest bin/Release or bin/Debug above it, Release first.
    # Both editors are built beside a bin/ holding only this configuration's engine, apart from the
    # ordinary editor build, so a Debug run never loads a Release (or stale) engine.
    stage = run / "engine"
    (stage / "bin" / config).mkdir(parents=True)
    for library in bin_dir.glob("*.dll"):
        shutil.copy2(library, stage / "bin" / config)
    for name, properties in (("editor", []), ("editor-without-save", ["-p:PixieChannelSave=false"])):
        build(name, ENGINE / "editor/PixieEditor/PixieEditor.csproj", *properties,
              "-p:OutputPath=" + str(stage / (name + "-bin")) + "/", "-p:IntermediateOutputPath=" + str(run / (name + "-obj")) + "/")
    mcp = ENGINE / f"tools/pixie-mcp/bin/{config}/net10.0/PixieMcp.dll"
    tests = ENGINE / f"tools/pixie-mcp.tests/bin/{config}/net10.0/PixieMcp.Tests.exe"
    editor = next((stage / "editor-bin").rglob("PixieEditor.exe"))
    editor_without_save = next((stage / "editor-without-save-bin").rglob("PixieEditor.exe"))
    channel_tests = ENGINE / f"tests/editor/channel/bin/{config}/net10.0/PixieEditor.ChannelTests.exe"
    sprites = ENGINE / "samples/sprites"

    suite("player-mcp", [tests, player, project_copy("player", sprites), mcp, run / "captures player", None], 300)
    suite("editor-mcp-save", [tests, "--editor", editor, project_copy("editor save", sprites), mcp,
                              run / "captures editor", None, "save=on"], 300)
    suite("editor-mcp-without-save", [tests, "--editor", editor_without_save, project_copy("editor without save", sprites), mcp,
                                      run / "captures editor without save", None, "save=off"], 300)
    suite("editor-pipe-and-faults", [channel_tests, probe, run / "fault projects", None], 300)
    physics = run / "projetos ação"
    suite("editor-mcp-physics", [tests, "--editor-physics", editor, empty_project(physics / "física sem código"),
                                 empty_project(physics / "segunda sessão"), legacy_project(physics / "legado v1"), mcp,
                                 run / "captures physics", None], 600)
    records.append(orphan_case(run, mcp, player, project_copy("orphan", sprites)))
    if not args.skip_cold:
        cold = code_project(run / "projetos ação" / "Cold Code", args.cmake)
        deadline = code_project(run / "projetos ação" / "Deadline Code", args.cmake)
        empty = run / "projetos ação" / "not a project"
        empty.mkdir()
        suite("editor-mcp-cold-code", [tests, "--editor-code", editor, cold, mcp, run / "captures code", None, empty, deadline], 3600)
    (run / "results.json").write_text(json.dumps({"config": config, "suites": records}, indent=2), encoding="utf-8")
    print(f"PASS: {sum(r.get('cases', 1) for r in records)} MCP/development-channel cases in {config}. Evidence: {run}")


def empty_project(folder):
    """A project folder without code and an empty scene, for authoring from nothing."""
    (folder / "scenes").mkdir(parents=True)
    (folder / "physics.pixie").write_text(json.dumps({"format": "pixie-project", "version": 1, "name": "physics without code",
                                                      "startScene": "scenes/main.scene"}, indent=2) + "\n", encoding="utf-8")
    (folder / "scenes" / "main.scene").write_text(json.dumps({"format": "pixie-scene", "version": 1, "seed": 1, "entities": []}) + "\n",
                                                  encoding="utf-8")
    return folder


def legacy_project(folder):
    """The frozen version 1 movement laboratory with two differing legacy motor gravities (1650 and 0)."""
    scene = json.loads((ENGINE / "tests/runtime/fixtures/movement-lab-v1.scene").read_text(encoding="utf-8"))
    motor = next(e for e in scene["entities"] if "CharacterMotor2D" in e["components"])
    motor["components"]["CharacterMotor2D"]["gravity"] = 1650
    still = json.loads(json.dumps(motor))
    still["id"], still["name"] = "0000000000000201", "weightless character"
    still["components"]["CharacterMotor2D"]["gravity"] = 0
    scene["entities"].append(still)
    empty_project(folder)
    (folder / "scenes" / "main.scene").write_text(json.dumps(scene, indent=2) + "\n", encoding="utf-8")
    return folder


def code_project(folder, cmake):
    """A project with code, as the editor's Add code makes it, never built: the editor builds it on open."""
    (folder / "scenes").mkdir(parents=True)
    (folder / (folder.name + ".pixie")).write_text(json.dumps(
        {"format": "pixie-project", "version": 1, "name": folder.name, "startScene": "scenes/main.scene"}, indent=2) + "\n", encoding="utf-8")
    (folder / "scenes/main.scene").write_text(json.dumps({"format": "pixie-scene", "version": 1, "seed": 1, "entities": [
        {"id": "0123456789abcdef", "name": "spinner", "components": {
            "Transform2D": {"v": 1, "position": [0, 0], "rotation": 0, "scale": [1, 1]},
            "Shape": {"v": 1, "kind": "Triangle", "size": [24, 24]},
            "Spinner": {"v": 1, "speed": 1.5}}}]}), encoding="utf-8")
    subprocess.run([cmake, "-DPIXIE_PROJECT_DIR=" + str(folder), "-P", str(ENGINE / "cmake/PixieAddCode.cmake")],
                   check=True, stdout=subprocess.DEVNULL)
    return folder


def hosts_with(marker):
    """Process IDs whose command line names this channel."""
    query = ("Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*" + marker + "*' -and "
             "$_.Name -ne 'dotnet.exe' -and $_.Name -ne 'powershell.exe' } | ForEach-Object { $_.ProcessId }")
    output = subprocess.run(["powershell", "-NoProfile", "-Command", query], capture_output=True, text=True, timeout=60).stdout
    return [int(x) for x in output.split()]


def orphan_case(run, mcp, player, project):
    """A server killed outright (as a crash would) takes its host with it."""
    channel = "orphan-" + uuid.uuid4().hex
    server = subprocess.Popen(["dotnet", str(mcp), "--host", str(player), "--project", str(project),
                               "--captures", str(run / "captures orphan"), "--channel", channel],
                              stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        deadline = time.monotonic() + 30
        while not (hosts := hosts_with(channel)) and time.monotonic() < deadline:
            time.sleep(0.5)
        assert hosts, "the host never started"
        server.kill()
        server.wait(timeout=10)
        deadline = time.monotonic() + 15
        while (left := hosts_with(channel)) and time.monotonic() < deadline:
            time.sleep(0.5)
        if left:
            for pid in left:
                subprocess.run(["taskkill", "/F", "/T", "/PID", str(pid)], capture_output=True)
        assert not left, f"host {left} outlived its killed server"
        return {"suite": "server-killed-closes-host", "cases": 1, "host_pids": hosts}
    finally:
        if server.poll() is None:
            server.kill()


if __name__ == "__main__":
    os.environ.setdefault("DOTNET_CLI_TELEMETRY_OPTOUT", "1")
    main()
