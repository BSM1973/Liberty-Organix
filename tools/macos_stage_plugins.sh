#!/usr/bin/env bash
# Stage only Organix AU/VST3 into Apple's canonical system locations.
set -euo pipefail
stage="${1:?Usage: macos_stage_plugins.sh STAGING_ROOT}"
au="build/Organix_artefacts/Release/AU/Organix.component"
vst3="build/Organix_artefacts/Release/VST3/Organix.vst3"
test -f "$au/Contents/MacOS/Organix" || { echo "::error::Missing Organix AU"; exit 1; }
test -f "$vst3/Contents/MacOS/Organix" || { echo "::error::Missing Organix VST3"; exit 1; }
[[ "$stage" = /* ]] || { echo "::error::Use an absolute staging path"; exit 1; }
mkdir -p "$stage/Library/Audio/Plug-Ins/Components" \
         "$stage/Library/Audio/Plug-Ins/VST3"
dst_au="$stage/Library/Audio/Plug-Ins/Components/Organix.component"
dst_vst3="$stage/Library/Audio/Plug-Ins/VST3/Organix.vst3"
/usr/bin/ditto "$au" "$dst_au"
/usr/bin/ditto "$vst3" "$dst_vst3"
for bundle in "$dst_au" "$dst_vst3"; do
    test -f "$bundle/Contents/MacOS/Organix"
    /usr/bin/lipo -archs "$bundle/Contents/MacOS/Organix" | /usr/bin/grep -qw arm64
done
echo "PASS: AU + VST3 staged under /Library/Audio/Plug-Ins"
