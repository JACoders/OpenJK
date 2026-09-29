#!/usr/bin/env python3
import copy
import json
import tempfile
import unittest
from pathlib import Path
import re

import generate


class CoverageLedgerTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(generate.DEFAULT_SOURCE.read_text())

    def test_current_ledger_validates(self):
        generate.validate(self.data)

    def test_generated_markdown_links_exist(self):
        output = generate.build(self.data)
        text = generate.markdown(output)
        links = re.findall(r"\[[^\]]+\]\(([^)]+)\)", text)
        self.assertTrue(links)
        for link in links:
            self.assertTrue((generate.DEFAULT_MD.parent / link).resolve().is_file(), link)

    def test_legacy_imports_cannot_qualify_seal(self):
        output = generate.build(self.data)
        claimed = [cell for cell in output["cells"] if cell["evidence"]]
        self.assertTrue(claimed)
        self.assertTrue(all(not cell["seal_eligible"] for cell in claimed))

    def test_missing_artifact_fails(self):
        bad = copy.deepcopy(self.data)
        bad["evidence"][0]["log"]["path"] = "does/not/exist.log"
        with self.assertRaisesRegex(generate.LedgerError, "missing log"):
            generate.validate(bad)

    def test_stale_artifact_fails(self):
        bad = copy.deepcopy(self.data)
        bad["evidence"][0]["log"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(generate.LedgerError, "stale log"):
            generate.validate(bad)

    def test_wrong_build_fails(self):
        bad = copy.deepcopy(self.data)
        bad["build"]["binary_sha256"] = "f" * 64
        with self.assertRaisesRegex(generate.LedgerError, "stale binary"):
            generate.validate(bad)

    def test_missing_build_correlation_fails(self):
        bad = copy.deepcopy(self.data)
        del bad["build_correlation"]
        with self.assertRaisesRegex(generate.LedgerError, "build correlation"):
            generate.validate(bad)

    def test_missing_marker_fails(self):
        bad = copy.deepcopy(self.data)
        bad["evidence"][0]["claims"]["score"] = "not present in log"
        with self.assertRaisesRegex(generate.LedgerError, "missing assertion marker"):
            generate.validate(bad)

    def test_contradictory_claim_fails(self):
        bad = copy.deepcopy(self.data)
        duplicate = copy.deepcopy(bad["evidence"][0])
        duplicate["id"] = "contradiction"
        duplicate["claims"]["spawn"] = "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE"
        bad["evidence"].append(duplicate)
        with self.assertRaisesRegex(generate.LedgerError, "contradictory claim"):
            generate.validate(bad)


if __name__ == "__main__":
    unittest.main(verbosity=2)
