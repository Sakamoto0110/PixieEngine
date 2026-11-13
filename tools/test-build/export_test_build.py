"""Build a private Pixie test bundle using an explicitly selected installed SDK."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts" / "sdk"))
from verify_sdk import SYSTEM_DLLS, payload_path, pe_dependencies, read_json, verify


TUPLE = "profiles/local-development/Release"
MANIFEST = "pixie-test-build.json"
ASSET_FIELDS = {"Sprite": "image", "NormalMap2D": "image", "Text": "font"}
FRAME_ID = re.compile(r"[0-9a-f]{32}")
PNG_HEADER = b"\x89PNG\r\n\x1a\n\0\0\0\rIHDR"


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def unlinked(path: Path) -> None:
    for part in (path, *path.parents):
        try:
            attributes = part.lstat().st_file_attributes
        except FileNotFoundError:
            continue
        if attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
            raise ValueError(f"Linked paths are not supported: {part}")


def relative_path(root: Path, value: str) -> Path:
    path = payload_path(root, value)
    for part in PurePosixPath(value).parts:
        stem = part.split(".", 1)[0].rstrip().upper()
        if (any(char in part for char in '<>"|?*\x7f') or stem in {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"}
                or (stem.startswith(("COM", "LPT")) and len(stem) == 4 and stem[3] in "123456789¹²³")):
            raise ValueError(f"Invalid Windows path component: {part}")
    unlinked(path)
    return path


def run_cmake(cmake: Path, *arguments: str) -> None:
    subprocess.run([str(cmake), *arguments], check=True)


def build_player(cmake: Path, sdk: Path, profile: dict, project: Path, work: Path) -> tuple[Path, dict]:
    build = work / "build"
    binaries = work / "bin"
    run_cmake(cmake, "-S", str(project), "-B", str(build), "-G", "Visual Studio 18 2026", "-A",
              f"x64,version={profile['windowsSdkVersion']}", "-DCMAKE_CONFIGURATION_TYPES=Release",
              "-DPIXIE_ENGINE_MODE=PACKAGE", "-DPIXIE_PROJECT_BUILD_ROLE=authoring",
              "-DPIXIE_SDK_PROFILE=local-development", f"-DPIXIE_SDK_ROOT={sdk}",
              f"-DPIXIE_OUTPUT_ROOT={binaries}")
    declarations = list((build / "pixie-export" / "Release").glob("*.json"))
    if len(declarations) != 1:
        raise ValueError("A code project must declare exactly one Release player with pixie_game")
    declaration = declarations[0]
    target = declaration.stem
    if not target or target.startswith("-") or any(not ch.isascii() or not (ch.isalnum() or ch in "_.+-") for ch in target):
        raise ValueError("Invalid player target name")
    product = read_json(declaration)
    if (product["format"], product["version"], product["configuration"], product["development"], product["dev_channel"]) != (
        "pixie-product", 2, "Release", True, True
    ):
        raise ValueError("The configured product must be a private development player")
    name = product["player"]["destination"]
    if not name.lower().endswith(".exe") or PurePosixPath(name).name != name:
        raise ValueError("Invalid player executable basename")
    player = relative_path(binaries / "Release", name)
    if player != Path(product["player"]["source"]):
        raise ValueError("The product player must be in this build's Release output")
    run_cmake(cmake, "--build", str(build), "--config", "Release", "--target", target)
    unlinked(player)
    if read_json(declaration) != product:
        raise ValueError("Player declaration changed during the build")
    return player, product


def valid_frame_id(value: object) -> bool:
    return isinstance(value, str) and FRAME_ID.fullmatch(value) is not None and value != "0" * 32


def atlas_dependencies(project: Path, atlas: str) -> tuple[str, set[str]]:
    path = relative_path(project, atlas)
    if not path.name.lower().endswith(".atlas.json") or len(path.name) <= len(".atlas.json"):
        raise ValueError(f"Expected a .atlas.json descriptor: {atlas}")
    descriptor = read_json(path)
    if (not isinstance(descriptor, dict) or descriptor.get("format") != "pixie-sprite-atlas"
            or type(descriptor.get("version")) is not int or descriptor["version"] != 1
            or not isinstance(descriptor.get("frames"), list)):
        raise ValueError(f"Invalid sprite atlas descriptor: {atlas}")

    image = descriptor.get("image")
    image_path = relative_path(project, image)
    if image_path.suffix.lower() != ".png":
        raise ValueError(f"Expected a project-relative PNG for {atlas}: {image}")
    with image_path.open("rb") as stream:
        header = stream.read(len(PNG_HEADER) + 8)
    if len(header) != len(PNG_HEADER) + 8 or not header.startswith(PNG_HEADER):
        raise ValueError(f"Invalid PNG header for {atlas}: {image}")
    width, height = struct.unpack_from(">II", header, len(PNG_HEADER))
    size = descriptor.get("imageSize")
    if (not isinstance(size, list) or len(size) != 2 or any(type(value) is not int for value in size)
            or size != [width, height] or not 0 < width <= 0x7FFFFFFF or not 0 < height <= 0x7FFFFFFF):
        raise ValueError(f"PNG dimensions differ from the atlas imageSize for {atlas}: {image}")

    ids: set[str] = set()
    for frame in descriptor["frames"]:
        frame_id = frame.get("id") if isinstance(frame, dict) else None
        if not valid_frame_id(frame_id) or frame_id in ids:
            raise ValueError(f"Invalid or duplicate frame ID in {atlas}: {frame_id!r}")
        name, rect, pivot = frame.get("name"), frame.get("rect"), frame.get("pivot")
        if not isinstance(name, str) or not name:
            raise ValueError(f"Missing frame name in {atlas}: {frame_id}")
        if (not isinstance(rect, list) or len(rect) != 4 or any(type(value) is not int for value in rect)
                or rect[0] < 0 or rect[1] < 0 or not 0 < rect[2] <= width - rect[0]
                or not 0 < rect[3] <= height - rect[1]):
            raise ValueError(f"Frame rectangle is outside the PNG in {atlas}: {frame_id}")
        if (not isinstance(pivot, list) or len(pivot) != 2
                or any(type(value) not in (int, float) or not 0 <= value <= 1 for value in pivot)):
            raise ValueError(f"Invalid frame pivot in {atlas}: {frame_id}")
        ids.add(frame_id)
    return image, ids


def project_files(project: Path, assets: list[str]) -> tuple[str, set[str], set[str]]:
    projects = list(project.glob("*.pixie"))
    if len(projects) != 1 or not projects[0].is_file():
        raise ValueError("The project folder must contain one .pixie file")
    project_file = projects[0].name
    metadata = read_json(relative_path(project, project_file))
    if metadata.get("format") != "pixie-project" or metadata.get("version") != 1:
        raise ValueError("Unsupported .pixie project format")
    scenes = {metadata["startScene"]}
    unlinked(project / "scenes")
    for scene in (project / "scenes").rglob("*.scene"):
        scenes.add(scene.relative_to(project).as_posix())
    assets = set(assets)
    atlases: dict[str, tuple[str, set[str]]] = {}
    for name in sorted(scenes):
        if not name.endswith(".scene"):
            raise ValueError(f"Invalid scene path: {name}")
        scene = read_json(relative_path(project, name))
        for entity in scene["entities"]:
            components = entity.get("components", {})
            for component, field in ASSET_FIELDS.items():
                values = components.get(component, {})
                if component == "Sprite":
                    atlas, frame_id = values.get("atlas", ""), values.get("frameId", "")
                    if atlas != "" or frame_id != "":
                        if not isinstance(atlas, str) or not atlas or not valid_frame_id(frame_id):
                            raise ValueError(f"Invalid atlas reference in {name}: {atlas!r}, {frame_id!r}")
                        if atlas not in atlases:
                            atlases[atlas] = atlas_dependencies(project, atlas)
                        image, ids = atlases[atlas]
                        if frame_id not in ids:
                            raise ValueError(f"Atlas {atlas} has no frame {frame_id} referenced by {name}")
                        assets.update((atlas, image))
                        continue
                if values.get(field):
                    assets.add(values[field])
    return project_file, scenes, assets


def bundle(args: argparse.Namespace) -> Path:
    sdk, project, cmake, output = (path.absolute() for path in (args.sdk, args.project, args.cmake, args.output))
    for path in (sdk, project, cmake, output):
        unlinked(path)
    sdk, project, cmake = (path.resolve(strict=True) for path in (sdk, project, cmake))
    output = output.resolve()
    if output.exists() or any(output.is_relative_to(root) or root.is_relative_to(output) for root in (sdk, project)):
        raise ValueError("Choose a new output directory outside both the project and SDK")
    if cmake.name.lower() != "cmake.exe" or not cmake.is_file():
        raise ValueError("Select the consumer's cmake.exe")
    if read_json(relative_path(sdk, "sdk.json")).get("distributionClass") != "local-private":
        raise ValueError("Test export requires a local-private SDK")
    sdk_identity = verify(sdk, query=True)
    if sdk_identity["distributionClass"] != "local-private":
        raise ValueError("Test export requires a local-private SDK")
    profile = next((item for item in sdk_identity["profiles"] if item["configuration"] == "Release"), None)
    if profile is None:
        raise ValueError("The selected SDK has no local-development/Release profile")
    if profile["externalRuntimeDependencies"]:
        raise ValueError("The selected Release SDK must contain its native runtime dependencies")

    source = project / "CMakeLists.txt"
    unlinked(source)
    has_code = source.is_file()
    if has_code:
        requirement = read_json(relative_path(project, ".pixie-code/sdk.json"))
        if requirement.get("profile") != "local-development" or requirement.get("sdkId") != profile["sdkId"]:
            raise ValueError("Adopt this private SDK in a disposable project copy before exporting code")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pxw-", dir=output.parent) as scratch:
        work = Path(scratch)
        staged = work / "bundle"
        staged.mkdir()
        if has_code:
            player, product = build_player(cmake, sdk, profile, project, work)
        else:
            player = relative_path(sdk, profile["player"]["path"])
            product = {"assets": [], "dependencies": []}

        project_file, scenes, assets = project_files(project, [*product["assets"], *args.asset])
        records: dict[str, dict] = {}

        def copy(source: Path, name: str, role: str, expected_hash: str | None = None) -> None:
            unlinked(source)
            destination = relative_path(staged, name)
            if name.casefold() == MANIFEST or name.casefold() in records:
                raise ValueError(f"Duplicate or reserved bundle path: {name}")
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            actual_hash = digest(destination)
            if expected_hash is not None and actual_hash != expected_hash:
                raise ValueError(f"Copied bytes differ from the selected SDK: {name}")
            records[name.casefold()] = {"path": name, "role": role, "sha256": actual_hash, "size": destination.stat().st_size}

        copy(player, player.name, "player", None if has_code else profile["player"]["sha256"])
        for item in (profile["runtime"], *profile["nativeDependencies"]):
            path = relative_path(sdk, item["path"])
            if path.parent != sdk / TUPLE / "bin":
                raise ValueError("SDK DLL is outside the selected Release tuple")
            copy(path, path.name, "runtime", item["sha256"])
        for library in product["dependencies"]:
            path = Path(library["source"])
            name = library["destination"]
            if (not path.is_absolute() or path.suffix.lower() != ".dll" or not name.lower().endswith(".dll")
                    or PurePosixPath(name).name != name or name.lower().endswith(("-module.dll", "-capi.dll"))):
                raise ValueError("Private game libraries must be explicitly declared standalone DLLs")
            copy(path, name, "library")

        player_bytes = (staged / player.name).read_bytes()
        if b"--dev-channel\0" not in player_bytes:
            raise ValueError("The selected player has no Dev Channel launch option")
        for record in records.values():
            imports, _ = pe_dependencies(staged / record["path"])
            missing = {name for name in imports - records.keys() - SYSTEM_DLLS
                       if not name.startswith(("api-ms-win-", "ext-ms-win-"))}
            if missing:
                raise ValueError(f"Unresolved native dependencies for {record['path']}: {sorted(missing)}")

        copy(relative_path(project, project_file), project_file, "project")
        for name in sorted(scenes):
            copy(relative_path(project, name), name, "scene")
        for name in sorted(assets):
            path = relative_path(project, name)
            if path.suffix.lower() in {".exe", ".dll", ".lib", ".obj", ".pdb", ".ps1", ".cmd", ".bat", ".py", ".cpp", ".hpp", ".cs"}:
                raise ValueError(f"Build artifacts and executable source are not project assets: {name}")
            copy(path, name, "asset")
        for notice in sorted((sdk / "notices").rglob("*")):
            if notice.is_file():
                copy(notice, notice.relative_to(sdk).as_posix(), "notice")

        # Keep a relocatable launch recipe with the handoff. The receiving machine supplies
        # the matching SDK and bridge explicitly.
        recipe = work / "README-test-build.txt"
        recipe.write_text(
            "Private Pixie test build for local use.\n\n"
            "Use the same private SDK catalog on the receiving machine and a local PixieMcp build.\n"
            "Run from an MCP client using the stdio command below. Replace the paths in angle brackets.\n"
            'dotnet "<PixieMcp.dll>" --test-build "<bundle>/pixie-test-build.json" '
            '--sdk "<private SDK>" --captures "<folder outside the bundle>"\n\n'
            f"SDK catalog SHA-256: {sdk_identity['manifestSha256']}\n"
            f"SDK Release build ID: {profile['sdkBuildId']}\n"
            "The standalone player opens its Dev Channel only when launched with --dev-channel.\n",
            encoding="utf-8",
        )
        copy(recipe, recipe.name, "notice")

        manifest = {
            "format": "pixie-test-build", "version": 1, "distributionClass": "local-private",
            "profile": "local-development", "configuration": "Release", "development": True, "dev_channel": True,
            "sdkId": profile["sdkId"], "sdkBuildId": profile["sdkBuildId"], "sdkManifestSha256": sdk_identity["manifestSha256"],
            "player": player.name, "project": project_file, "files": list(records.values()),
        }
        (staged / MANIFEST).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        staged.rename(output)
    return output / MANIFEST


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--cmake", type=Path, required=True)
    parser.add_argument("--asset", action="append", default=[], help="Include an additional project-relative data file")
    parser.add_argument("project", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        if sys.platform != "win32":
            raise ValueError("Private test export currently supports Windows x64")
        print(f"Test-build manifest: {bundle(args)}")
        return 0
    except (KeyError, TypeError, ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"Test export failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
