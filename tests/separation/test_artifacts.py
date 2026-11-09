"""Synthetic validator sensitivity only; never installed-SDK/product evidence.

Run with explicit --contract <B0-Q1.json> --work <existing-private-output-root>.
Every temporary fixture lives below that verified root; no producer is built.
"""

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

sys.dont_write_bytecode = True

import artifacts
import qualify


WORK = None
CONTRACT = None


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    return artifacts.digest(path)


class ArtifactTests(unittest.TestCase):
    def setUp(self):
        artifacts.require(WORK is not None and CONTRACT is not None, "test-input",
                          "Run this test script with explicit --contract and --work")
        # TemporaryDirectory owns only a new, bounded child, including cleanup.
        self.temporary = tempfile.TemporaryDirectory(prefix="validator-", dir=WORK)
        self.root = Path(self.temporary.name).absolute()
        self.assertTrue(self.root.is_relative_to(WORK))
        self.addCleanup(self.temporary.cleanup)
        self.contract = self.root / "contract.json"
        self.contract.write_bytes(CONTRACT.read_bytes())
        self.contract_hash = artifacts.digest(self.contract)
        self.dependencies = artifacts.read_json(self.contract)["technicalContract"]["dependencyMap"]

    def problem(self, code, operation):
        with self.assertRaises(artifacts.InputError) as result:
            operation()
        self.assertEqual(result.exception.code, code)

    def fixture(self, name="payload", tree=False):
        content = {"include/api.hpp": b"synthetic header", "profiles/a/runtime.dll": b"not executable"}
        entries = [{"path": path, "size": len(value), "sha256": hashlib.sha256(value).hexdigest()}
                   for path, value in sorted(content.items())]
        inventory_path = self.root / (name + ".inventory.json")
        inventory_hash = write_json(inventory_path, {"schema": "pixie-artifact-inventory/1", "files": entries})
        value = {"sdkIdentity": "1" * 64, "productVersion": "0.0.0.0", "os": "windows",
                 "architecture": "x64", "configuration": "Release", "compilerIdentity": "synthetic-compiler",
                 "toolsetIdentity": "synthetic-toolset", "standardLibraryPolicy": "exact",
                 "crt": "MD", "cpuFeatures": ["avx2", "fma", "f16c"],
                 "publicDependencyLockSha256": "2" * 64, "publicOptionsSha256": "3" * 64,
                 "capabilityProfile": "authoring", "capabilities": {"moduleLoading": True, "nativeAutomation": False},
                 "cAbi": 28, "candidateContract": 1, "sceneEnvelope": 2, "presentationSchema": 1}
        provenance_path = self.root / (name + ".provenance.json")
        provenance_hash = write_json(provenance_path, {"schema": "pixie-artifact-provenance/1",
            "producerHead": "4" * 40, "sourceInventorySha256": "5" * 64,
            "dirtyManifestSha256": "6" * 64, "sourceRoots": [str(self.root / "absent-source")], "tuple": value})
        result = {"role": name, "inventoryPath": str(inventory_path), "inventorySha256": inventory_hash,
                  "provenancePath": str(provenance_path), "provenanceSha256": provenance_hash, "tuple": value}
        if tree:
            folder = self.root / name
            for path, body in content.items():
                file = folder / path
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_bytes(body)
            result["treePath"] = str(folder)
        else:
            archive = self.root / (name + ".zip")
            with zipfile.ZipFile(archive, "w") as output:
                for path, body in content.items():
                    output.writestr(path, body)
            result.update(archivePath=str(archive), archiveSha256=artifacts.digest(archive))
        return result

    def inputs(self):
        item = self.fixture("sdk-a")
        roles = [item, {"role": "runtime-a", "payloadRef": {"role": "sdk-a", "path": "profiles/a"}},
                 {**item, "role": "g1-consumer-fixture"}, {**item, "role": "fixture-runner"}]
        tool = self.root / "declared-tool.txt"
        tool.write_text("synthetic tool, never executed", encoding="utf-8")
        tools = {role: {"path": str(tool), "sha256": artifacts.digest(tool), "version": "synthetic-only"}
                 for role in ("python", "cmake", "generator", "compiler")}
        receipts = []
        selected = qualify.requirements(self.dependencies, "G1")
        for index, role in enumerate(selected["inputReceipts"]):
            path = self.root / ("receipt-" + str(index) + ".json")
            sha = write_json(path, {"syntheticDocument": role})
            receipts.append({"role": role, "path": str(path), "sha256": sha, "kind": "document"})
        return {"schema": "pixie-separation-qualification-inputs/1", "interfaceRevision": 1,
                "draft": False, "runId": "synthetic-only", "phase": "early", "gates": ["G1"],
                "contract": {"path": str(self.contract), "sha256": self.contract_hash},
                "artifacts": roles, "tools": tools, "receipts": receipts,
                "decisionFlags": {"nativeLibExternal": False, "editorExportWorkflowRequired": True},
                "outputs": {"root": str(self.root)}, "candidate": None,
                "isolation": {"producerRoots": [str(self.root / "absent-source")]}}

    def input_path(self, value):
        path = self.root / "inputs.json"
        write_json(path, value)
        return path

    def test_healthy_zip_inventory(self):
        item = self.fixture()
        verified = artifacts.ArtifactVerifier([item]).verify("payload")
        self.assertEqual(set(verified["files"]), {"include/api.hpp", "profiles/a/runtime.dll"})

    def test_healthy_tree_inventory(self):
        item = self.fixture(tree=True)
        self.assertEqual(len(artifacts.ArtifactVerifier([item]).verify("payload")["files"]), 2)

    def test_wrong_file_hash(self):
        source = self.root / "healthy.txt"
        source.write_text("healthy content", encoding="utf-8")
        self.problem("hash-mismatch", lambda: artifacts.file_reference(str(source), "f" * 64))

    def test_changed_archive(self):
        item = self.fixture()
        with Path(item["archivePath"]).open("ab") as output:
            output.write(b"changed bytes")
        self.problem("hash-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def test_changed_inventory_even_with_valid_json(self):
        item = self.fixture()
        with Path(item["inventoryPath"]).open("a") as output:
            output.write(" ")
        self.problem("hash-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def test_undeclared_zip_payload(self):
        item = self.fixture()
        with zipfile.ZipFile(item["archivePath"], "a") as output:
            output.writestr("unlisted.dll", b"extra")
        item["archiveSha256"] = artifacts.digest(item["archivePath"])
        self.problem("undeclared-payload", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def test_missing_zip_payload(self):
        item = self.fixture()
        with zipfile.ZipFile(item["archivePath"], "w") as output:
            output.writestr("include/api.hpp", b"synthetic header")
        item["archiveSha256"] = artifacts.digest(item["archivePath"])
        self.problem("inventory-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def test_rehashed_archive_with_wrong_payload_digest(self):
        item = self.fixture()
        with zipfile.ZipFile(item["archivePath"], "w") as output:
            output.writestr("include/api.hpp", b"X" * len(b"synthetic header"))
            output.writestr("profiles/a/runtime.dll", b"not executable")
        item["archiveSha256"] = artifacts.digest(item["archivePath"])
        self.problem("inventory-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def test_changed_tree_and_undeclared_tree(self):
        item = self.fixture(tree=True)
        (Path(item["treePath"]) / "include/api.hpp").write_bytes(b"changed")
        self.problem("inventory-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))
        (Path(item["treePath"]) / "extra.dll").write_bytes(b"extra")
        self.problem("inventory-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def test_unsafe_names(self):
        for value in ("../escape", "/absolute", "C:/drive", "//server/share", "a\\b", "a/../b",
                      "a//b", "a/./b", "file:stream", "name. ", "NUL.txt", "COM1", "a/CON", "a\x00b"):
            with self.subTest(value=value):
                self.problem("unsafe-member", lambda: artifacts.relative_name(value))

    def test_zip_traversal(self):
        path = self.root / "traversal.zip"
        with zipfile.ZipFile(path, "w") as output:
            output.writestr("../escape", b"escape")
        self.problem("unsafe-member", lambda: artifacts.zip_files(path, {}))
        self.assertFalse((self.root / "escape").exists())

    def test_case_and_file_directory_collisions(self):
        for first, second in (("A/x", "a/y"), ("x", "X/y"), ("api.hpp", "API.hpp"), ("x", "x")):
            with self.subTest(first=first, second=second):
                names = artifacts.Names()
                names.add(first)
                self.problem("path-collision", lambda: names.add(second))

    def test_zip_symbolic_link(self):
        path = self.root / "symlink.zip"
        entry = zipfile.ZipInfo("shortcut")
        entry.create_system = 3
        entry.external_attr = (stat.S_IFLNK | 0o777) << 16
        with zipfile.ZipFile(path, "w") as output:
            output.writestr(entry, "../outside")
        self.problem("linked-path", lambda: artifacts.zip_files(path, {}))

    def test_actual_hardlink(self):
        source, alias = self.root / "original", self.root / "alias"
        source.write_bytes(b"file")
        os.link(source, alias)
        self.problem("hardlinked-file", lambda: artifacts.no_links(alias))

    def test_actual_symlink_when_supported(self):
        source, alias = self.root / "original", self.root / "alias"
        source.write_bytes(b"file")
        try:
            alias.symlink_to(source)
        except OSError as error:
            self.skipTest(f"Host does not permit creating symlinks: {error.winerror}")
        self.problem("linked-path", lambda: artifacts.no_links(alias))

    def test_reparse_attribute(self):
        source = self.root / "reparse-simulation"
        source.write_bytes(b"fixture")
        original = Path.lstat

        def lstat(path, *args, **kwargs):
            info = original(path, *args, **kwargs)
            if path == source:
                return mock.Mock(st_mode=info.st_mode, st_nlink=1, st_file_attributes=0x400)
            return info

        with mock.patch.object(Path, "lstat", lstat):
            self.problem("linked-path", lambda: artifacts.no_links(source))

    def test_payload_member_and_cycle(self):
        item = self.fixture()
        alias = {"role": "runtime", "payloadRef": {"role": "payload", "path": "profiles/a"}}
        self.assertEqual(len(artifacts.ArtifactVerifier([item, alias]).verify("runtime")["files"]), 1)
        alias["payloadRef"]["role"] = "runtime"
        self.problem("payload-cycle", lambda: artifacts.ArtifactVerifier([alias]).verify("runtime"))

    def test_payload_alias_cannot_override_tuple(self):
        item = self.fixture()
        alias = {"role": "runtime", "payloadRef": {"role": "payload", "path": "profiles/a"}, "tuple": {}}
        self.problem("ambiguous-payload", lambda: artifacts.ArtifactVerifier([item, alias]).verify("runtime"))

    def test_provenance_tuple_mismatch(self):
        item = self.fixture()
        item["tuple"]["configuration"] = "Debug"
        self.problem("provenance-mismatch", lambda: artifacts.ArtifactVerifier([item]).verify("payload"))

    def receipt(self):
        item = self.fixture()
        identity = artifacts.ArtifactVerifier([item]).verify("payload")["identity"]
        path = self.root / "gate-receipt.json"
        body = {"schema": "pixie-separation-receipt/1", "role": "G1/used-tuple", "gate": "G1",
                "contractSha256": self.contract_hash, "status": "pass", "scope": "full-gate",
                "artifacts": {"payload": identity}}
        reference = {"role": "G1/used-tuple", "path": str(path), "sha256": write_json(path, body),
                     "kind": "receipt", "artifactRoles": ["payload"]}
        return body, reference, {"payload": identity}

    def test_matching_receipt(self):
        _, reference, identities = self.receipt()
        self.assertFalse(artifacts.verify_receipt(reference, identities, self.contract_hash)["semanticAcceptance"])

    def test_receipt_wrong_artifact(self):
        _, reference, identities = self.receipt()
        identities["payload"]["tupleSha256"] = "f" * 64
        self.problem("receipt-artifact-mismatch", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))

    def test_receipt_wrong_role_and_contract(self):
        _, reference, identities = self.receipt()
        reference["role"] = "G2/different"
        self.problem("receipt-mismatch", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))

    def test_receipt_case_and_contract_binding(self):
        body, reference, identities = self.receipt()
        reference["case"] = "relocation"
        body["case"] = "profile"
        reference["sha256"] = write_json(Path(reference["path"]), body)
        self.problem("receipt-mismatch", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))
        reference.pop("case")
        self.problem("receipt-mismatch", lambda: artifacts.verify_receipt(reference, identities, "f" * 64))

    def test_partial_receipt_cannot_close_gate(self):
        body, reference, identities = self.receipt()
        body["scope"] = "partial"
        reference["sha256"] = write_json(Path(reference["path"]), body)
        self.problem("partial-receipt", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))

    def test_document_cannot_replace_gate_receipt(self):
        _, reference, identities = self.receipt()
        reference["kind"] = "document"
        self.problem("unbound-receipt", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))

    def test_missing_gate_bindings_and_failed_receipt(self):
        body, reference, identities = self.receipt()
        reference["artifactRoles"] = []
        self.problem("unbound-receipt", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))
        body["status"] = "blocked"
        reference["sha256"] = write_json(Path(reference["path"]), body)
        self.problem("receipt-not-passed", lambda: artifacts.verify_receipt(reference, identities, self.contract_hash))

    def test_g1_does_not_require_other_products(self):
        value = qualify.requirements(self.dependencies, "G1")
        self.assertEqual(set(value["artifacts"]), {"sdk-a", "runtime-a", "fixture-runner", "g1-consumer-fixture"})
        self.assertEqual(set(value["tools"]), {"python", "cmake", "generator", "compiler"})
        self.assertFalse(value["unresolvedDecisions"])

    def test_g5_native_scope(self):
        value = qualify.requirements(self.dependencies, "G5", "native-protocol")
        self.assertFalse(any("editor" in item or "export" in item for item in value["artifacts"]))
        self.assertNotIn("D06", value["unresolvedDecisions"])
        self.assertNotIn("D07", value["unresolvedDecisions"])
        self.assertFalse(value["priorGateReceipts"])

    def test_product_exclusion_selects_only_one_product(self):
        value = qualify.requirements(self.dependencies, "G5", "product-exclusion", "public-sdk")
        self.assertEqual(set(value["artifacts"]), {"fixture-runner", "product-exclusion-probe", "public-sdk"})

    def test_full_g5_and_g6_require_complete_inputs(self):
        g5 = qualify.requirements(self.dependencies, "G5")
        self.assertIn("private-automation-editor-save-off", g5["artifacts"])
        self.assertIn("shipping-code-export", g5["artifacts"])
        g6 = qualify.requirements(self.dependencies, "G6")
        self.assertIn("sdk-b", g6["artifacts"])
        self.assertIn("b11.documentation", g6["inputReceipts"])
        self.assertIn("D12", g6["unresolvedDecisions"])

    def test_accepted_decision_cannot_be_overridden(self):
        self.problem("decision-conflict", lambda: qualify.requirements(self.dependencies, "G1", flags={"nativeLibExternal": True}))

    def test_generated_context_receipt_is_not_input(self):
        value = qualify.requirements(self.dependencies, "G4")
        self.assertNotIn("k03.native-context-result", value["inputReceipts"])
        self.assertIn("k03.native-context-result", value["generatedCaseReceipts"])
        self.assertIn("b0.d09-disposition", value["acceptanceReceipts"])

    def test_no_product_pass_after_healthy_preflight(self):
        value = self.inputs()
        value["artifacts"].append({"role": "editor", "archivePath": None})
        value["tools"]["dotnet"] = None
        result, folder = qualify.run(str(self.input_path(value)), "G1", str(self.root / "run"))
        self.assertEqual(result["structuralValidation"]["status"], "pass")
        self.assertFalse(result["productGatePassed"])
        self.assertEqual(result["productGates"]["G1"], "blocked")
        self.assertEqual(result["productGates"]["G6"], "not-run")
        self.assertEqual(result["sourceUnavailable"], "not-run")
        self.assertEqual(result["productCasesExecuted"], [])
        self.assertTrue((folder / "outputs.manifest.sha256").is_file())

    def test_missing_required_runtime_is_blocked(self):
        value = self.inputs()
        value["artifacts"] = [item for item in value["artifacts"] if item["role"] != "runtime-a"]
        result, _ = qualify.run(str(self.input_path(value)), "G1", str(self.root / "run"))
        self.assertEqual([issue["role"] for issue in result["structuralValidation"]["issues"]], ["runtime-a"])
        self.assertEqual(result["structuralValidation"]["status"], "blocked")

    def test_output_escape_and_existing_run(self):
        value = self.inputs()
        path = self.input_path(value)
        self.problem("output-outside-owned-root", lambda: qualify.run(str(path), "G1", str(self.root.parent / "escape")))
        existing = self.root / "existing"
        existing.mkdir()
        sentinel = existing / "user-data"
        sentinel.write_bytes(b"preserve")
        self.problem("output-not-fresh", lambda: qualify.run(str(path), "G1", str(existing)))
        self.assertEqual(sentinel.read_bytes(), b"preserve")

    def test_output_cannot_enter_input_tree(self):
        value = self.inputs()
        item = self.fixture("tree-sdk", tree=True)
        item["role"] = "sdk-a"
        value["artifacts"][0] = item
        destination = Path(item["treePath"]) / "new-run"
        self.problem("output-input-overlap", lambda: qualify.run(str(self.input_path(value)), "G1", str(destination)))
        self.assertFalse(destination.exists())

    def test_input_manifest_drift_is_detected(self):
        value = self.inputs()
        path = self.input_path(value)
        original = qualify.preflight

        def changing_preflight(*args):
            result = original(*args)
            with path.open("a") as stream:
                stream.write(" ")
            return result

        with mock.patch.object(qualify, "preflight", changing_preflight):
            self.problem("input-drift", lambda: qualify.run(str(path), "G1", str(self.root / "run")))
        self.assertFalse((self.root / "run").exists())

    def test_unrelated_null_receipts_and_unknown_variant(self):
        value = self.inputs()
        value["receipts"].append({"role": "b11.documentation", "path": None, "sha256": None})
        result, _ = qualify.run(str(self.input_path(value)), "G1", str(self.root / "run"))
        self.assertEqual(result["structuralValidation"]["status"], "pass")
        self.problem("variant-required", lambda: qualify.requirements(self.dependencies, "G5", "product-exclusion", "unknown"))

    def test_draft_and_unrequested_gate(self):
        value = self.inputs()
        value["draft"] = True
        self.problem("draft-input", lambda: qualify.run(str(self.input_path(value)), "G1", str(self.root / "run")))
        value["draft"] = False
        self.problem("unrequested-gate", lambda: qualify.run(str(self.input_path(value)), "G6", str(self.root / "run")))

    def test_input_schema_and_duplicate_json_keys(self):
        value = self.inputs()
        value["interfaceRevision"] = True
        self.problem("input-schema", lambda: qualify.run(str(self.input_path(value)), "G1", str(self.root / "run")))
        path = self.root / "duplicate.json"
        path.write_text('{"key": 1, "key": 2}', encoding="utf-8")
        self.problem("duplicate-json-key", lambda: artifacts.read_json(path))

    def test_duplicate_artifact_roles(self):
        item = self.fixture()
        self.problem("duplicate-role", lambda: artifacts.ArtifactVerifier([item, item]))

    def test_cli_never_returns_product_success(self):
        value = self.inputs()
        command = [sys.executable, "-B", str(Path(__file__).with_name("qualify.py")),
                   "--inputs", str(self.input_path(value)), "--gate", "G1", "--out", str(self.root / "cli-run")]
        completed = subprocess.run(command, capture_output=True, text=True, timeout=30)
        self.assertEqual(completed.returncode, 3, completed.stderr)
        self.assertFalse(json.loads(completed.stdout)["productGatePassed"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--contract", required=True)
    parser.add_argument("--work", required=True)
    options, remaining = parser.parse_known_args()
    WORK = artifacts.absolute_path(options.work)
    CONTRACT = artifacts.absolute_path(options.contract)
    artifacts.require(WORK.is_dir() and CONTRACT.is_file(), "test-input", "Explicit work directory and contract file must exist")
    unittest.main(argv=[sys.argv[0], *remaining])
