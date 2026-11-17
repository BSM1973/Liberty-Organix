# ORGANIX v1.3.3 — Audition des 100 presets

## Périmètre de cette livraison

Les 100 noms, 10 catégories et 100 profils individuels v1.3 sont inchangés.
Cette version améliore le **protocole de validation**, sans appliquer de
normalisation de gain automatique ni prétendre à une écoute humaine.

Le nouveau programme `Tests/actual_processor_audio_audit.cpp` instancie le
véritable `OrganixAudioProcessor`, charge chaque preset, traite un signal
stéréo par `processBlock()` à 48 kHz / 256 échantillons et exporte :

- Une ligne de mesures pour chacun des 100 sons (entrée, sortie, gain
  relatif, crête, facteur de crête et corrélation stéréo).
- Pour chaque technologie réellement active, un rendu de contrôle avec
  cette technologie désactivée sur le **même** signal source. Deux mesures
  sont consignées : différence de niveau RMS et différence du signal
  échantillon par échantillon. La deuxième détecte une transformation
  même lorsque les niveaux sont identiques.
- Cent préécoutes stéréo WAV du moteur complet JUCE : deux secondes
  chacune, trois secondes pour les seize presets prioritaires.
- Le statut `PENDING_HUMAN_LISTENING` pour tous les presets.

Il s'agit de sources synthétiques déterministes, pas d'enregistrements
humains ni d'un vrai hôte AU/VST3. Les résultats permettent une sélection
rigoureuse pour l'écoute, sans fabriquer un verdict musical.

## 16 cas prioritaires identifiés dans l'audit v1.3

| N° | Preset | RMS de la préécoute simplifiée | Critère artistique à confirmer |
|---:|---|---:|---|
| 21 | Liquid Guitar Dream | -38,64 dBFS | Queue fluide, accords qui restent nets |
| 22 | Crystal Strings | -39,95 dBFS | Harmoniques utiles, pas de son métallique gênant |
| 24 | Ambient Guitar Clouds | -41,16 dBFS | Spatialisation sans perte de la mélodie |
| 26 | Luminous Guitar | -39,68 dBFS | Brillance sans agressivité |
| 27 | Infinite Guitar Pad | -42,50 dBFS | Longues textures, départ lisible, évolution progressive |
| 28 | Oceanic Guitar | -42,04 dBFS | Mouvement stéréo sans trou au centre |
| 29 | Cosmic Guitar | -36,87 dBFS | Spectre animé, attaques intelligibles |
| 30 | Frozen Harmonics | -40,71 dBFS | Sustains contrôlés, absence de résonances excessives |
| 31 | Liquid Drums | -36,70 dBFS | Groove et transitoires conservés |
| 32 | Crystal Percussion | -36,10 dBFS | Clarté rythmique et résonance musicale |
| 38 | Fluid Breaks | -37,74 dBFS | Précision du rythme malgré le morphing |
| 81 | Liquid Groove | -37,56 dBFS | Mouvement synchronisé au tempo |
| 82 | Crystal Sequence | -37,64 dBFS | Articulation des notes à plusieurs BPM |
| 84 | Vapor Motion | -43,18 dBFS | Densité régulière, sensation de matière sans disparition du son |
| 86 | Lightwave Sequencer | -39,26 dBFS | Pulsation audible, pas de dureté |
| 89 | Stereo Matter Dance | -41,84 dBFS | Mouvement stéréo maîtrisé et compatibilité mono |

Les niveaux ci-dessus proviennent de la **préécoute v1.3 MaterialDSP
simplifiée**, dont les sources ont un facteur de crête particulièrement
élevé pour la guitare, les percussions et les boucles rythmiques. Le
seuil de -35 dBFS est un simple indicateur de tri, pas une preuve que les
volumes d'usine doivent être remontés. Aucun +6 dB automatique.

## Grille d'écoute artistique à renseigner sur sources réelles

Pour **chaque** preset, comparer à niveau perçu équivalent au bypass sur
un fichier de son groupe. Pour les 16 cas ci-dessus, choisir au moins
deux sources de caractères différents, par exemple guitare clean ET
crunch ; batterie acoustique ET électronique.

Consigner les éléments suivants dans le DAW :

1. Source et projet, tempo, sample rate et buffer.
2. Bypass vs effet : crête, niveau perçu (LUFS sur une durée musicale
   pertinente), et compensation de gain d'écoute.
3. Caractère sonore (1–5), intelligibilité (1–5), originalité (1–5).
4. Clics, instabilité, grave, mono, feedback, mouvement stéréo et CPU.
5. MATTER DNA / Morph 4D / Trajectory / Reactions / Matrix :
   contrôle A/B sur le **son réellement entendu**, et non sur les seuls
   indicateurs d'activation.
6. Verdict : `VALIDÉ_À_L_OREILLE`, `À_AJUSTer` ou `À_RETESTER`,
   accompagné d'une modification précise et justifiée.

### Conditions de publication
Ne déclarer aucun preset « équilibré à l'oreille » avant une écoute
humaine réellement effectuée. Les WAV du banc JUCE peuvent servir à
préparer la séance mais ne reproduisent pas toutes les particularités de
l'hôte ni le ressenti d'une production musicale. Ne pas remplacer un
jugement de sound design par une égalisation aveugle à -18 dBFS.


## Résultats de la première passe sur le moteur JUCE complet

Le premier Artifact de la PR #17 a rendu 100 sorties stéréo par
`OrganixAudioProcessor::processBlock`. Aucun programme ne fut silencieux.
Les 16 cas prioritaires ont montré un gain relatif à la source allant
d'environ -1,6 dB à -9,3 dB : **écart d'amplitude, pas de qualité sonore**.

L'audit comparait initialement Morph 4D uniquement à la scène A. Or
celle-ci peut être volontairement identique au son de base. Le protocole
v1.3.3 teste désormais une position XY non nulle, avec Trajectory
désactivé dans les deux bras, avant d'évaluer la différence de Morphing.

Deux affectations factory de Mod Matrix n'avaient pas d'effet dans
leur contexte : `Metallic Strings` visait la densité du moteur Metal,
et `Metallic Oscillations` visait son timbre. Les routes principales
ont été remplacées, respectivement par Transitoires -> Énergie A et
LFO 1 -> Énergie A, sans toucher aux autres noms et catégories.
Ces corrections doivent être vérifiées par le nouvel audit et à
l'écoute réelle avant un statut de sound design final.
