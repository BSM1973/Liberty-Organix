#!/usr/bin/env python3
"""Organix UI typography preflight.

Catch damaged UTF-8, mojibake, decomposed French accents and font-dependent
symbol characters before the AU/VST3 builds. This is a static gate; a visual
review inside real DAWs is still required for platform font fallback.
"""
from __future__ import annotations

import re
import sys
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source"

# Discover newly added Organix UI pages automatically. Also scan shared
# Organix components that can contribute string data to the visible UI.
UI_FILES = sorted(
    path for path in SOURCE.iterdir()
    if path.is_file()
    and path.suffix in {".cpp", ".h"}
    and (path.name.startswith("Organix") or path.name.startswith("PluginEditor"))
)

# Plain ASCII symbols and vector shapes drawn by JUCE are reliable across hosts.
# Characters in these ranges may turn into tofu/boxes in DAW menu fonts.
DISALLOWED = {
    0x00A0: "non-breaking space",
    0x00B7: "middle dot",
    0x00D7: "multiplication sign",
    0x2018: "curly apostrophe",
    0x2019: "curly apostrophe",
    0x2013: "en dash",
    0x2014: "em dash",
    0x2026: "ellipsis",
    0xFFFD: "replacement character",
}
FRENCH_LETTERS = set(
    "ÀÂÄÆÇÉÈÊËÎÏÔÖŒÙÛÜŸàâäæçéèêëîïôöœùûüÿ"
)
KNOWN_SPELLING = {"Francais": "Français", "ALEATOIRE": "ALÉATOIRE"}
STRING_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
errors: list[str] = []

if not UI_FILES:
    errors.append("No Organix source files found: UI typography gate did not run")

for path in UI_FILES:
    filename = path.relative_to(ROOT)
    try:
        content = path.read_text(encoding="utf-8", errors="strict")
    except (OSError, UnicodeError) as exc:
        errors.append(f"{filename}: invalid UTF-8: {exc}")
        continue
    if content.startswith("\ufeff"):
        errors.append(f"{filename}: remove UTF-8 BOM")
    for number, line in enumerate(content.splitlines(), 1):
        if any(fragment in line for fragment in ("\ufffd", "Ã", "â€")):
            errors.append(f"{filename}:{number}: mojibake or replacement character")
        for match in STRING_LITERAL.finditer(line):
            literal = match.group(0)
            if unicodedata.normalize("NFC", literal) != literal:
                errors.append(f"{filename}:{number}: normalize accents to NFC")
            for spelling, correction in KNOWN_SPELLING.items():
                if re.search(rf"(?<![A-Za-z]){re.escape(spelling)}(?![A-Za-z])", literal):
                    errors.append(
                        f"{filename}:{number}: use '{correction}', not '{spelling}'"
                    )
            for char in literal:
                point = ord(char)
                if point in DISALLOWED:
                    reason = DISALLOWED[point]
                elif char.isascii():
                    continue
                elif char in FRENCH_LETTERS:
                    continue
                elif (0x2190 <= point <= 0x27BF
                      or 0x1F300 <= point <= 0x1FAFF):
                    reason = "arrow, geometric symbol or emoji: draw with JUCE"
                elif unicodedata.category(char) in {"Cf", "Cc"}:
                    reason = "hidden or non-printing Unicode character"
                else:
                    reason = "non-Latin glyph outside approved FR/EN labels"
                errors.append(
                    f"{filename}:{number}: {reason} U+{point:04X}"
                )

if errors:
    print("FAIL: Organix typography / Unicode preflight")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print(
    f"PASS: {len(UI_FILES)} Organix source files checked; "
    "UTF-8/NFC, FR accents, no fragile glyphs"
)
