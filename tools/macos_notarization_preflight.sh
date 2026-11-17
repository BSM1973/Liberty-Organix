#!/usr/bin/env bash
# One-click commercial release: fail BEFORE recompilation if Apple settings
# are missing. Secrets exist ONLY in GitHub Actions; never copy .p12 into git.
set -euo pipefail

required=(
    APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64
    APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64
    APPLE_P12_PASSWORD
    APPLE_NOTARY_APPLE_ID
    APPLE_NOTARY_APP_PASSWORD
    APPLE_NOTARY_TEAM_ID
)
missing=0
for name in "${required[@]}"; do
    if [[ -z "${!name:-}" ]]; then
        echo "::error::Secret GitHub Actions manquant : ${name}"
        missing=1
    fi
done
if (( missing )); then
    echo "::error::Configurer les 6 secrets dans Settings > Secrets and variables > Actions, comme pour PEDAL LEGENDS."
    exit 1
fi

# Check the two encoded certificate files without logging certificate bytes.
for name in APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64 APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64; do
    if ! printf '%s' "${!name}" | /usr/bin/base64 -D >/dev/null 2>&1; then
        echo "::error::Le certificat ${name} n'est pas un fichier .p12 encodé en base64 valide."
        exit 1
    fi
done
if [[ "${APPLE_NOTARY_TEAM_ID}" =~ [[:space:]] ]]; then
    echo "::error::Le Team ID Apple contient des espaces."; exit 1
fi
echo "::notice::6 secrets présents et certificats base64 décodables : démarrage du build Organix."
if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    {
        echo "### Certificats et compte Apple"
        echo "Les 6 paramètres requis sont présents, les deux .p12 sont décodables."
        echo "Aucun identifiant secret ou certificat n'a été publié dans les logs."
    } >> "$GITHUB_STEP_SUMMARY"
fi
