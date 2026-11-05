"""Validate a selected gate's immutable inputs; product executors are not supplied.

Exit 2 means invalid input; exit 3 means blocked product qualification. Even a
healthy structural preflight exits 3 and records no product-gate pass. No tool,
archive content, build, renderer, release policy or producer source is executed.
"""

import argparse
import hashlib
import json
from pathlib import Path
import platform
import sys

sys.dont_write_bytecode = True

from artifacts import (ArtifactVerifier, InputError, absolute_path, canonical_hash,
                       digest, file_reference, no_links, read_json, require,
                       role_index, validate_schema, verify_receipt)


def requirements(dependencies, gate, case=None, variant=None, flags=None):
    """Resolve the frozen map, not an all-product list or a second SDK selector."""
    flags = flags or {}
    require(gate in dependencies.get("gates", {}), "unknown-gate", f"Unknown gate: {gate}")
    common, definition = dependencies["common"], dependencies["gates"][gate]
    result = {key: set() for key in ("artifacts", "tools", "inputReceipts", "priorGateReceipts",
                                   "generatedPreflightReceipts", "generatedCaseReceipts",
                                   "acceptanceReceipts", "independenceAcceptanceReceipts",
                                   "decisionsAtExecution")}
    unresolved = set()

    def merge(item):
        for key in result:
            result[key].update(item.get(key, []))

    merge(common)
    if case:
        require(case in definition["cases"], "unknown-case", f"Unknown {gate} case: {case}")
        slices = definition.get("evidenceSlices", {})
        selected = slices.get(case, definition)
        merge(selected)
        if selected.get("variantRequired"):
            require(variant in selected["variants"], "variant-required", f"Select a known {case} variant")
            merge(selected["variants"][variant])
        else:
            require(variant is None, "unexpected-variant", f"Case {case} has no variant")
        if case in slices:
            # Q1 provides per-slice roles but no separate decision array. Avoid
            # making native-only slices wait for Editor selection/generation.
            decisions = set(definition.get("decisionsAtExecution", [])) - {"D06", "D07"}
            if any("editor" in role for role in result["artifacts"]):
                decisions.add("D06")
            if "generation-tool" in result["artifacts"]:
                decisions.update(("D06", "D07"))
            result["decisionsAtExecution"].update(decisions)
    else:
        require(variant is None, "unexpected-variant", "A variant requires a case")
        merge(definition)
        # Whole G5 includes every slice/variant; a selected slice uses only itself.
        for selected in definition.get("evidenceSlices", {}).values():
            merge(selected)
            for alternative in selected.get("variants", {}).values():
                merge(alternative)

    for name in definition.get("conditionalRules", []):
        rule = dependencies["conditionalRules"][name]
        if rule.get("appliesToArtifactRoles") and not result["artifacts"].intersection(rule["appliesToArtifactRoles"]):
            continue
        if name == "managed-generation-tool" and case and case != "generation":
            continue
        value = flags.get(rule["flag"])
        if "acceptedFlagValue" in rule:
            accepted = rule["acceptedFlagValue"]
            require(value is None or value is accepted, "decision-conflict", f"Flag contradicts accepted {rule['decision']}")
            value = accepted
        if value is None:
            unresolved.add(rule["decision"] + "/" + rule["flag"])
            continue
        extra = rule.get("whenTrue" if value else "whenFalse", {})
        merge(extra)
        result["inputReceipts"].update(extra.get("receipts", []))
        result["priorGateReceipts"].update(extra.get("requiredPriorGateReceipts", []))

    if gate == "G6":
        # Q1 freezes this dependency union only; no candidate assembly/policy runs.
        for upstream in ("G1", "G2", "G3", "G4", "G5"):
            resolved = requirements(dependencies, upstream, flags=flags)
            merge(resolved)
            unresolved.update(resolved["unresolvedDecisions"])
    accepted = set(dependencies.get("acceptedDecisionEvidence", {}).get("acceptedIds", []))
    unresolved.update(result["decisionsAtExecution"] - accepted)
    return {**{key: sorted(value) for key, value in result.items()},
            "unresolvedDecisions": sorted(unresolved), "partialSelection": case is not None}


