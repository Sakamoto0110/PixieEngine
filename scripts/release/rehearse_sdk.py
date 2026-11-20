"""Manually assemble and verify one public Windows SDK rehearsal locally."""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import sys
import zipfile
import zlib

HASH = re.compile(r"[0-9a-f]{64}")
VERSION = re.compile(r"[0-9]+(?:\.[0-9]+){3}")
WINDOWS_DEVICE_NAME = re.compile(r"(?:CON|PRN|AUX|NUL|CONIN\$|CONOUT\$|(?:COM|LPT)[1-9\u00b9\u00b2\u00b3])", re.IGNORECASE)
ZIP_TIME = (1980, 1, 1, 0, 0, 0)
REPARSE = 0x400
INPUT_SCHEMA = "pixie-manual-sdk-rehearsal/1"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path):
    return json.loads(path.read_bytes(), object_pairs_hook=unique_object)


def write_json(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=False)
        stream.write("\n")


def absolute_path(value):
    path = Path(value)
    require(path.is_absolute() and ".." not in path.parts, f"Expected an absolute path without traversal: {path}")
    require(re.fullmatch(r"[A-Za-z]:\\", path.anchor)
            and ctypes.windll.kernel32.GetDriveTypeW(ctypes.c_wchar_p(path.anchor)) == 3,
            f"Expected a local fixed drive: {path}")
    for component in (path, *path.parents):
        try:
            info = component.lstat()
        except FileNotFoundError:
            continue
        require(not info.st_file_attributes & REPARSE, f"Linked path is not allowed: {component}")
    return path.resolve()


def portable_name(name):
    path = PurePosixPath(name)
    require(path.parts and path.as_posix() == name and not path.is_absolute()
            and all(not re.search(r'[<>:"\\|?*\x00-\x1f]', part) and not part.endswith((".", " "))
                    and part not in (".", "..") and not WINDOWS_DEVICE_NAME.fullmatch(part.partition(".")[0].rstrip(" "))
                    for part in path.parts), f"Unsafe SDK payload path: {name}")
    return path


def pinned(reference):
    path = absolute_path(reference["path"])
    expected = reference["sha256"]
    require(isinstance(expected, str) and HASH.fullmatch(expected)
            and path.is_file() and digest(path) == expected, f"Missing or changed receipt: {path}")
    return path


def inventory(sdk, catalog):
    declared = {"sdk.json": (sdk.joinpath("sdk.json").stat().st_size, digest(sdk / "sdk.json"))}
    folded = {"sdk.json"}
    for entry in catalog["files"]:
        name, size, sha256 = entry["path"], entry["size"], entry["sha256"]
        portable_name(name)
        require(name.casefold() not in folded, f"Duplicate or case-colliding SDK payload: {name}")
        require(type(size) is int and size >= 0 and isinstance(sha256, str) and HASH.fullmatch(sha256),
                f"Invalid SDK payload record: {name}")
        folded.add(name.casefold())
        declared[name] = (size, sha256)
    return declared


def check_tree(sdk, declared):
    actual = set()
    for path in sdk.rglob("*"):
        name = path.relative_to(sdk).as_posix()
        checked = absolute_path(path)
        info = checked.lstat()
        if stat.S_ISREG(info.st_mode):
            require(name in declared and (info.st_size, digest(checked)) == declared[name],
                    f"Undeclared or changed SDK payload: {name}")
            actual.add(name)
        else:
            require(stat.S_ISDIR(info.st_mode), f"Special SDK path is not allowed: {name}")
    require(actual == declared.keys(), "SDK tree is missing declared payloads")


def archive_tree(sdk, declared, output):
    with output.open("xb") as stream, zipfile.ZipFile(stream, "w") as archive:
        for name in sorted(declared):
            data = absolute_path(sdk / name).read_bytes()
            require((len(data), hashlib.sha256(data).hexdigest()) == declared[name], f"Payload drift: {name}")
            member = zipfile.ZipInfo(name, ZIP_TIME)
            member.create_system = 3
            member.external_attr = 0o100644 << 16
            archive.writestr(member, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)


