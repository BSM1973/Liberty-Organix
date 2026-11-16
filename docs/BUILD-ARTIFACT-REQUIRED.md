# RÈGLE DE LIVRAISON — BUILD VERT = ARTIFACT AU + VST3 TÉLÉCHARGEABLE

**Règle obligatoire pour les builds Organix macOS.** Un indicateur vert n'est
valable pour un plugin testable que si **les deux formats Audio Unit et VST3
ont été compilés, archivés, publiés ET réellement retéléchargés depuis
GitHub Actions**. Le seul succès de CMake/CTest ne suffit pas.

## Deux workflows distincts

- **Organix BUILD AU/VST3 — ARTIFACT OBLIGATOIRE** :
  build macOS ARM64, tests, contrôle Mach-O, création des deux ZIP, upload
  dans un seul Artifact GitHub et double validation de l'archive publiée.
  Le build **DOIT ÊTRE ROUGE** si l'un de ces points échoue.
- **Organix TESTS UNIQUEMENT — SANS AU/VST3** :
  régressions C++ rapides sur Linux. Ce workflow ne produit ni AU ni VST3.
  Son succès ne signifie **JAMAIS** qu'un plugin a été livré.

## Vérifications bloquantes

1. Les exécutables `Organix.component/Contents/MacOS/Organix` et
   `Organix.vst3/Contents/MacOS/Organix` existent et sont arm64 (lipo).
2. Les ZIP AU et VST3 existent et peuvent être ouverts ; leurs exécutables
   internes sont reconnus comme Mach-O.
3. `actions/upload-artifact@v4` crée l'Artifact obligatoire
   `Organix-v<version-CMake>-macOS-arm64-AU-VST3` avec
   `if-no-files-found: error` et conservation demandée 90 jours.
4. Le workflow interroge l'API GitHub avec l'ID effectif de l'Artifact,
   vérifie qu'il appartient **à ce run**, qu'il est non expiré et non vide,
   **télécharge réellement** le ZIP depuis l'API et en contrôle les deux
   archives intérieures, jusqu'aux binaires Mach-O.
5. Le lien direct d'Artifact est inscrit au résumé du build **uniquement
   après** cette validation. L'étape est située en fin de job : si elle échoue,
   le job et le workflow deviennent rouges.

La vérification est codée dans `tools/verify_downloadable_artifact.py`,
appelé avant **et après** la publication.

## Discipline avant d'annoncer un build comme disponible

Ne jamais qualifier de « prêt à tester » une validation C++ verte.
Toujours relever **le run de compilation macOS**, **l'Artifact ID** et
le **lien direct** ; vérifier que l'Artifact n'est pas expiré. En cas
d'Artifact manquant, annoncer un build non livrable, même si CTest passe.

**Important :** GitHub peut annuler des jobs, les files d'attente peuvent
retarder la création des fichiers, et les Artifacts GitHub expirent selon
la politique de rétention du dépôt (90 jours demandés ici, ou une limite
administrative plus faible). La présence après expiration ne peut pas
être garantie par un workflow. La protection de branche GitHub, gérée
séparément, est nécessaire pour empêcher la fusion de modifications qui
échouent ce workflow. La validation d'écoute en DAW et la notarisation
commerciale restent des étapes distinctes.
