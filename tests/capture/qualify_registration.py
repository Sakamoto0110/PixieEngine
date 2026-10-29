"""Qualify CTest registration with synthetic manifests; no approved product reference is created."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--ctest", required=True)
    parser.add_argument("--work", required=True)
    args = parser.parse_args()
    engine = Path(__file__).resolve().parents[2]
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="registration-", dir=work))
    records = []
    for name, approved, code, target in (("code-approved", True, True, True),
                                         ("candidate", False, True, True),
                                         ("code-target-missing", True, True, False),
                                         ("scene-approved", True, False, False)):
        project = run / name
        snapshots = project / "samples/probe/snapshots"
        snapshots.mkdir(parents=True)
        (snapshots / "scenarios.json").write_text(json.dumps({"scenarios": {"fixture": {"approved": approved}}}), encoding="utf-8")
        if code:
            (snapshots.parent / "CMakeLists.txt").write_text("# Synthetic code project\n", encoding="utf-8")
        content = f'''cmake_minimum_required(VERSION 3.25)
project(SnapshotRegistration NONE)
enable_testing()
add_executable(pixie-player IMPORTED GLOBAL)
set_target_properties(pixie-player PROPERTIES IMPORTED_LOCATION "${{CMAKE_SOURCE_DIR}}/generic.exe")
'''
        if target:
            content += '''add_executable(probe IMPORTED GLOBAL)
set_target_properties(probe PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/code.exe")
'''
        content += f'include("{(engine / "cmake/PixieSnapshots.cmake").as_posix()}")\npixie_add_snapshot_tests()\n'
        (project / "CMakeLists.txt").write_text(content, encoding="utf-8")
        result = subprocess.run([args.cmake, "-S", str(project), "-B", str(project / "build")],
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
        (project / "configure.log").write_text(result.stdout, encoding="utf-8")
        if code and approved and not target:
            assert result.returncode != 0 and "enable its sample build" in result.stdout, result.stdout
        else:
            assert result.returncode == 0, result.stdout
            result = subprocess.run([args.ctest, "--test-dir", str(project / "build"), "-C", "Release", "--show-only=json-v1"],
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
            assert result.returncode == 0, result.stderr
            tests = json.loads(result.stdout)["tests"]
            assert len(tests) == (1 if approved else 0), tests
            if approved:
                command = tests[0]["command"]
                assert Path(command[command.index("--player") + 1]).name == ("code.exe" if target else "generic.exe"), tests
        records.append({"case": name, "passed": True, "scope": "synthetic registration, no image approval"})
    (run / "results.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    print(f"PASS: four snapshot registration cases. Evidence: {run}")


if __name__ == "__main__":
    main()
