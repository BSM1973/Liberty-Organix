#!/usr/bin/env bash
# Untrusted PR/CI path: verify package layout and clean installation only.
# This unsigned test package MUST NOT be published to customers.
set -euo pipefail
version="${1:?Usage: macos_pkg_qa.sh SEMVER}"
root="${RUNNER_TEMP:-/tmp}/organix-qa-payload"
mkdir -p "$root" "installer-qa"
bash tools/macos_stage_plugins.sh "$root"
pkg="installer-qa/Organix-v${version}-QA-UNSIGNED.pkg"
/usr/bin/pkgbuild --root "$root" \
  --identifier "com.libertymusicsoftware.organix.installer" \
  --version "$version" --install-location / \
  --ownership recommended "$pkg"
/usr/sbin/pkgutil --payload-files "$pkg" > installer-qa/payload.txt
/usr/bin/grep -Fq "Library/Audio/Plug-Ins/Components/Organix.component/Contents/MacOS/Organix" installer-qa/payload.txt
/usr/bin/grep -Fq "Library/Audio/Plug-Ins/VST3/Organix.vst3/Contents/MacOS/Organix" installer-qa/payload.txt
sudo /usr/sbin/installer -pkg "$pkg" -target / > installer-qa/install.txt
/usr/sbin/pkgutil --pkg-info "com.libertymusicsoftware.organix.installer" > installer-qa/receipt.txt
for spec in "Components/Organix.component" "VST3/Organix.vst3"; do
  installed="/Library/Audio/Plug-Ins/$spec"
  test -f "$installed/Contents/MacOS/Organix"
  /usr/bin/lipo -archs "$installed/Contents/MacOS/Organix" | /usr/bin/grep -qw arm64
done
echo "PASS: temporary runner installed AU and VST3 in standard directories"
echo "QA ONLY: unsigned package, not for customer distribution"
