#!/usr/bin/env python3
"""Audit a private eye-ROI candidate set without emitting identity data.

The input manifest is produced by the C++ eye-crop extractor. Output contains
only anonymous clip/class aggregates. An optional contact sheet is private
review material and must remain outside Git.
"""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw


def quantiles(values: list[float]) -> dict[str, float]:
    if not values:
        return {}
    array = np.asarray(values, dtype=np.float64)
    return {
        "min": round(float(np.min(array)), 4),
        "p05": round(float(np.quantile(array, 0.05)), 4),
        "median": round(float(np.median(array)), 4),
        "p95": round(float(np.quantile(array, 0.95)), 4),
        "max": round(float(np.max(array)), 4),
    }


def image_metrics(path: Path) -> tuple[tuple[int, int], dict[str, float]]:
    with Image.open(path) as source:
        gray = np.asarray(source.convert("L"), dtype=np.float32)
        size = source.size
    horizontal = np.diff(gray, axis=1)
    vertical = np.diff(gray, axis=0)
    return size, {
        "mean": float(np.mean(gray)),
        "contrast": float(np.std(gray)),
        "gradient": float((np.mean(np.abs(horizontal)) + np.mean(np.abs(vertical))) / 2.0),
        "dark_fraction": float(np.mean(gray <= 10.0)),
        "bright_fraction": float(np.mean(gray >= 245.0)),
    }


def aggregate(records: list[dict]) -> dict:
    return {
        "count": len(records),
        "brightness": quantiles([item["metrics"]["mean"] for item in records]),
        "contrast": quantiles([item["metrics"]["contrast"] for item in records]),
        "gradient": quantiles([item["metrics"]["gradient"] for item in records]),
        "dark_fraction": quantiles([item["metrics"]["dark_fraction"] for item in records]),
        "bright_fraction": quantiles([item["metrics"]["bright_fraction"] for item in records]),
        "low_contrast_count": sum(item["metrics"]["contrast"] < 12.0 for item in records),
        # This descriptive screen is intentionally conservative. Gradient is
        # camera/domain dependent and is not a blur ground-truth label.
        "very_low_gradient_count": sum(item["metrics"]["gradient"] < 1.0 for item in records),
        "dark_clipped_count": sum(item["metrics"]["dark_fraction"] > 0.25 for item in records),
        "bright_clipped_count": sum(item["metrics"]["bright_fraction"] > 0.25 for item in records),
    }


def evenly_spaced(records: list[dict], count: int) -> list[dict]:
    if len(records) <= count:
        return records
    indexes = np.linspace(0, len(records) - 1, count, dtype=int)
    return [records[int(index)] for index in indexes]


def write_contact_sheet(groups: dict[str, list[dict]], output: Path, samples: int) -> None:
    tile_width, tile_height = 192, 150
    selected = [(name, item) for name in sorted(groups)
                for item in evenly_spaced(groups[name], samples)]
    columns = max(1, min(samples, 8))
    rows = (len(selected) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * tile_width, rows * tile_height), "#202020")
    draw = ImageDraw.Draw(sheet)
    for index, (name, item) in enumerate(selected):
        x = (index % columns) * tile_width
        y = (index // columns) * tile_height
        with Image.open(item["path"]) as source:
            crop = source.convert("RGB")
            crop.thumbnail((tile_width - 8, 100))
            sheet.paste(crop, (x + 4, y + 4))
        draw.text((x + 4, y + 108), name, fill="white")
        draw.text((x + 4, y + 126), f'{item["clip_id"]} {item["side"]}', fill="#b8d8ff")
    output.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--contact-sheet", type=Path)
    parser.add_argument("--samples-per-class", type=int, default=8)
    args = parser.parse_args()
    if args.samples_per_class < 1:
        parser.error("--samples-per-class must be positive")

    root = args.manifest.resolve().parent
    records: list[dict] = []
    missing: list[str] = []
    invalid_dimensions = 0
    with args.manifest.open(newline="", encoding="utf-8-sig") as stream:
        for row in csv.DictReader(stream):
            path = root / row["relative_path"]
            if not path.is_file():
                missing.append(row["relative_path"])
                continue
            size, metrics = image_metrics(path)
            if size != (128, 80):
                invalid_dimensions += 1
            records.append({
                "clip_id": row["clip_id"],
                "side": row["side"],
                "candidate_class": row["candidate_class"],
                "path": path,
                "metrics": metrics,
            })

    by_class: dict[str, list[dict]] = defaultdict(list)
    by_clip: dict[str, list[dict]] = defaultdict(list)
    by_side: dict[str, list[dict]] = defaultdict(list)
    for record in records:
        by_class[record["candidate_class"]].append(record)
        by_clip[record["clip_id"]].append(record)
        by_side[record["side"]].append(record)

    report = {
        "schema_version": 1,
        "privacy": "anonymous aggregate; input crops and contact sheet remain private",
        "crop_count": len(records),
        "pair_count": len(records) // 2,
        "missing_file_count": len(missing),
        "invalid_dimension_count": invalid_dimensions,
        "overall": aggregate(records),
        "by_candidate_class": {name: aggregate(items) for name, items in sorted(by_class.items())},
        "by_clip": {name: aggregate(items) for name, items in sorted(by_clip.items())},
        "by_side": {name: aggregate(items) for name, items in sorted(by_side.items())},
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.contact_sheet:
        write_contact_sheet(by_class, args.contact_sheet, args.samples_per_class)
    print(f'Audited {len(records)} crops; missing={len(missing)}; invalid_dimensions={invalid_dimensions}')
    return 0 if not missing and invalid_dimensions == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
