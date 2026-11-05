"""Build local publish fixtures; no package or artifact is uploaded."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", required=True)
    parser.add_argument("--work", required=True)
    args = parser.parse_args()
    project, work = Path(args.project).resolve(), Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="run-", dir=work)).resolve()
    assert run.is_relative_to(work) and run != work
    records = []

    def publish(name, properties, failure=None, no_build=False):
        output = run / (name + "-publish")
        command = ["dotnet", "publish", str(project), "-c", "Release", "-o", str(output),
                   "-p:OutputPath=" + str(run / (name + "-bin")) + "/",
                   "-p:IntermediateOutputPath=" + str(run / (name + "-obj")) + "/", *properties]
        if no_build:
            command.append("--no-build")
        result = subprocess.run(command, text=True, encoding="utf-8", errors="replace",
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
        (run / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        if failure:
            assert result.returncode != 0 and failure in result.stdout, result.stdout
            assert not (output / "PixieEditor.dll").exists()
        else:
            assert result.returncode == 0, result.stdout
        records.append({"case": name, "exit": result.returncode, "expected_error": failure})
        return output

    publish("development-blocked", [], "development operations cannot be published")
    published = publish("shipping", ["-p:PixieDevelopmentTools=false"])
    binary = (published / "PixieEditor.dll").read_bytes()
    for marker in (b"ProjectExportJob\0", b"ExportPanel\0", b"get_Export\0", "Export game...".encode("utf-16-le")):
        assert marker in binary, marker
    # Attribute the literal to its method so the public export cache can use the same word.
    inspector = Path(__file__).resolve().with_name("InspectEditorPublishIl.cs")
    audit_temp = run / "il-temp"
    audit_temp.mkdir()
    audit = subprocess.run(
        ["dotnet", "run", "--file", str(inspector), "--no-cache", "--artifacts-path", str(run / "il-artifacts"),
         "--", str(published / "PixieEditor.dll")],
        env=os.environ | {"TEMP": str(audit_temp), "TMP": str(audit_temp), "TMPDIR": str(audit_temp)},
        text=True, encoding="utf-8", errors="replace", stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
    (run / "scripted-export-il.log").write_text(audit.stdout, encoding="utf-8")
    assert audit.returncode == 0, audit.stdout
    for marker in (b"PixieEditor.Development", b"DeterministicCapture", b"get_QuitAfterSteps",
                   b"EditorChannel", b"EditorPipe", b"CaptureDirectory", b"SynchronizeChannel", b"ChannelTree", b"ChannelGesture",
                   b"DispatchDevelopment", b"DevDispatch", b"pixie_dev_dispatch", b"get_DevChannel", b"get_DevHidden", b"FindSceneEntity",
                   b"PixieEditor.ChannelTests", b"ModelContextProtocol",
                   b"ChannelReferenceLabel",
                   "--dev-channel".encode("utf-16-le"), "--dev-capture-dir".encode("utf-16-le"), "--dev-hidden".encode("utf-16-le"),
                   "--quit-after-steps".encode("utf-16-le")):
        assert marker not in binary, marker
    for pattern in ("*pixie-mcp*", "*PixieMcp*", "*ModelContextProtocol*", "*pixie-export*"):
        assert not list(published.rglob(pattern)), pattern
    publish("no-build-blocked", ["-p:PixieDevelopmentTools=false"], "existing editor binary", no_build=True)
    (run / "results.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    print(f"PASS: development publish rejected, clean local publish verified, NoBuild rejected. Evidence: {run}")


if __name__ == "__main__":
    main()
