# ORGANIX v1.1 — SPECTRAL VIEW

## Identité et conservation de l'interface

Le champ sonore central ne devient pas une image statique ni un autre plugin : ses **six rendus natifs JUCE** (LIQUID, CRYSTAL, PLASMA, VAPOR, METAL, LIGHT) sont conservés. L'animation et les déformations intègrent les mesures de la sortie réelle, plutôt que le seul niveau RMS : transitoires, brillance, centre spectral et flux spectral. Les commandes XY de forme et les couleurs de chaque moteur restent en place.

## Analyse FFT réelle AVANT/APRÈS

- Deux analyseurs indépendants, `Source/OrganixSpectralView.h/.cpp`, alimentés directement dans `PluginProcessor::processBlock`.
- **AVANT** : vraie entrée de l'hôte, enregistrée avant toute modification en place du buffer.
- **APRÈS** : vrai buffer de sortie final après les traitements JUCE, la matrice, les 4 scènes, le wet/dry, la mise en forme des transitoires et le bypass.
- Fenêtre Hann 1024 échantillons, hop 512, FFT radix-2 pré-calculée, 48 bandes logarithmiques, magnitude en échelle dB (fenêtre de visualisation -78..0 dBFS), lissage rapide à l'attaque et lent au relâchement.
- Deux tracés superposables, palette neutre pour l'entrée et couleur propre au matériau affiché pour la sortie.
- Bouton natif cyclique **AVANT → APRÈS → AVANT / APRÈS** (INPUT → OUTPUT → BEFORE / AFTER).
- Indicateurs séparés : énergie, transitoires, centroïde, brillance HF, flux spectral ; le champ choisit des pondérations différentes selon les six moteurs.
- L'analyse FFT est **indépendante de l'activation MATTER DNA**, donc une session sans ADN bénéficie également d'une visualisation spectrale.
- Somme stéréo moyenne (Mid) pour la forme FFT, RMS calculée comme énergie moyenne des deux canaux. Une source totalement anti-phase peut donc apparaître plus faible dans la FFT que dans le RMS.
- Analyse FFT à la cadence d'acquisition (environ 94 transformations/s/stream à 48 kHz, indépendamment de la taille du bloc). Aucune allocation, verrou, accès disque, ou appel UI sur le thread audio ; transmission des 48 bandes via des variables atomiques lisibles à 30 FPS.

## Vrai contrôle des transitoires

- Paramètre automatisable `spectralTransient` dans `[-1,1]`, neutre par défaut à zéro pour préserver le son des anciens presets.
- `0..+100%` renforce les attaques, `-100..0%` les adoucit, avec détection d'enveloppe rapide ~1,8 ms et lente ~45 ms et profondeur lissée ~25 ms.
- Le shaper travaille sur le vrai signal de la voie d'effet avant l'étage de bypass hôte. Il ne modifie donc pas la voie directe quand Organix est bypassé. Niveau limité à ±1 pour la sécurité.
- Le DSP est isolé dans `Source/OrganixSpectralTransient.h` et testé à profondeur neutre, positive et négative.

## Charge DSP

L'interface affiche **DSP / BUDGET** en pourcentage : rapport du temps mesuré par `std::chrono::steady_clock` pour `processBlock` sur la durée du bloc audio (`échantillons / sampleRate`). C'est **la part de la fenêtre temps réel consommée par Organix**, et non la charge CPU globale du Mac ni celle du DAW. Des mesures au-delà de 100 % signalent un dépassement potentiel ; l'indicateur est filtré pour être lisible.

Les trois anciens compteurs ENTRÉE RMS, SORTIE RMS et CRÊTE SORTIE restent disponibles, désormais dans une rangée à quatre colonnes avec l'indicateur DSP.

## Vérifications

- Onzième test C++ : différence fréquentielle mesurable entre sinus à 220 Hz et 5 kHz, ségrégation entrée/sortie, silence, transitoires exclusivement sur la sortie, résistance aux NaN, stéréo à 192 kHz, protection du son en position neutre et modification réelle des transitoires ±100 %.
- Régression des 756 tailles de fenêtres du champ central : sélecteur de comparaison, curseur transitoires, tracés FFT et 4 compteurs sans chevauchement.
- Tests de compatibilité Material Reactions, Mod Matrix, Morph 4D, DNA et Trajectory.
- Compilation JUCE AU/VST3 pour macOS ARM64 **avec Artifact obligatoire**. La CI n'est verte que si elle publie, retélécharge et ouvre effectivement les deux ZIP AU/VST3.
- L'écoute sur un véritable système et la comparaison CPU DAW restent nécessaires : le build vert prouve la compilation et la conformité logicielle, pas une validation subjective de tous les styles sonores.

Version source : 1.1.0 ; pas de changement des 100 presets d'usine ni des six moteurs DSP en dehors de la nouvelle commande de transitoires, désactivée par défaut.
