"""Build local publish fixtures; no package or artifact is uploaded."""
import argparse
import json
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
    for marker in (b"ProjectExportJob", b"ExportPanel", b"PixieEditor.Development",
                   b"get_Export", "Export game...".encode("utf-16-le")):
        assert marker not in binary, marker
    assert not list(published.rglob("*pixie-mcp*")) and not list(published.rglob("*pixie-export*"))
    publish("no-build-blocked", ["-p:PixieDevelopmentTools=false"], "existing editor binary", no_build=True)
    (run / "results.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    print(f"PASS: development publish rejected, clean local publish verified, NoBuild rejected. Evidence: {run}")


if __name__ == "__main__":
    main()
