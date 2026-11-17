# ORGANIX v1.2 — PRESET LAB

## Objectif

La priorité est la qualité et la reproductibilité des 100 sons d'usine existants, pas l'ajout artificiel de nouveaux noms. La bibliothèque conserve ses 100 références et les formats utilisateurs `.organix`.

## PRESET LAB natif, FR par défaut

Le navigateur existant reçoit une entrée PRESET LAB qui ouvre un espace de travail plein format dans la bibliothèque. Aucun élément n'est superposé à l'éditeur audio principal. Dans le LAB :

- **Comparaison A/B** : conserver l'état complet de deux sons dans la mémoire courante, passer de A à B et conserver les modifications de chacun. Les scènes morph, la trajectoire, les réglages de la matrice et les paramètres host sont inclus via les états XML JUCE.
- **UNDO / REDO** : jusqu'à 48 instantanés d'états complets au niveau message thread. Les changements manuels sont regroupés après une période de stabilisation, et les variations/rappels produisent une étape de retour. Les boutons restaurent les paramètres d'origine par `setStateInformation`.
- **Randomisation musicale** : 44 paramètres réels choisis et pondérés. La profondeur, bornée à 5–100 %, limite l'ampleur des changements. Le résultat est reproductible avec une graine déterministe. Les valeurs aléatoires préservent le choix des deux moteurs, les modes de routage, le bypass, le gain d'entrée et celui de sortie.
- **Cinq verrous indépendants** : couche A, couche B, espace, mouvement, macros. Lorsque verrouillés, aucun des paramètres de leur groupe ne peut être modifié.
- **Quatre mémoires de variations** : boutons distincts CAPTURER / ÉCOUTER pour quatre états complets, à comparer à volonté avant de les enregistrer en presets personnels.
- **Recherche par caractère sonore** : 11 familles descriptives en FR/EN (atmosphérique, chaud, brillant, sombre, rythmique, organique, métallique, agressif, doux, spacieux, expérimental). Les recherches se combinent au nom, catégorie, moteur et favoris.
- Les caractères sont actuellement indexés à partir des descriptions, noms, catégories et moteurs de 100 presets ; **ce ne sont pas des classifications audio mesurées**. Les presets personnels sans métadonnées ne sont pas assignés automatiquement à des caractères inventés.

A/B et historique sont des **instantanés de la session courante du LAB**, non des banques persistantes à travers une fermeture du plugin. Pour conserver définitivement une variation, l'enregistrer au format `.organix`. Aucun traitement ne démarre sur le thread audio à partir de ces commandes d'interface.

## Qualité audio des 100 sons d'usine

Ce dépôt contenait déjà 100 noms et 10 catégories, mais la méthode `loadFactoryPreset` utilisait des formules par catégorie/index, sans écoute instrumentale documentée. Cette version ajoute :

1. Un audit automatique qui rend **les 100 combinaisons DSP réelles** de matériaux/paramètres de fond avec des entrées musicales **synthétiques contrôlées** (sons harmoniques, attaques de guitare/percussions, basses et impulsions rythmiques selon la famille).
2. Un rapport CSV par preset avec RMS de sortie, peak, crest factor, différence RMS avec l'entrée, suggestion de compensation limitée à ±6 dB et signalements de faible niveau ou de crête élevée.
3. **100 extraits WAV**, trois secondes chacun en PCM mono 16 bits/48 kHz, réunis dans un second Artifact GitHub `Organix-100-Factory-Audition-and-QA-v1.2.0`.
4. Un test bloquant sur NaN/Inf et silence de sortie, exécuté dans les tests C++/macOS. L'Artifact de préécoute est publié indépendamment de l'Artifact obligatoire AU/VST3.

**Limite essentielle :** ces rendus sont des préécoutes DSP avec des entrées synthétiques, elles ne remplacent pas l'écoute de vrais instruments ou mixages dans un DAW, et le rendu complet du plugin peut différer (macros, dynamique, trajectoires, sommation/mixage et réglages de l'hôte). Aucun modèle n'a « écouté et validé » subjectivement les 100 presets ; leur équilibrage artistique doit être décidé à partir d'une session d'écoute A/B, de fichiers sources identiques et de mesures de niveau. Les suggestions de gain CSV ne sont pas appliquées automatiquement, car une normalisation sur synthèse de test pourrait détériorer le niveau d'un vrai instrument.

## Validation et livraison

- Tests C++ : indexation des 100 presets, 11 filtres descriptifs, 44 réglages/mutations bornés, reproductibilité, verrouillage strict de chaque groupe.
- Test dédié : rendu de tous les 100 moteurs d'usine avec métriques audio, contrôle de silence et de non-finitude, et export de 100 WAV pour audition humaine.
- Tests DSP/Morph/DNA/Spectral/Matrix/Reactions/Trajectory précédents conservés.
- Build macOS ARM64 AU/VST3, binaires Mach-O vérifiés, Artifact GitHub publié **et retéléchargé puis ouvert** ; sans ces trois gates, aucun build ne sera annoncé comme « vert et téléchargeable ».
- Validation subjective à faire : écoute des 100 presets sur une batterie de sources (voix, guitare, batterie, synthé, basse, bruit/impulsions) avec niveaux appariés, jugement des transitoires et de la stéréo, élimination des quasi-doublons, puis réglage manuel final des 100 programmes.

## Historique

Début de v1.2 : 100 presets conservés, aucun nouveau preset ajouté.
