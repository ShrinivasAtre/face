#!/usr/bin/env python3
"""Prepare private, session-grouped dense eye-ROI annotation templates."""

from __future__ import annotations

import argparse
import csv
import hashlib
from collections import defaultdict
from pathlib import Path


LABEL_FIELDS = (
    "clip_id", "frame", "timestamp_ms", "side", "visibility", "eye_state",
    "occluder", "quality", "annotator_id", "notes", "relative_path",
    "candidate_class",
)


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8-sig") as stream:
        return list(csv.DictReader(stream))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_name(value: str) -> str:
    return "".join(character if character.isalnum() or character in "-_" else "-" for character in value)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--clip-mapping", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--annotator-id", action="append", required=True)
    args = parser.parse_args()

    if args.output_dir.exists():
        raise SystemExit(f"refusing to overwrite existing output directory: {args.output_dir}")

    manifest = read_csv(args.manifest)
    mapping = read_csv(args.clip_mapping)
    if not manifest:
        raise SystemExit("candidate manifest has no rows")
    if not mapping:
        raise SystemExit("clip mapping has no rows")

    required_manifest = {"clip_id", "frame", "timestamp_ms", "side", "relative_path", "candidate_class"}
    required_mapping = {"clip_id", "subject_id", "session_id", "slice", "sha256"}
    if not required_manifest.issubset(manifest[0]):
        raise SystemExit("candidate manifest is missing required columns")
    if not required_mapping.issubset(mapping[0]):
        raise SystemExit("clip mapping is missing required columns")

    clip_metadata = {row["clip_id"]: row for row in mapping}
    grouped: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in manifest:
        if row["clip_id"] not in clip_metadata:
            raise SystemExit(f"no mapping row for clip {row['clip_id']}")
        metadata = clip_metadata[row["clip_id"]]
        grouped[(metadata["subject_id"], metadata["session_id"])].append(row)

    args.output_dir.mkdir(parents=True)
    checksum_path = args.output_dir / "frozen-input-checksums.txt"
    with checksum_path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(f"candidate_manifest_sha256={sha256(args.manifest)}\n")
        for row in sorted(mapping, key=lambda item: item["clip_id"]):
            stream.write(f"{row['clip_id']}_source_video_sha256={row['sha256']}\n")

    batch_rows: list[dict[str, object]] = []
    for annotator_id in args.annotator_id:
        if not annotator_id.strip():
            raise SystemExit("annotator ID must not be empty")
        annotator_dir = args.output_dir / safe_name(annotator_id)
        annotator_dir.mkdir()
        for (subject_id, session_id), rows in sorted(grouped.items()):
            rows.sort(key=lambda row: (row["clip_id"], int(row["frame"]), row["side"]))
            batch_id = safe_name(f"{subject_id}-{session_id}")
            output_path = annotator_dir / f"labels-{batch_id}.csv"
            with output_path.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, fieldnames=LABEL_FIELDS)
                writer.writeheader()
                for row in rows:
                    writer.writerow({
                        "clip_id": row["clip_id"],
                        "frame": row["frame"],
                        "timestamp_ms": row["timestamp_ms"],
                        "side": row["side"],
                        "visibility": "",
                        "eye_state": "",
                        "occluder": "",
                        "quality": "",
                        "annotator_id": annotator_id,
                        "notes": "",
                        "relative_path": row["relative_path"],
                        "candidate_class": row["candidate_class"],
                    })
            clips = sorted({row["clip_id"] for row in rows})
            batch_rows.append({
                "annotator_id": annotator_id,
                "batch_id": batch_id,
                "subject_id": subject_id,
                "session_id": session_id,
                "clip_ids": ";".join(clips),
                "crop_rows": len(rows),
                "template": output_path.relative_to(args.output_dir).as_posix(),
            })

    with (args.output_dir / "batch-manifest.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=batch_rows[0].keys())
        writer.writeheader()
        writer.writerows(batch_rows)

    print(f"PREPARED {len(batch_rows)} BATCHES / {len(manifest)} CROPS PER ANNOTATOR")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

