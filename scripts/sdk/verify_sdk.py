"""Validate an installed Pixie SDK. Package mode never searches producer sources."""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path, PurePosixPath

NATIVE_FIELDS = (
    "schemaVersion", "sdkId", "sdkBuildId", "cApiVersion", "moduleIdentityContract", "platform",
    "architecture", "compilerId", "compilerVersion", "msvcFullVersion", "stlVersion", "stlUpdate",
    "iteratorDebugLevel", "cppStandard", "configuration", "crt", "cpuFeatures", "profile",
    "distributionClass", "exceptions", "rtti", "windowsSdkVersion", "dependencyInventorySha256",
)
MODULE_MARKERS = {"pixie_game_api_version", "pixie_game_create", "pixie_game_candidate_contract", "pixie_game_sdk_identity"}
SYSTEM_DLLS = {
    "kernel32.dll", "kernelbase.dll", "ntdll.dll", "user32.dll", "gdi32.dll", "opengl32.dll",
    "shell32.dll", "ole32.dll", "oleaut32.dll", "advapi32.dll", "shlwapi.dll", "msvcrt.dll",
    "ucrtbase.dll", "bcrypt.dll", "version.dll", "imm32.dll", "dwmapi.dll", "shcore.dll",
    "setupapi.dll", "comdlg32.dll", "winmm.dll", "ws2_32.dll", "winspool.drv", "comctl32.dll",
    "cfgmgr32.dll", "sechost.dll", "rpcrt4.dll", "crypt32.dll", "normaliz.dll", "wintrust.dll",
}
HEX = re.compile(r"^[0-9a-f]{64}$")
GENERATOR_SUPPORT = {
    "cmake/PixieAddCode.cmake", "cmake/PixieComponentRegistry.cmake", "cmake/PixieGame.cmake",
    "cmake/PixieInitComponents.cmake", "cmake/PixieProject.cmake", "cmake/PixieTargetDefaults.cmake",
    "support/player/PlayerMain.cpp", "support/player/SceneGame.cpp", "support/module/GameModuleEntry.cpp",
    "templates/project-code/versions.json",
}
_REGISTRY_TEMPLATES = {"Components.cmake.in", "Components.hpp.in", "Components.cpp.in", "Component.hpp.in", "Component.cpp.in"}
_PROJECT_TEMPLATES = {"Project.hpp.in", "Project.cpp.in"}
TEMPLATE_FAMILIES = {
    "legacy-v0": (0, {"legacy/CMakeLists.txt.in", "legacy/Project.hpp.in", "legacy/Project.cpp.in"}),
    "registered-v1": (1, _REGISTRY_TEMPLATES | _PROJECT_TEMPLATES | {"CMakeLists.txt.in"}),
    "sdk-v1": (1, _REGISTRY_TEMPLATES | _PROJECT_TEMPLATES | {"sdk/CMakeLists.txt.in"}),
    "custom-v1": (1, _REGISTRY_TEMPLATES),
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=_object)


def payload_path(root: Path, value: str) -> Path:
    if not isinstance(value, str) or not value or "\\" in value or ":" in value or any(ord(c) < 32 for c in value):
        raise ValueError(f"Invalid SDK-relative path: {value!r}")
    relative = PurePosixPath(value)
    if relative.is_absolute() or relative.as_posix() != value or any(part in (".", "..") or part.endswith((".", " ")) for part in relative.parts):
        raise ValueError(f"Noncanonical SDK-relative path: {value!r}")
    path = root.joinpath(*relative.parts)
    for parent in [path, *path.parents]:
        if parent == root:
            break
        if parent.is_symlink() or (hasattr(parent, "is_junction") and parent.is_junction()):
            raise ValueError(f"Linked SDK payload is not admitted: {value}")
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError(f"SDK path escapes its root: {value}")
    return path


