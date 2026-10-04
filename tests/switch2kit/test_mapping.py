#!/usr/bin/env python3
"""Static default-mapping regressions, not SDL, Bluetooth or gameplay tests."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Switch2KitMappingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "src/input/api/SDL/Switch2KitMapping.h").read_text(encoding="utf-8")
        cls.source = re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.DOTALL)

    def binding(self, button):
        entries = re.findall(
            r"\{\s*Pad::kButtonId_" + re.escape(button) + r"\s*,\s*([^{}]+?)\s*\}",
            self.source,
        )
        self.assertEqual(len(entries), 1, f"Expected one default binding for {button}")
        return " ".join(entries[0].split())

    def test_gamecube_z_supplies_r_without_changing_other_models(self):
        self.assertRegex(self.source, r"const\s+bool\s+gc\s*=\s*model\s*==\s*S2K_GAMECUBE\s*;")
        self.assertEqual(
            self.binding("R"),
            "gc ? SDL_GAMEPAD_BUTTON_MISC4 : SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER",
        )
        # The left Joy-Con must not acquire right-side mappings.
        self.assertRegex(self.source, r"const\s+bool\s+right\s*=\s*model\s*!=\s*S2K_JOYCON_LEFT\s*;")
        self.assertRegex(self.source, r"if\s*\(right\)\s*\{\s*mapping\s*=\s*\{")

    def test_trigger_axes_stay_independent_of_z(self):
        self.assertEqual(self.binding("ZR"), "kTriggerYP")
        self.assertEqual(self.binding("ZL"), "kTriggerXP")
        self.assertEqual(self.binding("L"), "SDL_GAMEPAD_BUTTON_LEFT_SHOULDER")
        # Misc 4 supplies only R, never ZR or another emulated control.
        self.assertEqual(len(re.findall(r"\bSDL_GAMEPAD_BUTTON_MISC4\b", self.source)), 1)

    def test_printed_face_button_mappings_are_unchanged(self):
        for button, gc, other in (("A", "SOUTH", "EAST"), ("B", "WEST", "SOUTH"),
                                  ("X", "EAST", "NORTH"), ("Y", "NORTH", "WEST")):
            with self.subTest(button=button):
                self.assertEqual(
                    self.binding(button),
                    f"gc ? SDL_GAMEPAD_BUTTON_{gc} : SDL_GAMEPAD_BUTTON_{other}",
                )


if __name__ == "__main__":
    unittest.main(verbosity=2)
