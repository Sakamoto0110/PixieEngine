"""Run private Pixie channel qualification from an explicit artifact and fixture inventory.

The inputs JSON identifies one SDK tuple, the complete runnable artifact directories,
and the immutable project fixtures. Use --preflight-only to verify them without
launching a host. All execution and copied projects live under the supplied work root.

Schema 1 takes an sdk object with root, profile, configuration, sdkId, sdkBuildId,
manifestSha256 and profileSha256. Each named artifact supplies root, entry and files.
The fixtures object supplies noCode (root and files) and legacyScene (path and sha256).
File inventory entries contain relative path, size and sha256.

The cold object is deferred with a reason, or ready with success and deadline
project inventories and a hashed authoringHandoff. Its receipt binds the exact
SDK manifest and build IDs, both fixture inventories and the deadline delay.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import tempfile
import time
import uuid


ARTIFACT_ENTRIES = {
    "bridge": "PixieMcp.dll",
    "client": "PixieMcp.Tests.dll",
    "editorSaveOn": "PixieEditor.exe",
    "editorSaveOff": "PixieEditor.exe",
    "channelTests": "PixieEditor.ChannelTests.dll",
    "writeProbe": None,
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def unique_object(pairs):
    result = dict(pairs)
    require(len(result) == len(pairs), "JSON contains a duplicate key")
    return result


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique_object)


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def regular_path(path):
    info = path.lstat()
    return not stat.S_ISLNK(info.st_mode) and not (
        getattr(info, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT)


def absolute_path(value):
    path = Path(value)
    require(path.is_absolute(), f"Expected an explicit absolute path: {path}")
    for entry in reversed((path, *path.parents)):
        if entry.exists() or entry.is_symlink():
            require(regular_path(entry), f"Symbolic links and junctions are not allowed: {entry}")
    return path.resolve()


def relative_path(name):
    require(isinstance(name, str) and "\\" not in name and ":" not in name and
            all(part not in ("", ".", "..") for part in name.split("/")), f"Invalid inventory path: {name!r}")
    return Path(name)


def checked_hash(value):
    require(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value), "Expected a lowercase SHA-256")
    return value


def inventory(root):
    require(root.is_dir(), f"Missing inventory directory: {root}")
    files = {}
    for directory, subdirs, names in os.walk(root):
        parent = Path(directory)
        for name in [*subdirs, *names]:
            path = parent / name
            require(regular_path(path), f"Symbolic links and junctions are not allowed: {path}")
        for name in names:
            path = parent / name
            require(path.is_file(), f"Expected a regular file: {path}")
            files[path.relative_to(root).as_posix()] = (path.stat().st_size, sha256(path))
    return files


def declared_files(records):
    files, folded = {}, set()
    for item in records:
        name = item["path"]
        relative_path(name)
        require(name.casefold() not in folded, f"Duplicate or case-colliding inventory path: {name}")
        size = item["size"]
        require(type(size) is int and size >= 0, f"Invalid inventory size: {name}")
        files[name] = (size, checked_hash(item["sha256"]))
        folded.add(name.casefold())
    require(files, "The file inventory is empty")
    return files


def verify_inventory(root, files):
    actual = inventory(root)
    if actual != files:
        difference = sorted(name for name in actual.keys() | files.keys() if actual.get(name) != files.get(name))
        raise ValueError(f"Inventory mismatch in {root}: {', '.join(difference[:8])}")


def checked_directory(record):
    root = absolute_path(record["root"])
    files = declared_files(record["files"])
    verify_inventory(root, files)
    return root, files


def checked_file(record):
    path = absolute_path(record["path"])
    require(path.is_file() and sha256(path) == checked_hash(record["sha256"]), f"File hash mismatch: {path}")
    return path


def selection_options(sdk, root):
    return ["--sdk", str(root), "--sdk-profile", sdk["profile"], "--native-configuration", sdk["configuration"],
            "--sdk-id", sdk["sdkId"], "--sdk-build-id", sdk["sdkBuildId"], "--sdk-manifest-sha256", sdk["manifestSha256"]]


def check_sdk(sdk):
    root = absolute_path(sdk["root"])
    config = sdk["configuration"]
    require(sdk["profile"] == "local-development" and config in ("Debug", "Release"),
            "Qualification requires an explicit private local-development SDK configuration")
    for name in ("sdkId", "sdkBuildId", "manifestSha256", "profileSha256"):
        checked_hash(sdk[name])
    manifest = root / "sdk.json"
    require(manifest.is_file() and regular_path(manifest) and sha256(manifest) == sdk["manifestSha256"],
            "SDK catalog SHA-256 mismatch")
    catalog = read_json(manifest)
    require(type(catalog["schemaVersion"]) is int and catalog["schemaVersion"] == 1 and
            catalog["distributionClass"] == "local-private" and catalog["sdkId"] == sdk["sdkId"],
            "SDK catalog does not identify the selected private SDK")
    files = declared_files(catalog["files"])
    require(not any(name.casefold() == "sdk.json" for name in files), "The SDK catalog cannot include itself")
    files["sdk.json"] = (manifest.stat().st_size, sdk["manifestSha256"])
    verify_inventory(root, files)
    matches = [item for item in catalog["profiles"] if item["profile"] == sdk["profile"] and item["configuration"] == config]
    require(len(matches) == 1, "The SDK must declare the selected profile and configuration exactly once")
    entry = matches[0]
    profile_dir = f"profiles/{sdk['profile']}/{config}"
    profile_name = f"{profile_dir}/profile.json"
    require(entry["manifest"] == profile_name and entry["manifestSha256"] == sdk["profileSha256"] and
            files.get(profile_name, (None, None))[1] == sdk["profileSha256"] and entry["sdkBuildId"] == sdk["sdkBuildId"],
            "Selected SDK profile manifest or build identity mismatch")
    profile = read_json(root / profile_name)
    for field in ("sdkId", "sdkBuildId", "profile", "configuration"):
        require(profile[field] == sdk[field], f"Selected SDK profile {field} mismatch")
    require(profile["distributionClass"] == "local-private" and profile["capabilities"]["nativeAutomation"] is True,
            "Selected SDK has no private native automation capability")
    for role, filename in (("player", "pixie-player.exe"), ("runtime", "pixie.dll")):
        name = f"{profile_dir}/bin/{filename}"
        require(profile[role]["path"] == name and files.get(name, (None, None))[1] == profile[role]["sha256"],
                f"Selected SDK {role} does not match its profile inventory")
    return root, files, profile


def check_project(record, *, cold=False):
    root, files = checked_directory(record)
    require(sum("/" not in name and name.lower().endswith(".pixie") for name in files) == 1 and
            "scenes/main.scene" in files, f"Expected one .pixie project and scenes/main.scene: {root}")
    require(("CMakeLists.txt" in files) == cold, f"Unexpected project code state: {root}")
    require(not any(name.split("/")[0].casefold() in ("build", "bin") or
                    Path(name).suffix.lower() in (".dll", ".exe", ".pdb", ".obj") or
                    Path(name).name.casefold() == "cmakecache.txt" for name in files),
            f"Project input contains build outputs: {root}")
    return root, files


def check_cold(cold, sdk, profile):
    if cold["status"] == "deferred":
        require(isinstance(cold["reason"], str) and cold["reason"].strip(), "A deferred cold suite requires a reason")
        return None
    require(cold["status"] == "ready", "Cold state must be ready or deferred")
    handoff = read_json(checked_file(cold["authoringHandoff"]))
    require(handoff["sdkManifestSha256"] == sdk["manifestSha256"] and handoff["sdkBuildId"] == sdk["sdkBuildId"] and
            handoff["neverBuilt"] is True, "Cold authoring handoff does not identify the selected SDK and never-built inputs")
    delay = handoff["deadlineDelaySeconds"]
    require(type(delay) is int and 3 < delay <= 120, "The deadline fixture must have a separate bounded build delay")
    roles = ("success", "deadline")
    projects = [check_project(cold[key], cold=True) for key in roles]
    require(projects[0][0] != projects[1][0], "Cold success and deadline must use distinct fixtures")
    for key, (root, files) in zip(roles, projects):
        require(declared_files(handoff["fixtures"][key]) == files, f"Cold authoring handoff does not cover the {key} fixture bytes")
        require(".pixie-code/sdk.json" in files, f"Missing portable installed SDK requirement: {root}")
        requirement = read_json(root / ".pixie-code/sdk.json")
        fields = ("schemaVersion", "sdkId", "profile", "platform", "architecture", "compilerVersion")
        require(all(requirement[field] == profile[field] for field in fields) and
                sorted(requirement["cpuFeatures"]) == sorted(profile["cpuFeatures"]),
                f"Cold project SDK requirement does not match the selected private profile: {root}")
    return projects


def preflight(document):
    require(type(document["schemaVersion"]) is int and document["schemaVersion"] == 1, "Unknown inputs schema")
    sdk = document["sdk"]
    sdk_root, sdk_files, profile = check_sdk(sdk)
    artifacts = {}
    for role, expected in ARTIFACT_ENTRIES.items():
        record = document["artifacts"][role]
        root, files = checked_directory(record)
        entry = record["entry"]
        relative_path(entry)
        require(entry in files and (entry == expected if expected else entry.lower().endswith(".dll")),
                f"Invalid entry point for {role}: {entry}")
        artifacts[role] = (root, files, entry)
    fixtures = document["fixtures"]
    no_code = check_project(fixtures["noCode"])
    legacy = checked_file(fixtures["legacyScene"])
    cold = check_cold(document["cold"], sdk, profile)
    return sdk_root, sdk_files, profile, artifacts, no_code, legacy, cold


def stage_directory(source, destination, files):
    shutil.copytree(source, destination)
    verify_inventory(destination, files)
    return destination


def empty_project(folder):
    """A project folder without code and an empty scene, for authoring from nothing."""
    (folder / "scenes").mkdir(parents=True)
    write_json(folder / "physics.pixie", {"format": "pixie-project", "version": 1, "name": "physics without code",
                                        "startScene": "scenes/main.scene"})
    write_json(folder / "scenes/main.scene", {"format": "pixie-scene", "version": 1, "seed": 1, "entities": []})
    return folder


def legacy_project(folder, scene_path):
    """The version 1 movement laboratory with two differing legacy motor gravities."""
    scene = read_json(scene_path)
    motor = next(entity for entity in scene["entities"] if "CharacterMotor2D" in entity["components"])
    motor["components"]["CharacterMotor2D"]["gravity"] = 1650
    still = json.loads(json.dumps(motor))
    still["id"], still["name"] = "0000000000000201", "weightless character"
    still["components"]["CharacterMotor2D"]["gravity"] = 0
    scene["entities"].append(still)
    empty_project(folder)
    write_json(folder / "scenes/main.scene", scene)
    return folder


def owned_hosts(powershell, env, parent, player, channel, *, stop=False):
    query = (
        "$ErrorActionPreference = 'Stop'; "
        "Get-CimInstance Win32_Process -Filter ('ParentProcessId = ' + [int]$env:PIXIE_TEST_PARENT) | "
        "Where-Object { $_.ExecutablePath -eq $env:PIXIE_TEST_HOST -and "
        "$_.CommandLine -like ('*--dev-channel ' + $env:PIXIE_TEST_CHANNEL + '*') } | "
        "ForEach-Object { "
        "if ($env:PIXIE_TEST_STOP -eq '1') { Stop-Process -Id $_.ProcessId -ErrorAction SilentlyContinue }; $_.ProcessId }"
    )
    process_env = {**env, "PIXIE_TEST_PARENT": str(parent), "PIXIE_TEST_HOST": str(player),
                   "PIXIE_TEST_CHANNEL": channel, "PIXIE_TEST_STOP": str(int(stop))}
    result = subprocess.run([powershell, "-NoProfile", "-NonInteractive", "-Command", query],
                            env=process_env, check=True, capture_output=True, text=True, timeout=15)
    return [int(pid) for pid in result.stdout.split()]


def orphan_case(run, env, dotnet, powershell, mcp, player, project, selection):
    """Kill the owned bridge and verify that its job also closes the owned host."""
    channel = "orphan-" + uuid.uuid4().hex
    command = [dotnet, mcp, "--host", player, "--project", project, "--captures", run / "captures orphan",
               "--channel", channel, *selection]
    with (run / "server-killed-closes-host.log").open("wb") as log:
        server = subprocess.Popen(command, cwd=run, env=env, stdin=subprocess.PIPE,
                                  stdout=subprocess.DEVNULL, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            hosts = []
            deadline = time.monotonic() + 30
            while not hosts and server.poll() is None and time.monotonic() < deadline:
                hosts = owned_hosts(powershell, env, server.pid, player, channel)
                if not hosts:
                    time.sleep(0.25)
            require(len(hosts) == 1, "The owned player did not start exactly once")
            server.kill()
            server.wait(timeout=10)
            deadline = time.monotonic() + 15
            left = owned_hosts(powershell, env, server.pid, player, channel)
            while left and time.monotonic() < deadline:
                time.sleep(0.25)
                left = owned_hosts(powershell, env, server.pid, player, channel)
            require(not left, f"Owned player {left} outlived its killed bridge")
            return {"suite": "server-killed-closes-host", "status": "passed", "cases": 1,
                    "hostPids": hosts, "bridgePid": server.pid, "argv": [str(value) for value in command]}
        finally:
            if server.poll() is None:
                server.kill()
                server.wait(timeout=10)
            owned_hosts(powershell, env, server.pid, player, channel, stop=True)
            server.stdin.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True, help="Absolute path to the exact artifact and fixture inventory JSON")
    parser.add_argument("--work", required=True, help="Absolute owned directory for a fresh run")
    parser.add_argument("--preflight-only", action="store_true", help="Verify inputs and record the cold status without launching processes")
    parser.add_argument("--include-committed-recovery", action="store_true",
                        help="Run the committed recovery suite with a private Debug SDK and N7 channel test artifact")
    args = parser.parse_args()
    input_path = absolute_path(args.inputs)
    input_hash = sha256(input_path)
    document = read_json(input_path)
    require(sha256(input_path) == input_hash, "Input manifest changed during preflight")
    sdk_root, sdk_files, profile, artifacts, no_code, legacy, cold = preflight(document)
    sdk = document["sdk"]
    require(not args.include_committed_recovery or sdk["configuration"] == "Debug",
            "Committed recovery requires a private Debug SDK")
    work = absolute_path(args.work)
    roots = [sdk_root, no_code[0], *(record[0] for record in artifacts.values()), *(record[0] for record in (cold or []))]
    require(not any(work.is_relative_to(root) for root in roots), "Work must be outside the SDK, artifacts and project inputs")
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix=f"run-{sdk['configuration']}-", dir=work))
    selection = selection_options(sdk, sdk_root)
    records = []
    report = {"inputs": str(input_path), "inputsSha256": input_hash, "selection": selection, "sdkPayloadFiles": len(sdk_files) - 1,
              "preflight": "passed", "mode": "preflight" if args.preflight_only else "run", "suites": records}
    if cold is None:
        records.append({"suite": "editor-mcp-cold-code", "status": "omitted", "reason": document["cold"]["reason"]})
    if args.preflight_only and args.include_committed_recovery:
        records.append({"suite": "editor-committed-recovery", "status": "omitted", "reason": "Preflight only"})
    write_json(run / "results.json", report)
    if args.preflight_only:
        print(f"PASS: verified explicit qualification inputs. Evidence: {run}")
        return 0

    require(os.name == "nt", "The Editor qualification requires Windows named pipes")
    dotnet = shutil.which("dotnet")
    powershell = shutil.which("powershell") or shutil.which("pwsh")
    require(dotnet and powershell, "The qualification requires dotnet and PowerShell")
    env = os.environ.copy()
    env.pop("PIXIE_SDK_ROOT", None)
    for key, directory in (("APPDATA", "app-data"), ("LOCALAPPDATA", "local-app-data"), ("TEMP", "temp"), ("TMP", "temp")):
        location = run / directory
        location.mkdir(exist_ok=True)
        env[key] = str(location)
    env["DOTNET_CLI_TELEMETRY_OPTOUT"] = "1"
    staged = {}
    for role, (root, files, entry) in artifacts.items():
        stage = stage_directory(root, run / "artifacts" / role, files)
        staged[role] = stage / relative_path(entry)
    legacy_copy = run / "legacy-input.scene"
    shutil.copyfile(legacy, legacy_copy)
    require(sha256(legacy_copy) == document["fixtures"]["legacyScene"]["sha256"], "Legacy scene changed while staging")

    def project_copy(name, source=no_code):
        return stage_directory(source[0], run / "projetos ação" / name, source[1])

    def suite(name, command, timeout):
        receipt = run / (name + ".json")
        log = run / (name + ".log")
        argv = [str(receipt if value is None else value) for value in command]
        record = {"suite": name, "status": "failed", "argv": argv, "receipt": str(receipt), "log": str(log)}
        started = time.monotonic()
        try:
            with log.open("wb") as output:
                result = subprocess.run(argv, cwd=run, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=timeout)
            record["exitCode"] = result.returncode
            cases = read_json(receipt)
            record["cases"] = len(cases)
            if result.returncode == 0 and cases and all(case["passed"] is True for case in cases):
                record["status"] = "passed"
        except (OSError, ValueError, subprocess.TimeoutExpired) as error:
            record["error"] = str(error)
        finally:
            record["seconds"] = round(time.monotonic() - started, 3)
            records.append(record)
            write_json(run / "results.json", report)

    player = sdk_root / relative_path(profile["player"]["path"])
    mcp, tests = staged["bridge"], staged["client"]
    editor = staged["editorSaveOn"]
    suite("player-mcp", [dotnet, tests, player, project_copy("player"), mcp, run / "captures player", None, *selection], 300)
    for save, host in (("on", editor), ("off", staged["editorSaveOff"])):
        name = "editor-mcp-save" if save == "on" else "editor-mcp-without-save"
        suite(name, [dotnet, tests, "--editor", host, project_copy(f"editor save {save}"), mcp,
                     run / f"captures editor {save}", None, f"save={save}", *selection], 300)
    suite("editor-pipe-and-faults", [dotnet, staged["channelTests"], staged["writeProbe"], run / "fault projects",
                                    None, *selection], 300)
    if args.include_committed_recovery:
        suite("editor-committed-recovery", [dotnet, staged["channelTests"], "--committed-recovery",
                                            run / "committed recovery projects", None, *selection], 300)
    projects = run / "projetos ação"
    suite("editor-mcp-physics", [dotnet, tests, "--editor-physics", editor, empty_project(projects / "física sem código"),
                                 empty_project(projects / "segunda sessão"), legacy_project(projects / "legado v1", legacy_copy),
                                 mcp, run / "captures physics", None, *selection], 600)
    try:
        records.append(orphan_case(run, env, dotnet, powershell, mcp, player, project_copy("orphan"), selection))
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        records.append({"suite": "server-killed-closes-host", "status": "failed", "error": str(error)})
    if cold is not None:
        empty = projects / "not a project"
        empty.mkdir()
        suite("editor-mcp-cold-code", [dotnet, tests, "--editor-code", editor, project_copy("Cold Code", cold[0]),
                                       mcp, run / "captures code", None, empty, project_copy("Deadline Code", cold[1]), *selection], 3600)
    write_json(run / "results.json", report)
    verify_inventory(sdk_root, sdk_files)
    for role, (_, files, _) in artifacts.items():
        verify_inventory(run / "artifacts" / role, files)
    report["inputsReverified"] = True
    write_json(run / "results.json", report)
    failed = [record["suite"] for record in records if record["status"] == "failed"]
    if failed:
        print(f"FAIL: {', '.join(failed)}. Evidence: {run}")
        return 1
    print(f"PASS: {sum(record.get('cases', 0) for record in records)} private qualification cases. Evidence: {run}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, KeyError, TypeError, ValueError) as error:
        raise SystemExit(f"Qualification refused: {error}") from None