def generation_ready(root: Path, files: set[str]) -> bool:
    if not GENERATOR_SUPPORT.issubset(files):
        return False
    prefix = "templates/project-code/"
    version = read_json(payload_path(root, prefix + "versions.json"))
    if type(version["schemaVersion"]) is not int or version["schemaVersion"] != 1 or version["hashPolicy"] != "utf8-lf":
        raise ValueError("Unsupported installed generator template inventory")
    if set(version["families"]) != set(TEMPLATE_FAMILIES):
        raise ValueError("Installed generator family inventory does not match its supported templates")
    templates = set()
    for family, (schema, paths) in TEMPLATE_FAMILIES.items():
        entry = version["families"][family]
        if type(entry["registrySchema"]) is not int or entry["registrySchema"] != schema or sorted(entry["templates"]) != sorted(paths):
            raise ValueError(f"Invalid installed generator family: {family}")
        templates.update(paths)
    if set(version["sha256"]) != templates:
        raise ValueError("Incomplete installed template hash inventory")
    for relative in templates:
        if prefix + relative not in files:
            raise ValueError(f"SDK omits an installed generator template: {relative}")
        canonical = payload_path(root, prefix + relative).read_bytes().replace(b"\r\n", b"\n")
        canonical.decode("utf-8")
        if hashlib.sha256(canonical).hexdigest() != version["sha256"][relative]:
            raise ValueError(f"Installed generator template digest mismatch: {relative}")
    return True


