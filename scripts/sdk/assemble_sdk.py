"""Finalize isolated install units and compose an immutable relocatable Pixie SDK."""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

# Installed tools must not create undeclared __pycache__ files in immutable SDKs.
sys.dont_write_bytecode = True
from verify_sdk import SYSTEM_DLLS, digest, generation_ready, payload_path, pe_dependencies, read_json, verify


def write_json(path: Path, value) -> None:
    path.write_text(json.dumps(value, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")


def strip_local_blocks(path: Path, guards: set[str]) -> None:
    """Derive public support from explicit inactive guards; never rewrite producer inputs."""
    if not path.is_file():
        return
    lines = path.read_text(encoding="utf-8-sig").splitlines(keepends=True)
    output = []
    depth = 0
    for line in lines:
        directive = line.strip()
        if not depth and directive in guards:
            depth = 1
        elif depth:
            if re.match(r"#\s*(if|ifdef|ifndef)\b", directive):
                depth += 1
            elif re.match(r"#\s*endif\b", directive):
                depth -= 1
            elif depth == 1 and re.match(r"#\s*(else|elif)\b", directive):
                raise ValueError(f"Public support derivation needs review for a new branch: {path.name}")
        else:
            output.append(line)
    if depth:
        raise ValueError(f"Unterminated private support guard: {path.name}")
    path.write_text("".join(output), encoding="utf-8", newline="\n")


def role(relative: str) -> str:
    if relative.endswith("/profile.json"):
        return "profile-manifest"
    if relative.startswith("notices/"):
        return "notice"
    if relative.startswith("provenance/"):
        return "provenance"
    if relative.startswith("bindings/"):
        return "interop-source"
    if relative.startswith("support/"):
        return "consumer-support"
    if relative.startswith(("include/", "third-party/include/")) or "/include/" in relative:
        return "public-header"
    if relative.endswith("/pixie.dll"):
        return "native-runtime"
    if relative.endswith("/pixie-player.exe"):
        return "no-code-player"
    if relative.endswith(".dll"):
        return "native-dependency"
    if relative.endswith(".lib"):
        return "native-link-input"
    return "sdk-build-input"


def inventory(root: Path) -> list[dict]:
    result = []
    for file in sorted(root.rglob("*")):
        if file.is_file() and file.relative_to(root).as_posix() != "sdk.json":
            relative = file.relative_to(root).as_posix()
            payload_path(root, relative)
            result.append({"path": relative, "size": file.stat().st_size, "sha256": digest(file), "role": role(relative)})
    return result


def root_manifest(root: Path, sdk_id: str, version: str, distribution: str, entries: list[dict]) -> None:
    binding = root / "bindings/dotnet/PixieCapi.cs"
    metadata_ready = binding.is_file() and "NativeApiVersion.Current" in binding.read_text(encoding="utf-8")
    files = inventory(root)
    generation = generation_ready(root, {record["path"] for record in files})
    write_json(root / "sdk.json", {
        "schemaVersion": 1, "sdkId": sdk_id, "productVersion": version,
        "distributionClass": distribution, "profiles": sorted(entries, key=lambda item: (item["profile"], item["configuration"])),
        "files": files, "tools": {},
        "features": {"nativeCpp": True, "noCodePlayer": True, "managedBindingInputs": metadata_ready,
                     "projectGeneration": generation and any(entry["profile"] != "shipping" for entry in entries),
                     "pairedExport": False},
        "qualification": "Intermediate SDK package; availability describes supplied inputs. No release or independent installed-consumer acceptance is implied.",
    })


def finalize(root: Path, identity_file: Path) -> None:
    root = root.resolve()
    if (root / ".git").exists() or (root / "CMakeLists.txt").exists():
        raise ValueError("An SDK install unit must not be a producer checkout")
    identity = read_json(identity_file)
    name, configuration = identity["profile"], identity["configuration"]
    distribution = identity["distributionClass"]
    if distribution == "public":
        strip_local_blocks(root / "include/pixie_capi.h", {"#ifdef PIXIE_DEV_CHANNEL"})
        strip_local_blocks(root / "bindings/dotnet/PixieCapi.cs", {"#if PIXIE_DEVELOPMENT"})
        strip_local_blocks(root / "support/player/PlayerMain.cpp", {"#ifdef PIXIE_DEV_CHANNEL"})
    prefix = f"profiles/{name}/{configuration}"
    binary = root / prefix / "bin"
    runtime = binary / "pixie.dll"
    player = binary / "pixie-player.exe"
    if not runtime.is_file() or not player.is_file():
        raise ValueError("Install must build/stage the actual runtime and generic player first")
    dependencies = [file for file in sorted(binary.glob("*.dll")) if file.name.lower() != "pixie.dll"]
    if not any(file.name.lower() == "glfw3.dll" for file in dependencies):
        raise ValueError("SDK profile omits its GLFW runtime dependency")
    supplied = {file.name.lower() for file in [runtime, *dependencies]}
    external = set()
    for image in [runtime, player, *dependencies]:
        imports, _ = pe_dependencies(image)
        for imported in imports - supplied - SYSTEM_DLLS:
            if imported.startswith(("api-ms-win-", "ext-ms-win-")):
                continue
            if configuration == "Debug" and re.fullmatch(r"(?:msvcp[0-9_]+d|vcruntime[0-9_]+d|concrt[0-9_]+d|ucrtbased)\.dll", imported):
                external.add(imported)
            else:
                raise ValueError(f"SDK install omitted native dependency {imported} needed by {image.name}")

    def record(path):
        return {"path": path.relative_to(root).as_posix(), "sha256": digest(path)}

    profile = dict(identity)
    generation = generation_ready(root, {file.relative_to(root).as_posix() for file in root.rglob("*") if file.is_file()})
    profile.update(runtime=record(runtime), player=record(player), nativeDependencies=[record(file) for file in dependencies],
        externalRuntimeDependencies=[{"name": name, "scope": "matching-developer-runtime", "compilerVersion": identity["compilerVersion"]} for name in sorted(external)],
        systemPrerequisites=["Windows x64", "OpenGL 4.5 core driver"],
        bindingPaths=["bindings/dotnet/PixieCapi.cs", "bindings/dotnet/NativeApiVersion.cs", "bindings/dotnet/NativeApiVersion.props"],
        projectGenerationAvailable=generation and name != "shipping", pairedExportAvailable=False)
    manifest_path = root / prefix / "profile.json"
    write_json(manifest_path, profile)
    entry = {"profile": identity["profile"], "configuration": configuration, "sdkBuildId": identity["sdkBuildId"],
             "manifest": manifest_path.relative_to(root).as_posix(), "manifestSha256": digest(manifest_path)}
    root_manifest(root, identity["sdkId"], identity["productVersion"], distribution, [entry])
    verify(root)


# Each unit declares one profile/configuration. Shared payload paths must match byte for byte.
def compose(units: list[Path], output: Path, complete: bool) -> None:
    if output.exists():
        raise ValueError("SDK composition requires a new output directory; existing candidates are immutable")
    manifests = []
    for unit in units:
        verify(unit)
        manifests.append(read_json(unit / "sdk.json"))
    if not manifests:
        raise ValueError("No SDK install units supplied")
    common = {(item["sdkId"], item["productVersion"], item["distributionClass"]) for item in manifests}
    if len(common) != 1:
        raise ValueError("Cannot mix SDK build inputs, product versions or public/private distributions")
    # Preflight all shared bytes and profile uniqueness before creating the output.
    files = {}
    profiles = {}
    for unit, manifest in zip(units, manifests, strict=True):
        for item in manifest["files"]:
            prior = files.get(item["path"].casefold())
            if prior and (prior[1]["path"] != item["path"] or prior[1]["sha256"] != item["sha256"]):
                raise ValueError(f"Conflicting SDK common inputs: {item['path']}")
            files[item["path"].casefold()] = (unit, item)
        for entry in manifest["profiles"]:
            key = (entry["profile"], entry["configuration"])
            if key in profiles:
                raise ValueError(f"Duplicate composed SDK tuple: {key}")
            profiles[key] = entry
    output.mkdir(parents=True)
    for unit, item in files.values():
        target = payload_path(output, item["path"])
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(unit / item["path"], target)
    sdk_id, version, distribution = next(iter(common))
    root_manifest(output, sdk_id, version, distribution, list(profiles.values()))
    verify(output, complete=complete)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    unit = sub.add_parser("finalize-unit")
    unit.add_argument("--root", required=True, type=Path)
    unit.add_argument("--identity", required=True, type=Path)
    assembly = sub.add_parser("compose")
    assembly.add_argument("--unit", action="append", required=True, type=Path)
    assembly.add_argument("--output", required=True, type=Path)
    assembly.add_argument("--allow-partial", action="store_true")
    args = parser.parse_args()
    try:
        if args.operation == "finalize-unit":
            finalize(args.root, args.identity)
        else:
            compose(args.unit, args.output, not args.allow_partial)
        return 0
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(f"SDK assembly refused: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
