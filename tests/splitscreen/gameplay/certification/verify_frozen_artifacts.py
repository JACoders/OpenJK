#!/usr/bin/env python3
"""Strictly verify the split-screen certification artifact freeze."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path, PurePosixPath
from typing import Any


FORMAT = "openjk-splitscreen-frozen-build-v1"
ARTIFACT_COUNT = 10
CLIENT_ARTIFACT = (
    "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
)
MANIFEST_RELATIVE_PATH = Path(
    "docs/splitscreen-gameplay-hardening/"
    "phase-5-frozen-certification/frozen-build.json"
)
SHA256_RE = re.compile(r"[0-9a-f]{64}")


class FrozenArtifactError(RuntimeError):
    """Raised when a frozen artifact manifest or artifact is invalid."""


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def _load_manifest(manifest_path: Path) -> dict[str, Any]:
    try:
        with manifest_path.open("r", encoding="utf-8") as stream:
            manifest = json.load(stream)
    except FileNotFoundError as exc:
        raise FrozenArtifactError(f"missing manifest: {manifest_path}") from exc
    except (OSError, json.JSONDecodeError) as exc:
        raise FrozenArtifactError(f"invalid manifest: {manifest_path}: {exc}") from exc

    if not isinstance(manifest, dict):
        raise FrozenArtifactError("manifest root must be a JSON object")
    if manifest.get("format") != FORMAT:
        raise FrozenArtifactError(
            f"unexpected manifest format: {manifest.get('format')!r}"
        )
    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, dict):
        raise FrozenArtifactError("manifest artifacts must be a JSON object")
    if len(artifacts) != ARTIFACT_COUNT:
        raise FrozenArtifactError(
            f"manifest must list exactly {ARTIFACT_COUNT} artifacts, "
            f"found {len(artifacts)}"
        )
    if CLIENT_ARTIFACT not in artifacts:
        raise FrozenArtifactError(
            f"manifest is missing client artifact: {CLIENT_ARTIFACT}"
        )
    return manifest


def _resolve_artifact(repo_root: Path, manifest_name: object) -> tuple[str, Path]:
    if not isinstance(manifest_name, str) or not manifest_name:
        raise FrozenArtifactError(f"invalid artifact path: {manifest_name!r}")

    pure_path = PurePosixPath(manifest_name)
    if pure_path.is_absolute() or ".." in pure_path.parts:
        raise FrozenArtifactError(f"artifact path escapes repository: {manifest_name}")
    if "\\" in manifest_name or pure_path.as_posix() != manifest_name:
        raise FrozenArtifactError(f"artifact path is not normalized: {manifest_name}")

    root = repo_root.resolve(strict=True)
    candidate = root.joinpath(*pure_path.parts)
    try:
        resolved = candidate.resolve(strict=True)
    except FileNotFoundError as exc:
        raise FrozenArtifactError(f"missing artifact: {manifest_name}") from exc
    except OSError as exc:
        raise FrozenArtifactError(f"cannot resolve artifact: {manifest_name}: {exc}") from exc

    try:
        resolved.relative_to(root)
    except ValueError as exc:
        raise FrozenArtifactError(f"artifact path escapes repository: {manifest_name}") from exc
    if not resolved.is_file():
        raise FrozenArtifactError(f"artifact is not a regular file: {manifest_name}")
    return manifest_name, resolved


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as exc:
        raise FrozenArtifactError(f"cannot read artifact: {path}: {exc}") from exc
    return digest.hexdigest()


def verify_frozen_artifacts(
    repo_root: Path,
    manifest_path: Path,
    expected_client_hash: str | None = None,
) -> str:
    """Verify a freeze manifest and return its verified client SHA-256."""

    root = repo_root.resolve(strict=True)
    manifest = _load_manifest(manifest_path)
    artifacts = manifest["artifacts"]

    if expected_client_hash is not None and not SHA256_RE.fullmatch(
        expected_client_hash
    ):
        raise FrozenArtifactError(
            "expected client hash must be 64 lowercase hexadecimal characters"
        )

    for manifest_name, expected_hash in artifacts.items():
        if not isinstance(expected_hash, str) or not SHA256_RE.fullmatch(expected_hash):
            raise FrozenArtifactError(
                f"invalid SHA-256 for artifact {manifest_name!r}: {expected_hash!r}"
            )
        name, artifact_path = _resolve_artifact(root, manifest_name)
        actual_hash = _sha256(artifact_path)
        if actual_hash != expected_hash:
            raise FrozenArtifactError(
                f"stale artifact: {name}: expected {expected_hash}, got {actual_hash}"
            )

    client_hash = artifacts[CLIENT_ARTIFACT]
    if expected_client_hash is not None and client_hash != expected_client_hash:
        raise FrozenArtifactError(
            f"wrong client hash: expected {expected_client_hash}, "
            f"manifest has {client_hash}"
        )
    return client_hash


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Verify all frozen split-screen certification artifacts."
    )
    parser.add_argument(
        "--expected-client-hash",
        help="also require this exact lowercase SHA-256 for the client executable",
    )
    args = parser.parse_args(argv)

    repo_root = _repo_root()
    manifest_path = repo_root / MANIFEST_RELATIVE_PATH
    try:
        client_hash = verify_frozen_artifacts(
            repo_root, manifest_path, args.expected_client_hash
        )
    except (FrozenArtifactError, OSError) as exc:
        print(f"FROZEN_ARTIFACTS_FAIL {exc}", file=sys.stderr)
        return 1

    print(
        f"FROZEN_ARTIFACTS_PASS artifacts={ARTIFACT_COUNT} client={client_hash}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
