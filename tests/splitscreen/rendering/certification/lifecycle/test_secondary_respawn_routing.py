from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[5]
SOURCE = (ROOT / "codemp/client/cl_input.cpp").read_text(encoding="utf-8")


class SecondaryRespawnRouting(unittest.TestCase):
    def test_rising_edge_is_latched_before_state_update(self):
        setter = re.search(
            r"void CL_SplitScreenSetControllerButton.*?\n\}", SOURCE, re.S
        ).group(0)
        latch = setter.index("pressed && !cl_splitScreenControllerButtons")
        update = setter.index("cl_splitScreenControllerButtons[player][button] = pressed")
        self.assertLess(latch, update)

    def test_attack_consumes_latched_edge(self):
        self.assertRegex(
            SOURCE,
            r'CL_SplitScreenCommandDown\( player, "\+attack" \) \|\|\s*'
            r'CL_SplitScreenCommandPressed\( player, "\+attack"',
        )

    def test_latch_is_cleared_after_command_generation(self):
        remember = re.search(
            r"static void CL_SplitScreenRememberControllerButtons.*?\n\}", SOURCE, re.S
        ).group(0)
        self.assertIn("cl_splitScreenControllerButtonPressed", remember)
        create = re.search(
            r"static void CL_SplitScreenCreateCmd\s*\([^;]*?\)\s*\{.*?\n\}", SOURCE, re.S
        ).group(0)
        apply_at = create.index("CL_SplitScreenApplyButtonBindings")
        self.assertLess(
            apply_at,
            create.index("CL_SplitScreenRememberControllerButtons", apply_at),
        )

    def test_held_attack_is_blocked_on_death_until_release(self):
        self.assertIn("cl_splitScreenAttackBlockedUntilRelease", SOURCE)
        transition = SOURCE.index("dead && !cl_splitScreenWasDead[player]")
        block = SOURCE.index("cl_splitScreenAttackBlockedUntilRelease[player] = qtrue", transition)
        release = SOURCE.index("!CL_SplitScreenCommandDown( player, \"+attack\" )", block)
        unblock = SOURCE.index("cl_splitScreenAttackBlockedUntilRelease[player] = qfalse", release)
        self.assertLess(transition, block)
        self.assertLess(release, unblock)


if __name__ == "__main__":
    unittest.main()
