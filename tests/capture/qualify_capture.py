"""Exercise development capture and comparison using retained synthetic fixtures, never goldens."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from snapshot_capture import capture, compare


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", required=True)
    parser.add_argument("--work", required=True)
    parser.add_argument("--editor")
    args = parser.parse_args()
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="run-", dir=work))
    records = []

    def record(value):
        records.append(value)
        (run / "results.json").write_text(json.dumps(records, indent=2), encoding="utf-8")

    strict = {"max_channel_difference": 0, "max_differing_pixel_share": 0}
    for sample in ("boids", "flocks", "particles", "sparks"):
        scenario = {"steps": 12, "seed": 42, "size": "320x180"}
        player = Path(args.bin).resolve() / (sample + ".exe")
        first, receipt = capture(player, ROOT / "samples" / sample, scenario, run / (sample + "-a"))
        second, _ = capture(player, ROOT / "samples" / sample, dict(scenario, fps=30, quit_after=.001), run / (sample + "-b"))
        for image in (first, second):
            log = (image.parent / "capture.log").read_text(encoding="utf-8")
            assert "Completed 12 fixed steps; capture alpha: 1" in log
        result = compare(first, second, strict, run / (sample + "-b"))
        assert result["passed"], (sample, result)
        record({"case": sample + "-repeat", "sha256": hashlib.sha256(first.read_bytes()).hexdigest(), **receipt})

    # Remove the authored seed only in this synthetic project to observe the initial seed option.
    project = run / "seed-project"
    (project / "scenes").mkdir(parents=True)
    (project / "boids.pixie").write_bytes((ROOT / "samples/boids/boids.pixie").read_bytes())
    scene = json.loads((ROOT / "samples/boids/scenes/main.scene").read_text(encoding="utf-8"))
    scene.pop("seed")
    (project / "scenes/main.scene").write_text(json.dumps(scene), encoding="utf-8")
    player = Path(args.bin).resolve() / "boids.exe"
    scenario = {"steps": 12, "seed": 42, "size": "320x180"}
    first, _ = capture(player, project, scenario, run / "seed42")
    repeated, _ = capture(player, project, scenario, run / "seed42-repeat")
    other, _ = capture(player, project, dict(scenario, seed=43), run / "seed43")
    other_step, _ = capture(player, project, dict(scenario, steps=13), run / "step13")
    assert first.read_bytes() == repeated.read_bytes()
    assert first.read_bytes() != other.read_bytes()
    assert first.read_bytes() != other_step.read_bytes()
    record({"case": "initial-seed-repeat-and-change", "passed": True,
            "scope": "synthetic scene without authored seed"})

    with Image.open(first) as image:
        changed = image.convert("RGBA")
    red, green, blue, alpha = changed.getpixel((0, 0))
    changed.putpixel((0, 0), ((red + 1) % 256, green, blue, alpha))
    mutation = run / "seed42/mutated.png"
    changed.save(mutation)
    assert not compare(mutation, first, strict, first.parent)["passed"]
    assert (first.parent / "diff.png").is_file()
    assert compare(first, first, strict, first.parent)["passed"]
    record({"case": "pixel-mutation-rejected-clean-restored", "passed": True})

    if args.editor:
        images = []
        for index in range(2):
            output = run / f"editor-{index}.png"
            result = subprocess.run([str(Path(args.editor).resolve()), "--game", "boids", "--seed", "42",
                                     "--quit-after-steps", "12", "--quit-after", ".001",
                                     "--screenshot", str(output), "--recent", "off"],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, encoding="utf-8", errors="replace", timeout=60)
            (run / f"editor-{index}.log").write_text(result.stdout, encoding="utf-8")
            assert result.returncode == 0 and re.search(r"in 12 steps", result.stdout), result.stdout
            images.append(output.read_bytes())
        assert images[0] == images[1], "editor repeat differs"
        record({"case": "editor-repeat", "sha256": hashlib.sha256(images[0]).hexdigest()})
    print(f"PASS: {len(records)} capture cases. Evidence: {run}")


if __name__ == "__main__":
    main()
