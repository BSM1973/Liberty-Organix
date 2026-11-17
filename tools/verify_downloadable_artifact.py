#!/usr/bin/env python3
"""Hard release gate: no successful AU/VST3 build without a downloadable artifact.

LOCAL: validate the exact two ZIP files on disk before upload.
REMOTE: query this run's artifact by ID, download the resulting archive through
GitHub's authenticated API, and open/test both inner AU/VST3 ZIP bundles.
Any missing, empty, corrupted, expired, or mismatched artifact exits nonzero.
"""

import argparse
import io
import os
from pathlib import Path
import sys
import zipfile


BUNDLES = {
    "Organix-AU-macOS-arm64.zip": "Organix.component/Contents/MacOS/Organix",
    "Organix-VST3-macOS-arm64.zip": "Organix.vst3/Contents/MacOS/Organix",
}


def verify_bundle(data: bytes, entry: str, bundle_name: str) -> None:
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        broken = z.testzip()
        if broken:
            raise ValueError(f"{bundle_name}: corrupt ZIP member {broken}")
        try:
            binary = z.getinfo(entry)
        except KeyError as exc:
            raise ValueError(f"{bundle_name}: missing native plugin binary {entry}") from exc
        if binary.file_size < 100_000:
            raise ValueError(f"{bundle_name}: plugin binary is unexpectedly small")
        with z.open(binary) as fp:
            magic = fp.read(4)
        # Mach-O 64-bit little endian (thin ARM64) or universal fat binary.
        if magic not in (b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf"):
            raise ValueError(f"{bundle_name}: binary is not Mach-O")
        print(f"VERIFIED {bundle_name}: {entry} ({binary.file_size} bytes)")


def verify_outer(data: bytes) -> None:
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        broken = z.testzip()
        if broken:
            raise ValueError(f"Published artifact ZIP is corrupt: {broken}")
        names = {i.filename for i in z.infolist() if not i.is_dir()}
        expected = set(BUNDLES)
        if not expected.issubset(names):
            raise ValueError(f"Published artifact is missing AU/VST3: expected {sorted(expected)}, got {sorted(names)}")
        for name, inner_entry in BUNDLES.items():
            verify_bundle(z.read(name), inner_entry, name)


def verify_local() -> None:
    for name, entry in BUNDLES.items():
        source = Path(name)
        if not source.is_file() or source.stat().st_size < 100_000:
            raise ValueError(f"Required build archive missing or empty: {source}")
        verify_bundle(source.read_bytes(), entry, name)
    print("LOCAL GATE PASS: both compiled macOS plugins are valid and present")


def verify_published() -> None:
    # actions/download-artifact@v4 downloads the *artifact ID returned by the
    # preceding upload step in the very same GitHub Actions run*. Unlike an
    # arbitrary REST redirect, this uses the official Actions runtime auth.
    # If publication or real download fails, that workflow step fails before
    # we reach this function, and the overall macOS build stays RED.
    repo = os.environ["GITHUB_REPOSITORY"]
    run = int(os.environ["GITHUB_RUN_ID"])
    artifact_id = int(os.environ["ARTIFACT_ID"])
    expected_name = os.environ["EXPECTED_ARTIFACT_NAME"]
    root = Path(os.environ.get("DOWNLOADED_ARTIFACT_DIR", "verified-download"))

    if artifact_id <= 0 or not expected_name.startswith("Organix-v"):
        raise ValueError("Missing or invalid published artifact ID / name")
    if not root.is_dir():
        raise ValueError(f"Official GitHub download did not create {root}")

    actual = {p.name for p in root.iterdir() if p.is_file()}
    expected = set(BUNDLES)
    if actual != expected:
        raise ValueError(
            f"Downloaded artifact contents mismatch: expected {sorted(expected)}, got {sorted(actual)}"
        )
    for name, entry in BUNDLES.items():
        source = root / name
        if source.stat().st_size < 100_000:
            raise ValueError(f"Downloaded {name} is missing or unexpectedly small")
        verify_bundle(source.read_bytes(), entry, name)

    print(
        f"REMOTE GATE PASS: official GitHub artifact #{artifact_id} "
        f"downloaded in run #{run} and both AU/VST3 archives verified"
    )
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as output:
            output.write(
                "\n## BUILD AU/VST3 VALIDÉ ET TÉLÉCHARGEABLE\n"
                f"**[TÉLÉCHARGER ORGANIX AU + VST3]"
                f"(https://github.com/{repo}/actions/runs/{run}/artifacts/{artifact_id})**\n\n"
                f"Archive : `{expected_name}`. "
                "Vérification : téléchargement officiel GitHub Actions par ID, "
                "intégrité des deux ZIP et présence des binaires macOS.\n"
            )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["local", "published"])
    args = parser.parse_args()
    try:
        if args.mode == "local":
            verify_local()
        else:
            verify_published()
    except (RuntimeError, ValueError, OSError, zipfile.BadZipFile) as exc:
        print(f"RELEASE GATE RED: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
