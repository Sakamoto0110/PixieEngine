"""Build and verify public Windows x64 Engine release candidates locally."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from rehearse_sdk import (VERSION, absolute_path, archive_tree, check_tree, digest,
                         inventory, read_json, require, verify_and_relocate, write_json)

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "sdk"))
from assemble_sdk import compose, inventory as sdk_inventory, write_json as write_catalog
from verify_sdk import payload_path, verify


def build_sdk(source: Path, work: Path) -> Path:
    units = []
    for profile, configurations in (("authoring", ("Debug", "Release")), ("shipping", ("Release",))):
        build = work / f"build-{profile}"
        subprocess.run([
            "cmake", "-S", source, "-B", build, "-G", "Ninja Multi-Config",
            f"-DCMAKE_CONFIGURATION_TYPES={';'.join(configurations)}",
            f"-DPIXIE_SDK_PROFILE={profile}", f"-DPIXIE_OUTPUT_ROOT={(work / f'bin-{profile}').as_posix()}",
            f"-DPIXIE_DEVELOPMENT={'ON' if profile == 'authoring' else 'OFF'}",
            "-DPIXIE_DEV_CHANNEL=OFF", "-DPIXIE_BUILD_SAMPLES=OFF", "-DPIXIE_ENGINE_BUILD_TESTS=OFF",
            "-DPIXIE_BUILD_TOOLS=OFF", "-DPIXIE_SDK_INSTALL=ON", f"-DPython3_EXECUTABLE={Path(sys.executable).as_posix()}",
        ], check=True)
        for configuration in configurations:
            unit = work / f"unit-{profile}-{configuration}"
            subprocess.run(["cmake", "--build", build, "--config", configuration, "--parallel", "--target",
                            "pixie-player", "pixie_shared", "pixie_math", "glfw"], check=True)
            subprocess.run(["cmake", "--install", build, "--config", configuration, "--prefix", unit,
                            "--component", "PixieSDK"], check=True)
            units.append(unit)
    sdk = work / "sdk"
    compose(units, sdk, complete=True)
    return sdk


def stage_file(sdk: Path, record: dict, directory: Path) -> dict:
    source = payload_path(sdk, record["path"])
    destination = directory / source.name
    with source.open("rb") as contents, destination.open("xb") as staged:
        shutil.copyfileobj(contents, staged)
    require(digest(destination) == record["sha256"], f"Staged payload hash mismatch: {destination}")
    return {"path": destination.name, "sha256": record["sha256"]}


def pair_exporter(source: Path, work: Path, sdk: Path, catalog: dict, shipping: dict) -> None:
    build = work / "build-exporter"
    subprocess.run([
        "cmake", "-S", source / "tools/export", "-B", build, "-G", "Ninja Multi-Config",
        "-DCMAKE_CONFIGURATION_TYPES=Release", f"-DPIXIE_SDK_ROOT={sdk.as_posix()}", "-DPIXIE_SDK_PROFILE=shipping",
        f"-DPIXIE_EXPORT_JSON_INCLUDE_DIR={(source / 'external/nlohmann_json/single_include').as_posix()}",
    ], check=True)
    subprocess.run(["cmake", "--build", build, "--config", "Release", "--parallel", "--target", "pixie-export"],
                   check=True)
    relative = "tools/export/Release"
    directory = sdk / relative
    directory.mkdir(parents=True)
    executable = directory / "pixie-export.exe"
    shutil.copyfile(build / "Release" / executable.name, executable)

    def stage(record):
        result = stage_file(sdk, record, directory)
        result["path"] = f"{relative}/{result['path']}"
        return result

    dependencies = []
    for record in shipping["nativeDependencies"]:
        dependency = stage(record)
        name = Path(dependency["path"]).name.lower()
        dependency["kind"] = "crt" if name.startswith(("msvcp", "vcruntime", "concrt")) else "native"
        dependencies.append(dependency)
    catalog["tools"]["exporter"] = {
        "schemaVersion": 1, "path": f"{relative}/{executable.name}", "sha256": digest(executable),
        "toolVersion": catalog["productVersion"], "sdkId": shipping["sdkId"],
        "sdkBuildId": shipping["sdkBuildId"], "profile": "shipping", "configuration": "Release",
        "runtime": stage(shipping["runtime"]), "dependencies": dependencies,
        "descriptorSchema": 1, "buildProductSchema": 2, "exportManifestSchema": 2,
    }
    catalog["features"]["pairedExport"] = True
    catalog["files"] = sdk_inventory(sdk)
    write_catalog(sdk / "sdk.json", catalog)


def stage_runtime(work: Path, sdk: Path, catalog: dict, shipping: dict) -> Path:
    runtime = work / "runtime"
    runtime.mkdir()
    for record in [shipping["runtime"], shipping["player"], *shipping["nativeDependencies"]]:
        stage_file(sdk, record, runtime)
    for record in catalog["files"]:
        if record["role"] in ("notice", "provenance"):
            source = payload_path(sdk, record["path"])
            destination = runtime / record["path"]
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            require(digest(destination) == record["sha256"], f"Runtime notice or provenance drift: {destination}")
    write_json(runtime / "runtime.json", {
        "schemaVersion": 1, "sdkCatalogSha256": digest(sdk / "sdk.json"),
        **{key: shipping[key] for key in ("sdkId", "sdkBuildId", "productVersion", "platform", "architecture",
                                         "profile", "configuration", "systemPrerequisites")},
        "files": sdk_inventory(runtime),
    })
    return runtime


def pack(root: Path, files: dict, archive: Path, relocated: Path) -> dict:
    check_tree(root, files)
    archive_tree(root, files, archive)
    verify_and_relocate(archive, files, relocated)
    return {"name": archive.name, "sha256": digest(archive), "size": archive.stat().st_size}


def run_json(arguments: list, cwd: Path) -> dict:
    environment = {key: value for key, value in os.environ.items()
                   if key.upper() in ("SYSTEMROOT", "WINDIR", "COMSPEC", "TEMP", "TMP")}
    environment["PATH"] = ""
    output = subprocess.check_output(arguments, cwd=cwd, env=environment, timeout=90)
    return json.loads(output)


def execute(source: Path, work: Path) -> None:
    def git(*arguments):
        return subprocess.check_output(["git", "-C", source, *arguments], text=True).strip()

    commit = git("rev-parse", "HEAD")
    require(not git("status", "--porcelain", "--untracked-files=normal"), "Prepare a reviewed clean checkout first")
    version = read_json(source / "version.json")["version"]
    require(isinstance(version, str) and VERSION.fullmatch(version), "Invalid Engine release version")
    require(not work.exists() and work.parent.is_dir() and not work.is_relative_to(source)
            and not source.is_relative_to(work), "Choose a new work directory outside the checkout")
    work.mkdir()
    sdk = build_sdk(source, work)
    catalog = read_json(sdk / "sdk.json")
    require(catalog["productVersion"] == version and catalog["distributionClass"] == "public",
            "Expected the public SDK for this Engine version")
    shipping = read_json(sdk / "profiles/shipping/Release/profile.json")
    pair_exporter(source, work, sdk, catalog, shipping)
    verify(sdk, complete=True)
    runtime = stage_runtime(work, sdk, catalog, shipping)

    assets = work / "assets"
    assets.mkdir()
    sdk_copy, runtime_copy = work / "check-sdk", work / "check-runtime"
    sdk_zip = pack(sdk, inventory(sdk, catalog), assets / f"PixieEngine-SDK-{version}-windows-x64.zip", sdk_copy)
    runtime_files = {entry["path"]: (entry["size"], entry["sha256"]) for entry in sdk_inventory(runtime)}
    runtime_zip = pack(runtime, runtime_files, assets / f"PixieEngine-Runtime-{version}-windows-x64.zip", runtime_copy)

    verify(sdk_copy, complete=True)
    tool = catalog["tools"]["exporter"]
    identity = run_json([sdk_copy / tool["path"], "--sdk", sdk_copy, "--identity"], work)
    require(identity["sdkId"] == shipping["sdkId"] and identity["sdkBuildId"] == shipping["sdkBuildId"]
            and identity["executableSha256"] == tool["sha256"]
            and identity["runtimeSha256"] == shipping["runtime"]["sha256"], "Relocated exporter pairing mismatch")
    standalone = run_json([sys.executable, "-B", sdk_copy / "tools/sdk/verify_sdk.py", "--identity-only",
                           runtime_copy / "pixie.dll"], work)
    require(standalone["queriedCapabilityBits"] == 4 and all(standalone[key] == shipping[key]
            for key in ("sdkId", "sdkBuildId", "productVersion", "profile", "configuration")),
            "Relocated runtime is not the matching shipping artifact")
    require(git("rev-parse", "HEAD") == commit and not git("status", "--porcelain", "--untracked-files=normal"),
            "Producer checkout changed during candidate preparation")

    archives = [sdk_zip, runtime_zip]
    (assets / "SHA256SUMS.txt").write_text("".join(f"{entry['sha256']}  {entry['name']}\n" for entry in archives),
                                         encoding="ascii", newline="\n")
    write_json(assets / "release.json", {"schemaVersion": 1, "productVersion": version, "sourceCommit": commit,
                                        "sdkCatalogSha256": digest(sdk / "sdk.json"), "sdkId": catalog["sdkId"],
                                        "shippingBuildId": shipping["sdkBuildId"], "archives": archives})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, help="Absolute path to a new work directory outside the checkout")
    args = parser.parse_args()
    require(os.name == "nt", "This producer requires Windows x64 MSVC")
    execute(absolute_path(Path(__file__).resolve().parents[2]), absolute_path(args.out))


if __name__ == "__main__":
    main()
