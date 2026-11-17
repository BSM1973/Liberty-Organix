#!/usr/bin/env bash
# Only run on protected, manually triggered main. No publication until Apple Accepted.
set -euo pipefail
for key in APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64 \
           APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64 APPLE_P12_PASSWORD \
           APPLE_NOTARY_APPLE_ID APPLE_NOTARY_APP_PASSWORD APPLE_NOTARY_TEAM_ID; do
  [[ -n "${!key:-}" ]] || { echo "::error::Missing secret $key. Release not published."; exit 1; }
done
version="${1:?Usage: macos_sign_notarize.sh SEMVER}"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || exit 1
keychain="$RUNNER_TEMP/organix-release.keychain-db"
pass="$(/usr/bin/uuidgen)"
appfile="$RUNNER_TEMP/organix-app.p12"
installerfile="$RUNNER_TEMP/organix-installer.p12"
cleanup() {
  /bin/rm -f "$appfile" "$installerfile"
  /usr/bin/security delete-keychain "$keychain" >/dev/null 2>&1 || true
}
trap cleanup EXIT
/usr/bin/security create-keychain -p "$pass" "$keychain"
/usr/bin/security set-keychain-settings -lut 21600 "$keychain"
/usr/bin/security unlock-keychain -p "$pass" "$keychain"
/usr/bin/security list-keychains -d user -s "$keychain"
/usr/bin/security default-keychain -s "$keychain"
printf %s "$APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64" | /usr/bin/base64 -D > "$appfile"
printf %s "$APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64" | /usr/bin/base64 -D > "$installerfile"
/usr/bin/security import "$appfile" -k "$keychain" -P "$APPLE_P12_PASSWORD" -T /usr/bin/codesign
/usr/bin/security import "$installerfile" -k "$keychain" -P "$APPLE_P12_PASSWORD" -T /usr/bin/pkgbuild
/usr/bin/security set-key-partition-list -S apple-tool:,apple: -s -k "$pass" "$keychain"
app_identity="$(/usr/bin/security find-identity -v -p codesigning "$keychain" | /usr/bin/awk -F '"' '/Developer ID Application:/{print $2;exit}')"
pkg_identity="$(/usr/bin/security find-identity -v "$keychain" | /usr/bin/awk -F '"' '/Developer ID Installer:/{print $2;exit}')"
[[ -n "$app_identity" && -n "$pkg_identity" ]] || {
  echo "::error::Developer ID Application and Installer identities required"; exit 1;
}
[[ "$app_identity" == *"($APPLE_NOTARY_TEAM_ID)"* &&
   "$pkg_identity" == *"($APPLE_NOTARY_TEAM_ID)"* ]] || {
  echo "::error::Developer ID certificates do not match Apple Team ID"; exit 1;
}
stage="$RUNNER_TEMP/organix-commercial-root"
bash tools/macos_stage_plugins.sh "$stage"
for spec in "Components/Organix.component" "VST3/Organix.vst3"; do
  bundle="$stage/Library/Audio/Plug-Ins/$spec"
  /usr/bin/codesign --force --options runtime --timestamp \
      --sign "$app_identity" "$bundle"
  /usr/bin/codesign --verify --strict --deep --verbose=2 "$bundle"
  signature="$(/usr/bin/codesign --display --verbose=4 "$bundle" 2>&1)"
  /usr/bin/grep -Fq 'Authority=Developer ID Application:' <<< "$signature"
  /usr/bin/grep -q 'runtime' <<< "$signature"
done
mkdir -p release-output release-validation
pkg="release-output/Organix-v${version}-macOS-SIGNED-NOTARIZED.pkg"
/usr/bin/pkgbuild --root "$stage" \
  --identifier "com.libertymusicsoftware.organix.installer" \
  --version "$version" --install-location / --ownership recommended \
  --sign "$pkg_identity" "$pkg"
/usr/sbin/pkgutil --check-signature "$pkg" | /usr/bin/tee release-validation/installer-signature.txt
/usr/sbin/pkgutil --payload-files "$pkg" > release-validation/payload.txt
/usr/bin/grep -Fq 'Library/Audio/Plug-Ins/Components/Organix.component/Contents/MacOS/Organix' release-validation/payload.txt
/usr/bin/grep -Fq 'Library/Audio/Plug-Ins/VST3/Organix.vst3/Contents/MacOS/Organix' release-validation/payload.txt
response="$RUNNER_TEMP/organix-notary-result.json"
/usr/bin/xcrun notarytool submit "$pkg" \
  --apple-id "$APPLE_NOTARY_APPLE_ID" \
  --password "$APPLE_NOTARY_APP_PASSWORD" \
  --team-id "$APPLE_NOTARY_TEAM_ID" \
  --wait --output-format json > "$response"
status="$(/usr/bin/python3 -c 'import json,sys;print(json.load(open(sys.argv[1])).get("status","Unknown"))' "$response")"
echo "Apple notarization result: $status"
if [[ "$status" != "Accepted" ]]; then
  echo "::error::Notary status not Accepted; no commercial installer"
  exit 1
fi
/usr/bin/xcrun stapler staple "$pkg"
/usr/bin/xcrun stapler validate "$pkg" | /usr/bin/tee release-validation/stapler.txt
/usr/sbin/spctl --assess --type install --verbose=2 "$pkg" 2>&1 \
  | /usr/bin/tee release-validation/gatekeeper.txt
# Clean CI runner install: no pre/post-install hooks or old plugin migrations.
sudo /usr/sbin/installer -pkg "$pkg" -target / | /usr/bin/tee release-validation/install.txt
/usr/sbin/pkgutil --pkg-info "com.libertymusicsoftware.organix.installer" \
  | /usr/bin/tee release-validation/receipt.txt
for spec in "Components/Organix.component" "VST3/Organix.vst3"; do
  bundle="/Library/Audio/Plug-Ins/$spec"
  test -f "$bundle/Contents/MacOS/Organix"
  /usr/bin/codesign --verify --strict --deep "$bundle"
  signature="$(/usr/bin/codesign --display --verbose=4 "$bundle" 2>&1)"
  /usr/bin/grep -Fq 'Authority=Developer ID Application:' <<< "$signature"
done
test -s "$pkg"
echo "PASS: Developer ID signed, Apple Accepted, stapled, Gatekeeper verified, AU/VST3 installed"
