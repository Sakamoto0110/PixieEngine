"""Read-only artifact, inventory and receipt controls; no product execution.

Inventories use schema pixie-artifact-inventory/1 and files [{path, size, sha256}].
Provenance uses pixie-artifact-provenance/1 with producerHead,
sourceInventorySha256, dirtyManifestSha256, sourceRoots and an exact tuple.
Only ZIP inspection and existing-tree inspection are implemented. Nothing extracts
archives or runs their contents. Hash references come from the calling handoff.
"""

import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import unicodedata
import zipfile


class InputError(ValueError):
    def __init__(self, code, message, *, blocked=False):
        super().__init__(message)
        self.code = code
        self.blocked = blocked


def require(condition, code, message, *, blocked=False):
    if not condition:
        raise InputError(code, message, blocked=blocked)


def canonical_hash(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":"),
                                     ensure_ascii=False, allow_nan=False).encode("utf-8")).hexdigest()


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def sha256(value):
    require(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None,
            "invalid-sha256", "Expected a lowercase SHA256 digest")
    return value


def no_links(path):
    """Check existing ancestors as well as the leaf; never follow a junction."""
    path = Path(os.path.abspath(path))
    for entry in reversed([path, *path.parents]):
        try:
            info = entry.lstat()
        except FileNotFoundError:
            continue
        require(not stat.S_ISLNK(info.st_mode)
                and not (getattr(info, "st_file_attributes", 0) & 0x400),
                "linked-path", f"Link/reparse point is forbidden: {entry}")
        if stat.S_ISREG(info.st_mode):
            require(info.st_nlink == 1, "hardlinked-file", f"Hardlinked input is forbidden: {entry}")
    return path


def absolute_path(value):
    require(isinstance(value, (str, os.PathLike)) and value and Path(value).is_absolute(),
            "absolute-input-required", f"Expected an absolute path, got {value!r}")
    return no_links(value)


def file_reference(path, expected):
    require(path is not None and expected is not None, "unresolved-reference",
            "Required path/hash is unresolved", blocked=True)
    source = absolute_path(path)
    sha256(expected)
    require(source.is_file(), "missing-file", f"Required file is absent: {source}", blocked=True)
    require(digest(source) == expected, "hash-mismatch", f"File changed or has wrong hash: {source}")
    return source


def read_json(path):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, "duplicate-json-key", f"Repeated JSON key: {key}")
            result[key] = value
        return result

    def constant(value):
        raise InputError("invalid-json", f"Nonfinite JSON number: {value}")

    try:
        return json.loads(Path(path).read_text(encoding="utf-8-sig"), object_pairs_hook=pairs,
                          parse_constant=constant)
    except (UnicodeError, json.JSONDecodeError) as error:
        raise InputError("invalid-json", str(error)) from error


def validate_schema(value, schema, root=None, location="$"):
    """The standard-library validator implements the subset used by our schema."""
    root = schema if root is None else root
    if "$ref" in schema:
        reference = schema["$ref"]
        require(reference.startswith("#/"), "schema-reference", "Only local schema references are supported")
        target = root
        for part in reference[2:].split("/"):
            target = target[part]
        validate_schema(value, target, root, location)
        return
    if "anyOf" in schema:
        for candidate in schema["anyOf"]:
            try:
                validate_schema(value, candidate, root, location)
                return
            except InputError as error:
                if error.code != "input-schema":
                    raise
        raise InputError("input-schema", f"{location}: no supported shape matches")
    if "const" in schema:
        require(type(value) is type(schema["const"]) and value == schema["const"],
                "input-schema", f"{location}: unexpected constant")
    if "enum" in schema:
        require(any(type(value) is type(option) and value == option for option in schema["enum"]),
                "input-schema", f"{location}: unsupported value")
    types = {"object": dict, "array": list, "string": str, "integer": int,
             "boolean": bool, "null": type(None)}
    if "type" in schema:
        names = schema["type"] if isinstance(schema["type"], list) else [schema["type"]]
        require(any(type(value) is types[name] for name in names), "input-schema", f"{location}: wrong type")
    if isinstance(value, dict):
        for key in schema.get("required", []):
            require(key in value, "input-schema", f"{location}.{key}: required")
        properties = schema.get("properties", {})
        for key, item in value.items():
            extra = schema.get("additionalProperties", True)
            require(key in properties or extra is not False, "input-schema", f"{location}.{key}: unknown field")
            child = properties.get(key, extra if isinstance(extra, dict) else None)
            if child:
                validate_schema(item, child, root, f"{location}.{key}")
    if isinstance(value, list):
        require(len(value) >= schema.get("minItems", 0), "input-schema", f"{location}: too few items")
        if schema.get("uniqueItems"):
            require(len({canonical_hash(item) for item in value}) == len(value), "input-schema", f"{location}: duplicate item")
        for index, item in enumerate(value):
            if "items" in schema:
                validate_schema(item, schema["items"], root, f"{location}[{index}]")
    if isinstance(value, str):
        require(len(value) >= schema.get("minLength", 0), "input-schema", f"{location}: empty string")
        if "pattern" in schema:
            require(re.search(schema["pattern"], value) is not None, "input-schema", f"{location}: invalid string")


