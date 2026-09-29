#!/usr/bin/env python3
from pathlib import Path
import unittest


RUNNER = Path(__file__).resolve().parent / "run.sh"


class MouseFocusRunnerContract(unittest.TestCase):
    def test_runner_is_frozen_and_fail_closed(self):
        source = RUNNER.read_text()
        self.assertIn("verify_frozen_artifacts.py", source)
        self.assertIn("737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13", source)
        self.assertNotIn("|| true", source)
        self.assertIn('[[ "$failures" -eq 0 ]]', source)

    def test_runner_supports_immutable_certification_root(self):
        self.assertIn("OPENJK_MOUSE_FOCUS_RESULTS", RUNNER.read_text())

    def test_restart_cases_reopen_the_ui_before_cursor_assertions(self):
        here = RUNNER.parent
        self.assertEqual(
            (here / "generate.py").read_text().count('"splitscreen_setup 1"'),
            3,
        )
        for players in (2, 3, 4):
            source = (here / f"mouse_focus_{players}p.cfg").read_text()
            self.assertEqual(
                source.count("vid_restart\nwait 360\nsplitscreen_setup 1\nwait 60"),
                2,
            )


if __name__ == "__main__":
    unittest.main()