def verify_and_relocate(archive_path, declared, relocated):
    with zipfile.ZipFile(archive_path) as archive:
        require(archive.namelist() == sorted(declared), "Archive member inventory mismatch")
        for entry in archive.infolist():
            expected = declared[entry.filename]
            require(not entry.is_dir() and entry.file_size == expected[0]
                    and entry.external_attr >> 16 == 0o100644, f"Invalid ZIP member: {entry.filename}")
            with archive.open(entry) as source:
                require(hashlib.file_digest(source, "sha256").hexdigest() == expected[1],
                        f"Archive member digest mismatch: {entry.filename}")
            target = relocated.joinpath(*portable_name(entry.filename).parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(entry) as source, target.open("xb") as output:
                shutil.copyfileobj(source, output)
    check_tree(relocated, declared)


# Rehearse the pinned, already-produced SDK and exporter as relocated bytes.
def execute(input_path, out):
    inputs = read_json(input_path)
    require(inputs["schema"] == INPUT_SCHEMA, "Unsupported SDK rehearsal input schema")
    sdk = absolute_path(inputs["sdkRoot"])
    catalog_path = absolute_path(sdk / "sdk.json")
    catalog_hash = inputs["sdkCatalogSha256"]
    require(catalog_path.is_file() and digest(catalog_path) == catalog_hash, "SDK catalog pin did not match")
    catalog = read_json(catalog_path)
    version = catalog["productVersion"]
    require(catalog["distributionClass"] == "public" and isinstance(version, str) and VERSION.fullmatch(version),
            "Expected an independently versioned public SDK")
    declared = inventory(sdk, catalog)
    check_tree(sdk, declared)
    exporter = catalog["tools"]["exporter"]
    require(catalog["features"]["pairedExport"] is True, "SDK has no paired exporter")
    composition = pinned(inputs["producerComposition"])
    tool_seal = pinned(inputs["producerExporterSeal"])
    tool_entry = pinned(inputs["producerExporterEntry"])
    composition_receipt = read_json(composition)
    require(composition_receipt["sdkManifestSha256"] == catalog_hash and
            {digest(tool_seal), digest(tool_entry)} <= set(composition_receipt["inputs"].values()) and
            read_json(tool_entry) == exporter, "Producer receipts do not bind the selected SDK and exporter")
    notice_names = [entry["path"] for entry in catalog["files"] if entry["role"] == "notice"]
    lock_names = ("provenance/producer-inputs.tsv", "provenance/dependency-inputs.tsv")
    require(notice_names and all(name in declared for name in lock_names), "SDK notices or provenance are missing")
    require(out.parent.is_dir() and not out.exists()
            and all(not path.is_relative_to(out) and not out.is_relative_to(path)
                    for path in (sdk, input_path, composition, tool_seal, tool_entry)), "Choose a fresh separate output root")
    original_receipts = {path: digest(path) for path in (input_path, composition, tool_seal, tool_entry)}
    out.mkdir()
    logs = out / "logs"
    logs.mkdir()
    relocated = out / "Relocated SDK"
    archive_path = out / f"PixieEngine-SDK-{version}-windows-x64-local.zip"
    environment = {key: value for key, value in os.environ.items()
                   if key.upper() in ("SYSTEMROOT", "WINDIR", "COMSPEC")}
    environment.update(PATH="", TEMP=str(logs), TMP=str(logs), PYTHONDONTWRITEBYTECODE="1")
    result = {
        "schema": "pixie-manual-sdk-rehearsal-result/1", "status": "incomplete", "scope": "local Engine SDK only",
        "sdkCatalogSha256": catalog_hash, "sdkId": catalog["sdkId"], "productVersion": version,
        "inputs": [{"path": str(path), "sha256": sha256} for path, sha256 in original_receipts.items()],
        "notices": {name: declared[name][1] for name in notice_names},
        "inputInventories": {name: declared[name][1] for name in lock_names},
        "environment": environment, "tools": {"pythonSha256": digest(Path(sys.executable)),
                                             "pythonVersion": sys.version.split()[0], "zlib": zlib.ZLIB_RUNTIME_VERSION},
        "commands": [], "sourceUnavailable": False, "productGatePassed": False, "G6": "open",
    }

    def call(label, arguments):
        argv = list(map(str, arguments))
        stdout, stderr = logs / (label + ".stdout"), logs / (label + ".stderr")
        command = {"stage": label, "argv": argv, "cwd": str(out), "exitCode": None, "timeoutSeconds": 90}
        try:
            with stdout.open("xb") as output, stderr.open("xb") as errors:
                run = subprocess.run(argv, cwd=out, env=environment, stdout=output, stderr=errors,
                                     stdin=subprocess.DEVNULL, timeout=90)
            command["exitCode"] = run.returncode
        finally:
            command["stdoutSha256"] = digest(stdout) if stdout.is_file() else None
            command["stderrSha256"] = digest(stderr) if stderr.is_file() else None
            result["commands"].append(command)
        require(command["exitCode"] == 0, f"{label} refused; see {stderr}")
        return read_json(stdout)

    try:
        archive_tree(sdk, declared, archive_path)
        archive_sha256 = digest(archive_path)
        verify_and_relocate(archive_path, declared, relocated)
        verifier = relocated / "tools/sdk/verify_sdk.py"
        complete = call("verify-complete", [sys.executable, "-B", verifier, relocated, "--complete"])
        require(complete["manifestSha256"] == catalog_hash and complete["fileCount"] == len(declared) - 1,
                "Packaged verifier did not confirm the exact SDK")
        observed = call("paired-identity", [relocated / exporter["path"], "--sdk", relocated, "--identity"])
        require(observed["sdkId"] == catalog["sdkId"] and observed["sdkBuildId"] == exporter["sdkBuildId"] and
                observed["executableSha256"] == exporter["sha256"] and
                observed["runtimeSha256"] == exporter["runtime"]["sha256"] and
                observed["runtimeIdentity"]["capabilityBits"] == 4 and
                all(observed[key] == exporter[key] for key in
                    ("descriptorSchema", "buildProductSchema", "exportManifestSchema", "toolVersion")),
                "Relocated paired exporter identity mismatch")
        check_tree(sdk, declared)
        check_tree(relocated, declared)
        require(digest(archive_path) == archive_sha256, "Archive changed during verification")
        require(all(digest(path) == sha256 for path, sha256 in original_receipts.items()), "Input receipt drift")
        result["pairedIdentity"] = observed
        result["archive"] = {"name": archive_path.name, "size": archive_path.stat().st_size,
                             "sha256": archive_sha256, "files": len(declared)}
        result["status"] = "local-sdk-rehearsal-pass"
    except Exception as error:
        result["error"] = str(error)
        raise
    finally:
        write_json(out / "artifact-receipt.json", result)
    print(json.dumps(result["archive"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True, help="Absolute path to the reviewed SDK rehearsal input JSON")
    parser.add_argument("--out", required=True, help="Absolute path to a fresh local output directory")
    args = parser.parse_args()
    try:
        require(os.name == "nt", "The first SDK rehearsal supports Windows only")
        execute(absolute_path(args.inputs), absolute_path(args.out))
        return 0
    except (OSError, ValueError, KeyError, TypeError, zipfile.BadZipFile, subprocess.TimeoutExpired) as error:
        print(f"SDK rehearsal incomplete: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
