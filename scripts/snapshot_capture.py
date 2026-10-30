"""Development-only capture/comparison helpers. Requires Pillow; never writes approved references."""
import argparse
import json
from pathlib import Path
import re
import subprocess

from PIL import Image, ImageChops


def scenario_name(value):
    if not re.fullmatch(r"[A-Za-z0-9_-]+", value):
        raise ValueError("scenario and sample names must contain only letters, digits, '_' or '-'")
    return value


def capture(player, project, scenario, output):
    output.mkdir(parents=True, exist_ok=True)
    image = output / "actual.png"
    image.unlink(missing_ok=True)  # Only this invocation's known output, never a reference.
    command = [str(Path(player).resolve()), "--quit-after-steps", str(scenario["steps"]),
               "--seed", str(scenario["seed"]), "--size", scenario["size"],
               "--vsync", "off", "--screenshot", str(image.resolve())]
    if project:
        command += ["--project", str(Path(project).resolve())]
    if "fps" in scenario:
        command += ["--fps", str(scenario["fps"])]
    if "quit_after" in scenario:
        command += ["--quit-after", str(scenario["quit_after"])]
    result = subprocess.run(command, text=True, encoding="utf-8", errors="replace",
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    (output / "capture.log").write_text(result.stdout, encoding="utf-8")
    if result.returncode or not image.is_file():
        raise RuntimeError(f"capture failed ({result.returncode}); see {output / 'capture.log'}")
    renderer = re.search(r"GL_RENDERER: (.+)", result.stdout)
    with Image.open(image) as pixels:
        expected = tuple(int(x) for x in scenario["size"].split("x"))
        if pixels.size != expected:
            raise RuntimeError(f"capture size {pixels.size} differs from {expected}")
    receipt = {"command": command, "renderer": renderer.group(1).strip() if renderer else None,
               "steps": scenario["steps"], "seed": scenario["seed"], "size": scenario["size"]}
    (output / "capture.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
    return image, receipt


def compare(actual, reference, tolerance, output):
    # Both limits apply: the largest channel delta, and the share of pixels with any delta.
    channel_limit = tolerance["max_channel_difference"]
    share_limit = tolerance["max_differing_pixel_share"]
    if not 0 <= channel_limit <= 255 or not 0 <= share_limit <= 1:
        raise ValueError("invalid per-scenario tolerance")
    with Image.open(actual) as left, Image.open(reference) as right:
        if left.size != right.size:
            result = {"passed": False, "reason": "image dimensions differ"}
            diff = Image.new("RGB", (max(left.width, right.width), max(left.height, right.height)))
            diff.paste(left.convert("RGB"))
            diff.save(output / "diff.png")
        else:
            delta = ImageChops.difference(left.convert("RGBA"), right.convert("RGBA"))
            largest = max(high for low, high in delta.getextrema())
            differing = sum(any(pixel) for pixel in delta.getdata()) / (left.width * left.height)
            result = {"passed": largest <= channel_limit and differing <= share_limit,
                      "max_channel_difference": largest, "differing_pixel_share": differing}
            if not result["passed"]:
                red, green, blue, alpha = delta.split()
                visible = Image.merge("RGB", tuple(ImageChops.lighter(band, alpha) for band in (red, green, blue)))
                visible.point(lambda value: min(value * 4, 255)).save(output / "diff.png")
    (output / "comparison.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--player", required=True)
    parser.add_argument("--project")
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    manifest = Path(args.manifest).resolve()
    scenario = json.loads(manifest.read_text(encoding="utf-8"))["scenarios"][scenario_name(args.scenario)]
    if scenario.get("approved") is not True:
        raise ValueError("snapshot tests require an explicitly approved reference")
    reference = (manifest.parent / scenario["reference"]).resolve()
    if not reference.is_relative_to(manifest.parent):
        raise ValueError("reference must remain inside the snapshots folder")
    output = Path(args.output).resolve()
    if output == manifest.parent or output.is_relative_to(manifest.parent):
        raise ValueError("test output must remain outside the snapshots folder")
    actual, receipt = capture(args.player, args.project, scenario, output)
    if receipt["renderer"] != scenario["renderer"]:
        print(f"Renderer differs: actual={receipt['renderer']!r}, approved={scenario['renderer']!r}")
    result = compare(actual, reference, scenario["tolerance"], output)
    if not result["passed"]:
        print(f"Snapshot differs. Actual: {actual}; diff: {output / 'diff.png'}")
        return 1
    print(f"Snapshot passed: {actual}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
