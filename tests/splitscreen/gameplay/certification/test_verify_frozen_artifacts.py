#!/usr/bin/env python3
"""Offline tests for strict frozen-artifact verification."""

from __future__ import annotations

import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from verify_frozen_artifacts import (
    ARTIFACT_COUNT,
    CLIENT_ARTIFACT,
    FORMAT,
    FrozenArtifactError,
    verify_frozen_artifacts,
)


class FrozenArtifactVerificationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)
        artifact_names = [CLIENT_ARTIFACT] + [
            f"build-x86_64/test/artifact-{index}.bin"
            for index in range(1, ARTIFACT_COUNT)
        ]
        self.artifacts: dict[str, str] = {}
        for index, artifact_name in enumerate(artifact_names):
            path = self.root / artifact_name
            path.parent.mkdir(parents=True, exist_ok=True)
            payload = f"frozen-artifact-{index}\n".encode()
            path.write_bytes(payload)
            self.artifacts[artifact_name] = hashlib.sha256(payload).hexdigest()

        self.manifest_path = self.root / "frozen-build.json"
        self._write_manifest()

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def _write_manifest(self, artifacts: dict[str, str] | None = None) -> None:
        manifest = {
            "format": FORMAT,
            "frozen_utc": "2026-07-23T00:00:00Z",
            "source_commit": "0" * 40,
            "artifacts": self.artifacts if artifacts is None else artifacts,
        }
        self.manifest_path.write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
        )

    def test_valid_freeze_and_expected_client_hash_pass(self) -> None:
        client_hash = verify_frozen_artifacts(
            self.root, self.manifest_path, self.artifacts[CLIENT_ARTIFACT]
        )
        self.assertEqual(client_hash, self.artifacts[CLIENT_ARTIFACT])

    def test_mutated_artifact_is_stale(self) -> None:
        (self.root / CLIENT_ARTIFACT).write_bytes(b"mutated\n")
        with self.assertRaisesRegex(FrozenArtifactError, "stale artifact"):
            verify_frozen_artifacts(self.root, self.manifest_path)

    def test_missing_artifact_is_rejected(self) -> None:
        missing_name = next(
            name for name in self.artifacts if name != CLIENT_ARTIFACT
        )
        (self.root / missing_name).unlink()
        with self.assertRaisesRegex(FrozenArtifactError, "missing artifact"):
            verify_frozen_artifacts(self.root, self.manifest_path)

    def test_wrong_expected_client_hash_is_rejected(self) -> None:
        wrong_hash = "f" * 64
        self.assertNotEqual(wrong_hash, self.artifacts[CLIENT_ARTIFACT])
        with self.assertRaisesRegex(FrozenArtifactError, "wrong client hash"):
            verify_frozen_artifacts(
                self.root, self.manifest_path, expected_client_hash=wrong_hash
            )

    def test_parent_path_escape_is_rejected(self) -> None:
        escaped = dict(self.artifacts)
        victim = next(name for name in escaped if name != CLIENT_ARTIFACT)
        escaped["../outside.bin"] = escaped.pop(victim)
        self._write_manifest(escaped)
        with self.assertRaisesRegex(FrozenArtifactError, "escapes repository"):
            verify_frozen_artifacts(self.root, self.manifest_path)


if __name__ == "__main__":
    unittest.main()
