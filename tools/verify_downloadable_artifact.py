#!/usr/bin/env python3
"""Hard release gate: no successful AU/VST3 build without a downloadable artifact.

LOCAL: validate the exact two ZIP files on disk before upload.
REMOTE: query this run's artifact by ID, download the resulting archive through
GitHub's authenticated API, and open/test both inner AU/VST3 ZIP bundles.
Any missing, empty, corrupted, expired, or mismatched artifact exits nonzero.
"""

import argparse
import io
import json
import os
from pathlib import Path
import sys
import time
import urllib.error
import urllib.request
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
        if magic not in (b"\\xcf\\xfa\\xed\\xfe", b"\\xca\\xfe\\xba\\xbe", b"\\xca\\xfe\\xba\\xbf"):
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


def request(url: str, token: str) -> bytes:
    req = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "X-GitHub-Api-Version": "2022-11-28",
            "User-Agent": "Organix-downloadable-artifact-gate",
        },
    )
    with urllib.request.urlopen(req, timeout=90) as response:
        return response.read()


def verify_published() -> None:
    repo = os.environ["GITHUB_REPOSITORY"]
    run = int(os.environ["GITHUB_RUN_ID"])
    artifact_id = int(os.environ["ARTIFACT_ID"])
    expected_name = os.environ["EXPECTED_ARTIFACT_NAME"]
    token = os.environ["GH_TOKEN"]
    if artifact_id <= 0 or not expected_name or not token:
        raise ValueError("Upload step did not provide an artifact ID/name/token")

    metadata_url = f"https://api.github.com/repos/{repo}/actions/artifacts/{artifact_id}"
    last_error: Exception | None = None
    # Newly published artifacts can take a short time to propagate to the API.
    for attempt in range(1, 13):
        try:
            metadata = json.loads(request(metadata_url, token))
            if metadata.get("name") != expected_name:
                raise ValueError(f"Wrong artifact name: {metadata.get('name')!r}")
            if metadata.get("expired") is not False:
                raise ValueError("Published artifact is expired or invalid")
            if int(metadata.get("size_in_bytes", 0)) < 100_000:
                raise ValueError("Published artifact has no meaningful content")
            workflow = metadata.get("workflow_run") or {}
            if int(workflow.get("id", -1)) != run:
                raise ValueError("Artifact belongs to a different workflow run")
            download_url = metadata.get("archive_download_url")
            if not download_url:
                raise ValueError("Artifact has no download URL")
            print(f"Found artifact #{artifact_id}: {expected_name}; downloading for integrity check")
            verify_outer(request(download_url, token))
            print(f"REMOTE GATE PASS: artifact #{artifact_id} from run #{run} is downloadable, and AU/VST3 are intact")
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as output:
                output.write(
                    "\\n## BUILD AU/VST3 VALIDÉ ET TÉLÉCHARGEABLE\\n"
                    f"**[TÉLÉCHARGER ORGANIX AU + VST3](https://github.com/{repo}/actions/runs/{run}/artifacts/{artifact_id})**\\n\\n"
                    f"Archive : \\u0060{expected_name}\\u0060. Vérification : téléchargement GitHub, deux ZIP intègres et deux binaires macOS présents.\\n"
                )
            return
        except (ValueError, OSError, urllib.error.URLError, zipfile.BadZipFile) as exc:
            last_error = exc
            print(f"Artifact verification attempt {attempt}/12: {exc}", file=sys.stderr)
            if attempt < 12:
                time.sleep(5)
    raise RuntimeError(f"BUILD FAILED: no verified downloadable AU/VST3 artifact: {last_error}")


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