def load_inputs(path, gate, case=None, variant=None):
    path = absolute_path(path)
    data = read_json(path)
    require(isinstance(data, dict), "input-schema", "Input must be a JSON object")
    require(data.get("draft") is not True, "draft-input", "Draft inputs cannot qualify products", blocked=True)
    schema = read_json(Path(__file__).with_name("inputs.schema.json"))
    validate_schema(data, schema)
    require(gate in data["gates"], "unrequested-gate", f"{gate} is not explicitly requested")
    require(gate != "G6" or data["phase"] == "candidate", "candidate-phase-required", "G6 requires candidate phase")
    contract = data["contract"]
    contract_path = file_reference(contract.get("path"), contract.get("sha256"))
    document = read_json(contract_path)
    require(isinstance(document, dict) and document.get("schema") == "pixie-separation-060-qualification-contract/1",
            "contract-schema", "Expected a frozen B0 qualification contract")
    technical = document.get("technicalContract", {})
    require(technical.get("inputSchema") == data["schema"]
            and technical.get("interfaceRevision") == data["interfaceRevision"],
            "contract-mismatch", "Input interface differs from frozen contract")
    dependencies = technical.get("dependencyMap", {})
    require(dependencies.get("schema") == "pixie-separation-gate-dependencies/1"
            and dependencies.get("draft") is False, "draft-contract", "Dependency map must be frozen")
    resolved = requirements(dependencies, gate, case, variant, data.get("decisionFlags"))
    return data, resolved, contract_path


def preflight(data, resolved):
    artifacts = ArtifactVerifier(data["artifacts"])
    receipts = role_index(data["receipts"])
    checks, issues, observed = [], [], []

    def attempt(kind, role, operation):
        try:
            value = operation()
            checks.append({"kind": kind, "role": role, "status": "pass", "detail": value})
        except InputError as error:
            issues.append({"kind": kind, "role": role, "code": error.code,
                           "status": "blocked" if error.blocked else "fail", "message": str(error)})

    for decision in resolved["unresolvedDecisions"]:
        issues.append({"kind": "decision", "role": decision, "code": "unresolved-decision",
                       "status": "blocked", "message": "Required decision is not frozen in the supplied contract"})
    for role in resolved["artifacts"]:
        attempt("artifact", role, lambda role=role: artifacts.verify(role)["identity"])
    identities = {role: result["identity"] for role, result in artifacts.verified.items()}
    for role in resolved["tools"]:
        def tool(role=role):
            reference = data["tools"].get(role)
            require(reference is not None, "missing-tool", f"Required tool: {role}", blocked=True)
            require(isinstance(reference.get("version"), str) and reference["version"],
                    "unresolved-tool-version", f"Declare actual tool version: {role}", blocked=True)
            source = file_reference(reference.get("path"), reference.get("sha256"))
            observed.append((source, reference["sha256"]))
            return {"path": str(source), "sha256": reference["sha256"],
                    "declaredVersion": reference["version"], "executed": False}
        attempt("tool", role, tool)
    for role in sorted(set(resolved["inputReceipts"] + resolved["priorGateReceipts"])):
        def receipt(role=role):
            require(role in receipts, "missing-receipt", f"Required input receipt: {role}", blocked=True)
            reference = receipts[role]
            value = verify_receipt(reference, identities, data["contract"]["sha256"])
            observed.append((Path(reference["path"]), reference["sha256"]))
            return value
        attempt("input-receipt", role, receipt)
    return artifacts, checks, issues, observed


def output_directory(data, output, artifacts, input_path, contract_path, observed):
    owned = absolute_path(data["outputs"]["root"])
    destination = absolute_path(output)
    require(owned.is_dir() and destination != owned and destination.is_relative_to(owned),
            "output-outside-owned-root", "--out must be a fresh descendant of the explicit existing owned root")
    require(not destination.exists() and destination.parent.is_dir(),
            "output-not-fresh", "Output must be absent with an existing parent; existing runs are never overwritten")
    protected = {input_path, contract_path, *artifacts.documents, *(path for path, _ in observed)}
    protected.update(Path(artifacts.entries[role]["treePath"]) for role in artifacts.verified
                     if artifacts.entries[role].get("treePath"))
    for source in protected:
        source = no_links(source)
        require(not source.is_relative_to(destination) and not destination.is_relative_to(source),
                "output-input-overlap", f"Output overlaps input: {source}")
    roots = artifacts.roots | {Path(path) for path in (data.get("isolation") or {}).get("producerRoots", [])}
    for root in roots:
        root = absolute_path(str(root))
        require(root != destination and not root.is_relative_to(destination),
                "output-source-overlap", f"Output would contain/replace a producer root: {root}")
    destination.mkdir()
    return destination