def pe_dependencies(path: Path) -> tuple[set[str], set[str]]:
    """Read bounded x64 PE import/export names; no execution or search-path resolution."""
    data = path.read_bytes()

    def read(fmt, offset):
        size = struct.calcsize(fmt)
        if offset < 0 or offset + size > len(data):
            raise ValueError(f"Truncated PE structure: {path.name}")
        return struct.unpack_from(fmt, data, offset)

    if data[:2] != b"MZ":
        raise ValueError(f"Not a PE payload: {path.name}")
    pe = read("<I", 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0" or read("<H", pe + 4)[0] != 0x8664:
        raise ValueError(f"SDK payload is not x64 PE: {path.name}")
    count = read("<H", pe + 6)[0]
    optional_size = read("<H", pe + 20)[0]
    optional = pe + 24
    if read("<H", optional)[0] != 0x20B or optional_size < 128:
        raise ValueError(f"Unsupported PE optional header: {path.name}")
    sections = []
    for index in range(count):
        offset = optional + optional_size + 40 * index
        virtual_size, virtual, raw_size, raw = read("<IIII", offset + 8)
        sections.append((virtual, max(virtual_size, raw_size), raw, raw_size))

    def rva(value):
        for virtual, size, raw, raw_size in sections:
            if virtual <= value < virtual + size and value - virtual < raw_size:
                return raw + value - virtual
        raise ValueError(f"Unmapped PE RVA in {path.name}")

    def text(value):
        offset = rva(value)
        end = data.find(b"\0", offset, min(offset + 4096, len(data)))
        if end < 0:
            raise ValueError(f"Unterminated PE name in {path.name}")
        return data[offset:end].decode("ascii")

    imports = set()
    import_rva, import_size = read("<II", optional + 120)
    if import_rva:
        offset = rva(import_rva)
        for index in range(min(import_size // 20 + 1, 4096)):
            descriptor = read("<IIIII", offset + 20 * index)
            if not any(descriptor):
                break
            imports.add(text(descriptor[3]).lower())
        else:
            raise ValueError(f"Unbounded PE import table: {path.name}")
    exports = set()
    export_rva, _ = read("<II", optional + 112)
    if export_rva:
        offset = rva(export_rva)
        names = read("<I", offset + 24)[0]
        table = rva(read("<I", offset + 32)[0]) if names else 0
        if names > 100000:
            raise ValueError(f"Unbounded PE exports: {path.name}")
        for index in range(names):
            exports.add(text(read("<I", table + 4 * index)[0]))
    return imports, exports


def runtime_identity(path: Path) -> dict:
    if os.name != "nt":
        raise ValueError("Native SDK query qualification requires Windows")
    with os.add_dll_directory(str(path.parent)):
        library = ctypes.CDLL(str(path))
        library.pixie_sdk_identity.argtypes = [ctypes.c_void_p, ctypes.c_int32]
        library.pixie_sdk_identity.restype = ctypes.c_int32
        library.pixie_capabilities.restype = ctypes.c_uint64
        size = library.pixie_sdk_identity(None, 0)
        if not 0 < size <= 65535:
            raise ValueError("Runtime returned an invalid identity length")
        buffer = ctypes.create_string_buffer(size + 1)
        if library.pixie_sdk_identity(buffer, size + 1) != size or buffer[size] != b"\0":
            raise ValueError("Runtime identity sizing/NUL contract failed")
        identity = json.loads(buffer.value, object_pairs_hook=_object)
        identity["queriedCapabilityBits"] = library.pixie_capabilities()
        return identity


def verify(root: Path, *, complete: bool = False, query: bool = False) -> dict:
    root = root.resolve()
    manifest = read_json(root / "sdk.json")
    if type(manifest.get("schemaVersion")) is not int or manifest["schemaVersion"] != 1 or not HEX.fullmatch(manifest.get("sdkId", "")):
        raise ValueError("Unsupported SDK manifest schema/identity")
    distribution = manifest.get("distributionClass")
    if distribution not in ("public", "local-private"):
        raise ValueError("Unknown SDK distribution class")
    inventory = {}
    folded = set()
    for item in manifest.get("files", []):
        relative = item["path"]
        path = payload_path(root, relative)
        if relative == "sdk.json" or relative in inventory or relative.casefold() in folded:
            raise ValueError(f"Duplicate/recursive/case-colliding SDK inventory: {relative}")
        if type(item.get("size")) is not int or item["size"] < 0 or not HEX.fullmatch(item.get("sha256", "")) or not isinstance(item.get("role"), str) or not item["role"]:
            raise ValueError(f"Invalid SDK file record: {relative}")
        if not path.is_file() or path.stat().st_size != item["size"] or digest(path) != item["sha256"]:
            raise ValueError(f"Missing or mixed SDK payload: {relative}")
        if distribution == "public":
            lower = relative.lower()
            if any(part in lower for part in ("devchannel/", "pixie-mcp", "local-development/", "localcommandchannel.hpp", "pixie/events/")):
                raise ValueError(f"Private/unsupported payload in public SDK: {relative}")
            if path.suffix in (".h", ".hpp", ".cpp", ".cs") and any(marker in path.read_bytes() for marker in (b"pixie_dev_dispatch", b"--dev-channel", b"--dev-capture-dir")):
                raise ValueError(f"Local automation source in public SDK: {relative}")
        inventory[relative] = item
        folded.add(relative.casefold())
    actual = set()
    for path in root.rglob("*"):
        relative = path.relative_to(root).as_posix()
        payload_path(root, relative)
        if path.is_file() and relative != "sdk.json":
            actual.add(relative)
    if actual != set(inventory):
        raise ValueError(f"Undeclared/missing SDK files: {sorted(actual ^ set(inventory))[:10]}")
    generation = generation_ready(root, actual)
    authoring_available = any(entry["profile"] != "shipping" for entry in manifest["profiles"])
    if manifest["features"]["projectGeneration"] is not (generation and authoring_available):
        raise ValueError("SDK generator availability does not match its installed inputs")
    required_common = {"include/pixie_capi.h", "include/pixie/runtime/World.hpp", "include/pixie/runtime/Scene.hpp",
                       "include/pixie/runtime/Presentation2D.hpp", "include/pixie/runtime/ViewportCamera2D.hpp",
                       "third-party/include/pixie/math/pxMat.hpp", "lib/cmake/PixieEngine/PixieEngineConfig.cmake"}
    if not required_common.issubset(inventory):
        raise ValueError(f"SDK omits required public interface inputs: {sorted(required_common - set(inventory))}")
    seen = set()
    profiles = []
    for entry in manifest.get("profiles", []):
        name, configuration = entry["profile"], entry["configuration"]
        allowed = name in ("authoring", "shipping") if distribution == "public" else name == "local-development"
        if not allowed or configuration not in ("Debug", "Release") or (name == "shipping" and configuration != "Release"):
            raise ValueError("Profile/configuration is not an admitted distribution tuple")
        key = (name, configuration)
        if key in seen:
            raise ValueError("Duplicate SDK profile/configuration")
        seen.add(key)
        relative = f"profiles/{name}/{configuration}/profile.json"
        if entry["manifest"] != relative or relative not in inventory or entry["manifestSha256"] != inventory[relative]["sha256"]:
            raise ValueError("Profile manifest hash/path is inconsistent")
        profile = read_json(payload_path(root, relative))
        if profile["projectGenerationAvailable"] is not (generation and name != "shipping"):
            raise ValueError("Profile generator availability does not match its installed inputs and role")
        for field in NATIVE_FIELDS:
            if field not in profile:
                raise ValueError(f"Profile omits native identity field: {field}")
        if profile["sdkId"] != manifest["sdkId"] or profile["sdkBuildId"] != entry["sdkBuildId"] or not HEX.fullmatch(profile["sdkBuildId"]) or profile["profile"] != name or profile["configuration"] != configuration or profile["distributionClass"] != distribution:
            raise ValueError("Profile/root identity is mixed")
        expected_bits = 7 if name == "local-development" else 5 if name == "authoring" else 4
        capabilities = profile.get("capabilities")
        if not isinstance(capabilities, dict) or any(type(capabilities.get(field)) is not bool or capabilities[field] != bool(expected_bits & bit) for field, bit in (("moduleLoading", 1), ("nativeAutomation", 2), ("candidateAuthoring", 4))):
            raise ValueError("Profile native capabilities do not match the admitted artifact role")
        for role in ("runtime", "player"):
            record = profile[role]
            expected_path = f"profiles/{name}/{configuration}/bin/" + ("pixie.dll" if role == "runtime" else "pixie-player.exe")
            if record["path"] != expected_path:
                raise ValueError(f"Profile {role} points outside its exact tuple")
            if record["path"] not in inventory or record["sha256"] != inventory[record["path"]]["sha256"]:
                raise ValueError(f"Profile {role} payload identity is inconsistent")
        for suffix in ("include/pixie_sdk_build.h", "lib/pixie.lib", "lib/pixie_math.lib"):
            if f"profiles/{name}/{configuration}/{suffix}" not in inventory:
                raise ValueError(f"SDK tuple omits required link/header input: {suffix}")
        local_dlls = {PurePosixPath(item["path"]).name.lower() for item in profile.get("nativeDependencies", [])}
        local_dlls.add(PurePosixPath(profile["runtime"]["path"]).name.lower())
        for record in profile.get("nativeDependencies", []):
            if not record["path"].startswith(f"profiles/{name}/{configuration}/bin/"):
                raise ValueError("Native dependency points outside the selected tuple closure")
            if record["path"] not in inventory or record["sha256"] != inventory[record["path"]]["sha256"]:
                raise ValueError("Native dependency is absent/mixed")
        external = {item["name"].lower() for item in profile.get("externalRuntimeDependencies", [])}
        if configuration != "Debug" and external:
            raise ValueError("Release SDK has undeployed native runtime dependencies")
        for record in [profile["runtime"], profile["player"], *profile.get("nativeDependencies", [])]:
            path = payload_path(root, record["path"])
            imports, exports = pe_dependencies(path)
            missing = imports - local_dlls - SYSTEM_DLLS - external
            missing = {name for name in missing if not name.startswith(("api-ms-win-", "ext-ms-win-"))}
            if missing:
                raise ValueError(f"Undeclared native dependency for {record['path']}: {sorted(missing)}")
            if exports & MODULE_MARKERS:
                raise ValueError(f"Game module image in SDK runtime payload: {record['path']}")
            if distribution == "public" and ("pixie_dev_dispatch" in exports or b"--dev-channel" in path.read_bytes() or b"PIXIE_DEVELOPMENT_CHANNEL_V1" in path.read_bytes()):
                raise ValueError(f"Local automation native image in public SDK: {record['path']}")
        if query:
            process = subprocess.run([sys.executable, __file__, "--identity-only", str(payload_path(root, profile["runtime"]["path"]))], capture_output=True, text=True)
            if process.returncode:
                raise ValueError(f"Selected SDK runtime cannot load/query: {process.stderr.strip()}")
            native = json.loads(process.stdout)
            for field in NATIVE_FIELDS:
                if type(native.get(field)) is not type(profile[field]) or native[field] != profile[field]:
                    raise ValueError(f"Actual runtime/profile mismatch: {field}")
            if native["queriedCapabilityBits"] != expected_bits:
                raise ValueError("Actual runtime facilities differ from its profile")
        profiles.append(profile)
    if not seen:
        raise ValueError("SDK declares no available profiles")
    required = {("authoring", "Debug"), ("authoring", "Release"), ("shipping", "Release")} if distribution == "public" else {("local-development", "Debug"), ("local-development", "Release")}
    if complete and seen != required:
        raise ValueError(f"Incomplete distribution profile composition: {sorted(seen)}")
    return {"sdkId": manifest["sdkId"], "manifestSha256": digest(root / "sdk.json"), "distributionClass": distribution, "profileCount": len(profiles), "fileCount": len(inventory), "profiles": profiles}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path)
    parser.add_argument("--complete", action="store_true")
    parser.add_argument("--query-runtime", action="store_true")
    parser.add_argument("--identity-only", type=Path)
    args = parser.parse_args()
    if args.root is None and args.identity_only is None:
        parser.error("supply an SDK root or --identity-only runtime")
    try:
        result = runtime_identity(args.identity_only) if args.identity_only else verify(args.root, complete=args.complete, query=args.query_runtime)
        print(json.dumps(result, indent=2))
        return 0
    except (ValueError, OSError, KeyError, TypeError, AttributeError, struct.error) as error:
        print(f"SDK validation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
