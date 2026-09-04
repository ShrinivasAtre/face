#!/usr/bin/env python3
"""Validate private dense eye-ROI labels against an extractor manifest."""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path


REQUIRED = (
    "clip_id", "frame", "timestamp_ms", "side", "visibility", "eye_state",
    "occluder", "quality", "annotator_id", "notes",
)
ALLOWED = {
    "side": {"left", "right"},
    "visibility": {"visible", "partial", "occluded", "invalid", "uncertain"},
    "eye_state": {"open", "closed", "transition", "unknown"},
    "occluder": {"none", "hand", "object", "glasses", "other", "unknown"},
    "quality": {"accepted", "uncertain", "exclude"},
}


def rows(path: Path) -> tuple[list[str], list[dict[str, str]]]:
    with path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        return list(reader.fieldnames or []), list(reader)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("labels", type=Path)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()

    fields, labels = rows(args.labels)
    missing_columns = [column for column in REQUIRED if column not in fields]
    if missing_columns:
        print("missing columns: " + ", ".join(missing_columns), file=sys.stderr)
        return 1
    _, manifest = rows(args.manifest)
    manifest_keys = {(row["clip_id"], row["frame"], row["side"]): row for row in manifest}
    seen: set[tuple[str, str, str]] = set()
    errors: list[str] = []
    for line, row in enumerate(labels, start=2):
        key = (row["clip_id"], row["frame"], row["side"])
        if key in seen:
            errors.append(f"line {line}: duplicate crop key")
        seen.add(key)
        if key not in manifest_keys:
            errors.append(f"line {line}: crop key is absent from manifest")
        else:
            expected = float(manifest_keys[key]["timestamp_ms"])
            try:
                actual = float(row["timestamp_ms"])
                if abs(actual - expected) > 1.0:
                    errors.append(f"line {line}: timestamp differs from manifest by more than 1 ms")
            except ValueError:
                errors.append(f"line {line}: timestamp_ms is not numeric")
        for column, allowed in ALLOWED.items():
            if row[column] not in allowed:
                errors.append(f"line {line}: invalid {column}={row[column]!r}")
        if not row["annotator_id"].strip():
            errors.append(f"line {line}: annotator_id is empty")
        if row["visibility"] == "visible" and row["occluder"] not in {"none", "glasses"}:
            errors.append(f"line {line}: visible eye has incompatible occluder")
        if row["visibility"] in {"occluded", "invalid", "uncertain"} and row["eye_state"] != "unknown":
            errors.append(f"line {line}: non-observable eye_state must be unknown")

    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        print(f"LABEL VALIDATION FAILED: {len(errors)} error(s)", file=sys.stderr)
        return 1
    print(f"LABEL VALIDATION PASSED: {len(labels)} row(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
