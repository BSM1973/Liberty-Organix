#!/usr/bin/env python3
"""Liberty Music Software UI text gate.

Reject damaged UTF-8, mojibake, non-printing controls and font-dependent
decorative glyphs inside C++ UI string literals. Legitimate French accents
are encouraged. This is a reusable guardrail, not a font replacement.
"""
from __future__ import annotations

import re
import sys
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source"
UI_FILES = [
    "PluginEditor.cpp",
    "OrganixRoutingSelector.h",
    "OrganixPresetBrowser.cpp",
    "OrganixMaterialButton.h",
    "OrganixI18n.h",
    "OrganixMatterDNAView.cpp",
    "OrganixMatterDNAView.h",
    "OrganixMorphView.cpp",
    "OrganixMorphView.h",
    "OrganixTrajectoryControls.cpp",
    "OrganixTrajectoryControls.h",
    "OrganixReactionPanel.cpp",
    "OrganixReactionPanel.h",
    "OrganixModMatrixView.cpp",
    "OrganixModMatrixView.h",
]
# These are known to render as missing-glyph boxes or stray symbols on some
# hosts/fonts. Use ASCII or draw vector icons rather than font characters.
DISALLOWED = {
    0x00B7: "middle dot",
    0x00D7: "multiplication sign",
    0x2018: "curly apostrophe",
    0x2019: "curly apostrophe",
    0x2013: "en dash",
    0x2014: "em dash",
    0x2026: "ellipsis",
    0x2190: "left arrow",
    0x2192: "right arrow",
    0x2605: "star",
    0x2606: "hollow star",
    0xFFFD: "replacement character",
    0x00A0: "non-breaking space",
}
STRING_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
errors: list[str] = []

for filename in UI_FILES:
    path = SOURCE / filename
    try:
        content = path.read_text(encoding="utf-8", errors="strict")
    except (OSError, UnicodeError) as exc:
        errors.append(f"{path.relative_to(ROOT)}: invalid UTF-8: {exc}")
        continue
    for number, line in enumerate(content.splitlines(), 1):
        if any(fragment in line for fragment in ("\ufffd", "Ã", "â€")):
            errors.append(f"{filename}:{number}: suspicious mojibake / replacement")
        for match in STRING_LITERAL.finditer(line):
            literal = match.group(0)
            if unicodedata.normalize("NFC", literal) != literal:
                errors.append(f"{filename}:{number}: use NFC for accented letters")
            for char in literal:
                point = ord(char)
                if point in DISALLOWED or (
                    unicodedata.category(char) == "Cf"
                ):
                    reason = DISALLOWED.get(point, "format/zero-width character")
                    errors.append(f"{filename}:{number}: forbidden {reason} U+{point:04X}")

if errors:
    print("FAIL: Liberty UI typography / Unicode preflight")
    for message in errors:
        print(" -", message)
    sys.exit(1)
print("PASS: valid UTF-8/NFC, real French accents, no fragile glyphs in UI labels")
