"""Generate review candidates; reference replacement requires an explicit --approve invocation."""
import argparse
import json
from pathlib import Path
import shutil

from snapshot_capture import capture, scenario_name


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sample", type=scenario_name)
    parser.add_argument("scenario", nargs="?", type=scenario_name)
    parser.add_argument("--config", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--manifest", help="optional external candidate manifest; no repository references are implied")
    parser.add_argument("--approve", action="store_true",
                        help="replace references only after the owner has approved the candidates")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    sample = root / "samples" / args.sample
    manifest = Path(args.manifest).resolve() if args.manifest else sample / "snapshots/scenarios.json"
    document = json.loads(manifest.read_text(encoding="utf-8"))
    scenarios = document["scenarios"]
    for name in ([args.scenario] if args.scenario else scenarios):
        scenario_name(name)
        scenario = scenarios[name]
        output = root / "build/snapshot-candidates" / args.sample / name
        player = root / "bin" / args.config / (args.sample + ".exe")
        if not player.is_file() and not (sample / "CMakeLists.txt").is_file():
            player = root / "bin" / args.config / "pixie-player.exe"
        actual, receipt = capture(player, sample, scenario, output)
        if args.approve:
            reference = (manifest.parent / scenario["reference"]).resolve()
            if not reference.is_relative_to(manifest.parent.resolve()):
                raise ValueError("reference must remain inside the snapshots folder")
            shutil.copyfile(actual, reference)
            scenario["renderer"] = receipt["renderer"]
            scenario["approved"] = True
        print(f"Candidate: {actual}")
    if args.approve:
        manifest.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
