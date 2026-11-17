#!/usr/bin/env bash
set -euo pipefail

AU="build/Organix_artefacts/Release/AU/Organix.component"
VST3="build/Organix_artefacts/Release/VST3/Organix.vst3"
test -f "$AU/Contents/MacOS/Organix"
test -f "$VST3/Contents/MacOS/Organix"
mkdir -p validation-logs pluginval-download

# Pin a known pluginval release, rather than executing an unreviewed latest URL.
curl --fail --location --retry 3 --silent --show-error \
  https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_macOS.zip \
  -o pluginval-download/pluginval_macOS.zip
ditto -x -k pluginval-download/pluginval_macOS.zip pluginval-download

PLUGINVAL="$(find pluginval-download -path '*/Contents/MacOS/pluginval' -type f -print -quit)"
test -n "$PLUGINVAL"
chmod +x "$PLUGINVAL"
file "$PLUGINVAL" | tee validation-logs/pluginval-binary.txt
file "$VST3/Contents/MacOS/Organix" "$AU/Contents/MacOS/Organix" | tee validation-logs/organix-binaries.txt

# Actual exported VST3, running in a third-party plug-in validator.
set -o pipefail
"$PLUGINVAL" --validate-in-process --strictness-level 5 --validate "$VST3" \
  2>&1 | tee validation-logs/pluginval-vst3.txt

# Register the AU only in this disposable CI account; do not install anything
# on the user's Mac. 'auval' loads the real AU component.
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
ditto "$AU" "$HOME/Library/Audio/Plug-Ins/Components/Organix.component"
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
sleep 4
auval -a 2>&1 | tee validation-logs/auval-list.txt
auval -v aumf Orgx LbMs 2>&1 | tee validation-logs/auval-organix.txt
