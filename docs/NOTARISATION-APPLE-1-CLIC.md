# ORGANIX — SIGNATURE ET NOTARISATION APPLE EN UN CLIC

## Procédure issue de PEDAL LEGENDS

Même système de certificats Apple Developer ID Application / Installer et
mêmes noms de secrets GitHub Actions que PEDAL LEGENDS et ECHO 86.
La version du plugin est déterminée **automatiquement** dans `CMakeLists.txt`.
Aucun paramètre à saisir lors du lancement.

### Une seule fois : configurer les secrets sur le dépôt ORGANIX

Ouvrir : `https://github.com/BSM1973/Liberty-Organix/settings/secrets/actions`.

Les secrets Actions sont **propres à chaque dépôt**. GitHub ne copie pas
automatiquement les valeurs de PEDAL LEGENDS vers ORGANIX.

Reprendre les six valeurs existantes, sans envoyer aucun mot de passe ni
fichier `.p12` dans une conversation, un commit ou une issue :

- `APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64` — export base64 du certificat
  Developer ID Application **avec sa clé privée** ;
- `APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64` — export base64 du certificat
  Developer ID Installer **avec sa clé privée** ;
- `APPLE_P12_PASSWORD` — mot de passe des exports `.p12` ;
- `APPLE_NOTARY_APPLE_ID` — Apple ID utilisé pour la notarisation ;
- `APPLE_NOTARY_APP_PASSWORD` — mot de passe spécifique à l'app Apple ;
- `APPLE_NOTARY_TEAM_ID` — identifiant de l'équipe Apple Developer.

Si deux certificats utilisent des mots de passe d'export distincts, les
réexporter avec le même mot de passe (le workflow attend une valeur commune).

### À chaque nouvelle version : UN CLIC GitHub Actions

1. Ouvrir `https://github.com/BSM1973/Liberty-Organix/actions/workflows/commercial-release.yml`.
2. Cliquer sur **Run workflow** / **Exécuter le workflow**.
3. Sélectionner la branche **release/organix-notarization-one-click** pour une première notarisation sur la branche dédiée, ou **main** après fusion, puis **Run workflow**.
4. Si tous les contrôles sont verts, télécharger l'Artifact
   **Organix-macOS-vX.Y.Z-SIGNED-NOTARIZED-PKG**.
5. Livrer uniquement ce `.pkg` aux clients, jamais une archive CI unsigned
   ni `Organix-vX.Y.Z-QA-UNSIGNED.pkg`.

GitHub nécessite que le fichier de workflow soit présent sur la branche
par défaut pour afficher le bouton Run workflow. C'est pourquoi le workflow
est préparé sur la branche `release/organix-notarization-one-click`, puis
fusionné dans `main`. L'opération de signature ne s'exécute pas
automatiquement lors d'une PR ou d'un push.

### Ce que fait automatiquement le système

1. Vérifie les six secrets Apple et la validité de leurs enveloppes base64
   **avant** de dépenser du temps de compilation.
2. Construit AU, VST3 et Standalone macOS Apple Silicon ; exécute les tests JUCE.
3. Valide les binaires avec `pluginval` et `auval`.
4. Vérifie l'installation d'un paquet de QA non signé sur une machine GitHub.
5. Importe les deux Developer ID dans un trousseau temporaire.
6. Signe les AU/VST3 avec Developer ID Application + hardened runtime,
   puis signe le PKG avec Developer ID Installer.
7. Envoie le paquet à Apple via `xcrun notarytool submit --wait`.
8. N'accepte que la réponse **Accepted** ; attache le ticket avec
   `xcrun stapler` et vérifie Gatekeeper, l'installation et le reçu.
9. Publie le **seul installateur commercial** signé et notarisé.
10. Retélécharge cet exact Artifact et reverifie sa signature et son ticket.

### En cas de problème

- **Secrets manquants** : échec immédiat dans la première étape ; aucune
  construction ou publication commerciale.
- **Certificat invalide ou expiré** : erreur d'import / signature, Artifact
  commercial absent.
- **Apple refuse** : pas de package commercial, consulter la sortie
  `release-validation/apple-notary-log.json` dans les logs techniques.
- **Gatekeeper ou installation échoue** : pas de publication comme version
  validée.
- **Vert** : l'Artifact final est signé et notarisé ; toutefois aucune
  validation de fonctionnement réel dans Ableton, Logic et Bitwig n'est
  implicite : elle doit être effectuée séparément sur ordinateur.

## Rappel

Ne jamais ajouter les certificats ou secrets au dépôt GitHub. Il s'agit
d'une procédure de préparation et d'automatisation de la distribution ;
l'utilisateur décide lui-même quand déclencher la notarisation payante
en temps de build, et quand mettre le `.pkg` sur Shopify.