def relative_name(value):
    require(isinstance(value, str) and value and "\\" not in value and ":" not in value,
            "unsafe-member", f"Invalid payload name: {value!r}")
    parts = value.split("/")
    require(not PurePosixPath(value).is_absolute() and all(part not in ("", ".", "..") for part in parts),
            "unsafe-member", f"Nonrelative/noncanonical payload name: {value!r}")
    for part in parts:
        require(not any(ord(char) < 32 or char in '<>"|?*' for char in part)
                and not part.endswith((".", " ")), "unsafe-member", f"Windows-aliased payload name: {value!r}")
        stem = part.split(".")[0].upper()
        require(stem not in {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"}
                and re.fullmatch(r"(?:COM|LPT)[0-9¹²³]", stem) is None,
                "unsafe-member", f"Reserved device name: {value!r}")
    return value


class Names:
    def __init__(self):
        self.entries = {}

    def add(self, name, directory=False):
        relative_name(name)
        parts = name.split("/")
        for count in range(1, len(parts) + 1):
            original = "/".join(parts[:count])
            folder = directory or count < len(parts)
            key = unicodedata.normalize("NFC", original).casefold()
            previous = self.entries.get(key)
            if previous is not None:
                require(previous == (original, True) and folder, "path-collision",
                        f"Duplicate, case alias or file/directory collision: {name}")
            self.entries[key] = (original, folder)


def inventory(data):
    require(isinstance(data, dict) and data.get("schema") == "pixie-artifact-inventory/1"
            and isinstance(data.get("files"), list), "inventory-schema", "Expected a file inventory")
    names, files = Names(), {}
    for item in data["files"]:
        require(isinstance(item, dict) and set(item) == {"path", "size", "sha256"},
                "inventory-schema", "Inventory entry must have path, size and sha256")
        name = relative_name(item["path"])
        names.add(name)
        require(type(item["size"]) is int and item["size"] >= 0,
                "inventory-schema", f"Invalid size for {name}")
        sha256(item["sha256"])
        files[name] = {"size": item["size"], "sha256": item["sha256"]}
    return files


def tree_files(root):
    root = absolute_path(root)
    require(root.is_dir(), "missing-tree", f"Artifact tree is absent: {root}", blocked=True)
    names, files = Names(), {}
    for folder, directories, filenames in os.walk(root, followlinks=False):
        for name in sorted(directories + filenames):
            path = no_links(Path(folder) / name)
            relative = path.relative_to(root).as_posix()
            info = path.lstat()
            require(stat.S_ISDIR(info.st_mode) or stat.S_ISREG(info.st_mode), "special-file", f"Unsupported file: {path}")
            names.add(relative, stat.S_ISDIR(info.st_mode))
            if stat.S_ISREG(info.st_mode):
                files[relative] = {"size": info.st_size, "sha256": digest(path)}
    return files


def zip_files(path, declared):
    names, seen, files = Names(), set(), {}
    try:
        with zipfile.ZipFile(path) as archive:
            for entry in archive.infolist():
                name = entry.filename[:-1] if entry.is_dir() else entry.filename
                relative_name(name)
                require(name not in seen, "path-collision", f"Repeated ZIP entry: {name}")
                seen.add(name)
                names.add(name, entry.is_dir())
                mode = entry.external_attr >> 16
                require(stat.S_IFMT(mode) in (0, stat.S_IFREG, stat.S_IFDIR)
                        and not (entry.external_attr & 0x400), "linked-path", f"Linked/special ZIP entry: {name}")
                require(not (entry.flag_bits & 1), "encrypted-archive", "Encrypted ZIP entries are unsupported")
                if entry.is_dir():
                    continue
                require(name in declared, "undeclared-payload", f"Undeclared ZIP entry: {name}")
                require(entry.file_size == declared[name]["size"], "inventory-mismatch", f"Wrong size: {name}")
                with archive.open(entry) as stream:
                    value = hashlib.file_digest(stream, "sha256").hexdigest()
                files[name] = {"size": entry.file_size, "sha256": value}
    except (zipfile.BadZipFile, NotImplementedError, RuntimeError) as error:
        raise InputError("invalid-archive", str(error)) from error
    return files


TUPLE_FIELDS = {"sdkIdentity", "productVersion", "os", "architecture", "configuration",
                "compilerIdentity", "toolsetIdentity", "standardLibraryPolicy", "crt", "cpuFeatures",
                "publicDependencyLockSha256", "publicOptionsSha256", "capabilityProfile", "capabilities",
                "cAbi", "candidateContract", "sceneEnvelope", "presentationSchema"}


class ArtifactVerifier:
    def __init__(self, artifacts):
        self.entries = role_index(artifacts)
        self.verified = {}
        self.documents = set()
        self.roots = set()

    def verify(self, role, active=()):
        require(role not in active, "payload-cycle", f"Cyclic payload reference: {role}")
        if role in self.verified:
            return self.verified[role]
        require(role in self.entries, "missing-artifact", f"Required artifact: {role}", blocked=True)
        item = self.entries[role]
        reference = item.get("payloadRef")
        if reference is not None:
            require(not any(item.get(key) is not None for key in
                            ("archivePath", "archiveSha256", "treePath", "inventoryPath", "inventorySha256",
                             "provenancePath", "provenanceSha256", "tuple")),
                    "ambiguous-payload", f"Payload alias cannot override its parent identity: {role}")
            parent = self.verify(reference["role"], (*active, role))
            prefix = relative_name(reference["path"])
            selected = {path: info for path, info in parent["files"].items()
                        if path == prefix or path.startswith(prefix + "/")}
            require(selected, "missing-payload", f"Missing payload {prefix} in {reference['role']}", blocked=True)
            result = {"identity": {**parent["identity"], "payloadRef": reference}, "files": selected}
        else:
            inv_path = file_reference(item.get("inventoryPath"), item.get("inventorySha256"))
            prov_path = file_reference(item.get("provenancePath"), item.get("provenanceSha256"))
            self.documents.update((inv_path, prov_path))
            expected = inventory(read_json(inv_path))
            provenance = read_json(prov_path)
            value = item.get("tuple")
            require(isinstance(value, dict) and TUPLE_FIELDS <= value.keys()
                    and all(value[key] is not None for key in TUPLE_FIELDS),
                    "unresolved-tuple", f"Incomplete immutable tuple: {role}", blocked=True)
            for key in ("sdkIdentity", "publicDependencyLockSha256", "publicOptionsSha256"):
                sha256(value[key])
            require(all(isinstance(value[key], str) and value[key] for key in
                        ("productVersion", "os", "architecture", "configuration", "compilerIdentity",
                         "toolsetIdentity", "standardLibraryPolicy", "crt", "capabilityProfile"))
                    and isinstance(value["cpuFeatures"], list) and bool(value["cpuFeatures"])
                    and all(isinstance(feature, str) and feature for feature in value["cpuFeatures"])
                    and isinstance(value["capabilities"], dict)
                    and all(type(flag) is bool for flag in value["capabilities"].values())
                    and all(type(value[key]) is int for key in
                            ("cAbi", "candidateContract", "sceneEnvelope", "presentationSchema")),
                    "tuple-schema", f"Invalid typed tuple fields: {role}")
            require(isinstance(provenance, dict) and provenance.get("schema") == "pixie-artifact-provenance/1"
                    and provenance.get("tuple") == value, "provenance-mismatch", f"Wrong tuple/provenance: {role}")
            require(isinstance(provenance.get("producerHead"), str)
                    and re.fullmatch(r"[0-9a-f]{40}|[0-9a-f]{64}", provenance["producerHead"]),
                    "provenance-mismatch", f"Missing producer source identity: {role}")
            for key in ("sourceInventorySha256", "dirtyManifestSha256"):
                sha256(provenance.get(key))
            require(isinstance(provenance.get("sourceRoots"), list) and provenance["sourceRoots"],
                    "provenance-mismatch", f"Explicit producer roots required: {role}")
            for source_root in provenance["sourceRoots"]:
                # These may be absent in a qualifying runner; do not open them.
                require(isinstance(source_root, str) and Path(source_root).is_absolute(),
                        "provenance-mismatch", f"Producer root must be absolute: {role}")
                self.roots.add(Path(source_root))
            require(bool(item.get("archivePath")) != bool(item.get("treePath")),
                    "artifact-location", f"Choose one explicit archive or tree for {role}", blocked=True)
            if item.get("archivePath"):
                payload = file_reference(item["archivePath"], item.get("archiveSha256"))
                self.documents.add(payload)
                actual = zip_files(payload, expected)
            else:
                require(item.get("archiveSha256") is None, "artifact-location", "A tree has no archive hash")
                payload = absolute_path(item["treePath"])
                actual = tree_files(payload)
            require(actual == expected, "inventory-mismatch", f"Missing, changed or undeclared payload in {role}")
            require(digest(inv_path) == item["inventorySha256"] and digest(prov_path) == item["provenanceSha256"],
                    "input-drift", f"Metadata changed during validation: {role}")
            if item.get("archivePath"):
                require(digest(payload) == item["archiveSha256"], "input-drift", f"Archive changed during validation: {role}")
            result = {"identity": {"archiveSha256": item.get("archiveSha256"),
                                   "inventorySha256": item["inventorySha256"],
                                   "provenanceSha256": item["provenanceSha256"],
                                   "tupleSha256": canonical_hash(value)}, "files": actual}
        self.verified[role] = result
        return result


def role_index(entries):
    result = {}
    for entry in entries:
        role = entry["role"]
        require(role not in result, "duplicate-role", f"Duplicate role: {role}")
        result[role] = entry
    return result


def verify_receipt(reference, identities, contract_hash):
    path = file_reference(reference.get("path"), reference.get("sha256"))
    document = read_json(path)
    require(digest(path) == reference["sha256"], "input-drift", "Receipt changed during validation")
    role = reference["role"]
    gate_receipt = re.match(r"^G[1-6]/", role) is not None
    if reference.get("kind", "receipt") == "document":
        require(not gate_receipt, "unbound-receipt", "A hashed document cannot satisfy a prior product gate")
        return {"role": role, "sha256": reference["sha256"], "kind": "document", "semanticAcceptance": False}
    require(isinstance(document, dict) and document.get("schema") == "pixie-separation-receipt/1"
            and document.get("role") == role and document.get("contractSha256") == contract_hash,
            "receipt-mismatch", f"Wrong receipt role/contract: {role}")
    require(document.get("status") == "pass", "receipt-not-passed", f"Receipt is not passing: {role}", blocked=True)
    bindings = reference.get("artifactRoles", [])
    require(not gate_receipt or bool(bindings), "unbound-receipt", "Prior gate receipt requires explicit artifact bindings")
    if gate_receipt:
        require(document.get("scope") == "full-gate" and document.get("gate") == role.split("/")[0],
                "partial-receipt", "A partial or unrelated receipt cannot satisfy a whole gate")
    for key in ("case", "variant"):
        if key in reference:
            require(document.get(key) == reference[key], "receipt-mismatch", f"Wrong {key}: {role}")
    for artifact in bindings:
        require(artifact in identities and document.get("artifacts", {}).get(artifact) == identities[artifact],
                "receipt-artifact-mismatch", f"Stale or different artifact/tuple for {role}: {artifact}")
    return {"role": role, "sha256": reference["sha256"], "kind": "receipt", "semanticAcceptance": False}