def write_json(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=False, allow_nan=False)
        stream.write("\n")


def run(input_path, gate, output, case=None, variant=None):
    input_path = absolute_path(input_path)
    initial_hash = digest(input_path)
    data, resolved, contract_path = load_inputs(input_path, gate, case, variant)
    artifacts, checks, issues, observed = preflight(data, resolved)
    # A second independent read catches input/receipt/tree changes around validation.
    second = ArtifactVerifier(data["artifacts"])
    for role in artifacts.verified:
        require(second.verify(role) == artifacts.verified[role], "input-drift", f"Artifact changed during preflight: {role}")
    for path, expected in observed:
        file_reference(str(path), expected)
    file_reference(str(contract_path), data["contract"]["sha256"])
    require(digest(input_path) == initial_hash, "input-drift", "Execution manifest changed during preflight")
    destination = output_directory(data, output, artifacts, input_path, contract_path, observed)
    snapshot = input_path.read_bytes()
    require(hashlib.sha256(snapshot).hexdigest() == initial_hash,
            "input-drift", "Execution manifest changed before snapshot")
    with (destination / "inputs.snapshot.json").open("xb") as stream:
        stream.write(snapshot)
    environment = {"schema": "pixie-separation-environment/1", "python": sys.version,
                   "executable": sys.executable, "platform": platform.platform(),
                   "scope": "Structural validator process only; no tool/context/isolation verification"}
    write_json(destination / "environment.json", environment)
    status = "fail" if any(issue["status"] == "fail" for issue in issues) else "blocked" if issues else "pass"
    result = {"schema": "pixie-separation-structural-result/1", "runId": data["runId"],
              "gate": gate, "case": case, "variant": variant, "phase": data["phase"],
              "inputsSha256": initial_hash, "contractSha256": data["contract"]["sha256"],
              "dependencySelectionSha256": canonical_hash(resolved), "requirements": resolved,
              "structuralValidation": {"status": status, "checks": checks, "issues": issues},
              "artifactIdentities": {role: item["identity"] for role, item in artifacts.verified.items()},
              "productGates": {name: "blocked" if name == gate else "not-run" for name in ("G1", "G2", "G3", "G4", "G5", "G6")},
              "productGatePassed": False, "productCasesExecuted": [], "sourceUnavailable": "not-run",
              "graphicsContext": "not-run", "scope": "partial structural preparation",
              "blockers": ["Product case executors/artifacts and actual independence evidence are outside B0-Q1 implementation clearance"],
              "validatorSources": {name: digest(Path(__file__).with_name(name))
                                   for name in ("artifacts.py", "qualify.py", "inputs.schema.json")}}
    write_json(destination / "results.json", result)
    manifest = {"schema": "pixie-artifact-inventory/1", "files": [
        {"path": path.name, "size": path.stat().st_size, "sha256": digest(path)}
        for path in sorted(destination.iterdir())]}
    write_json(destination / "outputs.manifest.json", manifest)
    with (destination / "outputs.manifest.sha256").open("x", encoding="ascii") as stream:
        stream.write(digest(destination / "outputs.manifest.json") + "\n")
    return result, destination


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True)
    parser.add_argument("--gate", choices=["G1", "G2", "G3", "G4", "G5", "G6"], required=True)
    parser.add_argument("--case")
    parser.add_argument("--variant")
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    try:
        result, destination = run(args.inputs, args.gate, args.out, args.case, args.variant)
        print(json.dumps({"structuralValidation": result["structuralValidation"]["status"],
                          "productGatePassed": False, "result": str(destination / "results.json"),
                          "resultSha256": digest(destination / "results.json")}))
        return 2 if result["structuralValidation"]["status"] == "fail" else 3
    except InputError as error:
        print(json.dumps({"code": error.code, "message": str(error),
                          "status": "blocked" if error.blocked else "invalid", "productGatePassed": False}), file=sys.stderr)
        return 3 if error.blocked else 2
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"code": "invalid-input", "message": str(error), "productGatePassed": False}), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
