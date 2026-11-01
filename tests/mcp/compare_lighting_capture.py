"""Compare only the room inside the default 1600x900 editor layout, excluding Inspector text.

This bounded fixture requires the standard panel layout. It does not qualify custom layouts.
"""
import json
from pathlib import Path
import sys

from PIL import Image, ImageChops


def main():
    left, right, mode = sys.argv[1:]
    if mode not in ("equal", "different"):
        raise ValueError("mode must be equal or different")
    region = (360, 250, 1150, 700)
    with Image.open(left) as a, Image.open(right) as b:
        if a.size != (1600, 900) or b.size != a.size:
            raise ValueError("lighting editor fixture requires the standard 1600x900 capture")
        delta = ImageChops.difference(a.convert("RGB").crop(region), b.convert("RGB").crop(region))
        largest = max(high for low, high in delta.getextrema())
        red, green, blue = delta.split()
        changed = ImageChops.lighter(ImageChops.lighter(red, green), blue)
        differing = 1 - changed.histogram()[0] / (delta.width * delta.height)
        # Native undo may reconstruct geometry: permit isolated one-level raster rounding,
        # while requiring a substantial scene change for the off-state assertion.
        passed = (largest <= 1 and differing <= 0.00001) if mode == "equal" else (largest >= 8 and differing >= 0.001)
        detail = {"mode": mode, "region": region, "max_channel_difference": largest,
                  "differing_pixel_share": differing, "passed": passed}
        Path(right).with_suffix("." + mode + ".json").write_text(json.dumps(detail, indent=2))
        print(json.dumps(detail))
        return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
