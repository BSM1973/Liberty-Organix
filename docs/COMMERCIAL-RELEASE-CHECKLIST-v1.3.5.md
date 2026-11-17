# ORGANIX v1.3.5 — Mise sur le marché macOS

## Livraison technique
- L'Artifact de CI **AU+VST3 ARM64** n'est pas automatiquement une version
  commerciale signée et notarisée.
- Le workflow manuel `ORGANIX COMMERCIAL RELEASE — APPLE SIGNED + NOTARIZED`
  ne peut être déclenché depuis GitHub que sur `main`. Aucune PR ne
  reçoit les certificats ni les mots de passe.
- Les secrets GitHub Actions requis dans **BSM1973/Liberty-Organix** :
  `APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64`,
  `APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64`,
  `APPLE_P12_PASSWORD`, `APPLE_NOTARY_APPLE_ID`,
  `APPLE_NOTARY_APP_PASSWORD`, `APPLE_NOTARY_TEAM_ID`.
- La présence et la validité de ces secrets ne sont pas connues par le
  code source. Aucun dépôt ne copie les certificats des autres plugins.

## Installer propre
- Les binaires ARM64 vont uniquement vers :
  `/Library/Audio/Plug-Ins/Components/Organix.component`
  et `/Library/Audio/Plug-Ins/VST3/Organix.vst3`.
- Aucun script post-install, réinitialisation de préférence, suppression
  d'autre plugin ou modification de fichiers de session.
- `pkgbuild` signe avec Developer ID Installer ; les deux composants
  sont signés Developer ID Application avec hardened runtime.
- `notarytool` doit retourner **Accepted**, `stapler validate`,
  `spctl --type install` et `pkgutil --check-signature` doivent réussir.
- Une installation sur runner macOS propre vérifie les 2 composants et le
  reçu d'installation avant publication de l'Artifact commercial.
- Le workflow télécharge l'Artifact publié et revalide ce téléchargement.
- Toute erreur, certificat manquant ou rejet Apple bloque la livraison.

## Matrice de compatibilité hôtes : à qualifier en vraie session
| Hôte | Format | Contrôles requis | État |
|---|---|---|---|
| Ableton Live | VST3 / AU sur macOS | CC MIDI, réouverture, presets, 32/64/128/256/512, automation, CPU | Non testé dans l'hôte |
| Logic Pro | AU | Validation AU, CC MIDI, automation, sauvegarde/rechargement | Non testé dans l'hôte |
| Bitwig Studio | VST3 | Routage MIDI FX, CC MIDI, modulations, rappel de projet | Non testé dans l'hôte |
| Cubase / Nuendo | VST3 | Gestion MIDI, rendu hors ligne, automation, 44,1–96 kHz | Non testé dans l'hôte |
| Studio One | VST3 / AU | Chargement, presets, DAW recall, MIDI learn, rendu | Non testé dans l'hôte |
| Reaper | VST3 / AU | Automations, Recall, MIDI CC, petits buffers | Non testé dans l'hôte |

**La compilation, pluginval et auval ne prouvent pas le fonctionnement
dans les versions propriétaires de ces DAW.** Il faut des sessions
d'essai sur la machine finale et l'interface audio réelle, puis enregistrer
leurs résultats avant de communiquer une liste de compatibilité certifiée.

## Contrôle produit
- 100 presets exacts ; 6 moteurs ; UI française et anglaise ; écran 13 pouces.
- Tester l'ouverture complète, sans ascenseur global ni boutons masqués.
- Ne pas annoncer la signature ou la notarisation tant qu'Apple n'a pas
  rendu `Accepted` pour **cet Artifact Organix exact**.
