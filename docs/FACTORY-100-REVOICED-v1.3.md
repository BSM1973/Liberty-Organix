# ORGANIX v1.3 — 100 FACTORY PRESETS REVOICED

## Règle absolue
Conserver **exactement les 100 noms et leurs catégories**, sans aucune augmentation du nombre de presets. Les 100 anciens points de départ construits par la formule `category + index % 10` sont remplacés par **100 profils indépendants** spécifiés explicitement dans `Source/FactorySoundDesigns.h`.

Les profils sont rédigés un par un avec des intensités/mouvements/timbres/feedbacks propres pour A et B, un Dry/Wet, un gain, une largeur stéréo, l'énergie, la densité et les profondeurs des nouvelles technologies. Les presets de basse privilégient la protection du grave et une largeur proche du mono ; les presets de mixage restent subtils et peu traités.

## Utilisation effective des technologies

- **MATTER DNA** : analyse sonore FFT active ; intensité et sensibilité choisies dans chaque profil, avec paramètres d'analyse d'enveloppes adaptés au matériau.
- **MORPHING 4D** : lorsqu'un preset l'utilise, les quatre scènes A/B/C/D sont réellement différentes, et non des copies identiques. Les transitions pilotent les mêmes moteurs physiques avec timbre, mouvement, équilibre et parfois matériaux alternatifs différents.
- **TRAJECTORY DESIGNER** : activé uniquement si des scènes 4D contrastées ont été créées. Cercle, spirale ou zigzag selon la conception individuelle, avec vitesse et synchronisation musicale adaptées aux programmes rythmiques.
- **MATERIAL REACTIONS** : lien audio A→B ou mutuel sur les presets qui en tirent un caractère audible. La deuxième couche est activée au rappel même lorsque le preset v0.1 n'avait qu'un moteur.
- **MODULATION MATRIX PRO** : deux vraies connexions routées sur les paramètres des DSP, la première avec une source/destination/profondeur choisies individuellement, la deuxième légère pour l'expressivité. Les 30 autres slots sont explicitement remis à zéro lors du rappel pour éviter la contamination entre programmes.
- **SPECTRAL VIEW / Transient Shape** : comparaison FFT AVANT/APRÈS disponible pour tous, avec réglage de transitoires positif ou négatif, borné selon la famille et adapté aux attaques.
- **PRESET LAB** : aucun bouton décoratif ajouté ; les nouvelles sonorités peuvent être comparées, modifiées, annulées et capturées en variations.

Le son des projets utilisateurs **sauvegardés** avec la v1.2 est conservé : leur état chargé via `setStateInformation` est indépendant des nouvelles valeurs chargées depuis le programme d'usine. En revanche, sélectionner volontairement un programme d'usine v1.3 change bien son son par rapport à l'ancien programme du même nom.

## 100 programmes, par familles

1. **Cinematic Atmospheres** (1–10) : nuages spectro-spatiaux, résonances lentes, profondeur et trajectoires variées.
2. **Vocal Alchemy** (11–20) : transitoires de consonnes préservés, harmonies, réactions contrôlées, FX à bonne intelligibilité.
3. **Guitar Transformations** (21–30) : picking, dimension spatiale, harmoniques, feedback contenu, Fluid/Crystal.
4. **Drums and Percussion** (31–40) : transitoires physiques renforcés, pulsations synchronisées, résos Metal/Crystal, tails plus courts.
5. **Synthesizer Textures** (41–50) : forte présence de matrice, dialogues de matériaux et transformations morphables.
6. **Bass and Low Frequencies** (51–60) : image réduite, sortie contenue, graves prioritaires, excitation harmonique prudente.
7. **Ambient and Chill** (61–70) : profondeur douce, mouvements très lents, feedback modéré, élargissement musical.
8. **Experimental and Glitch** (71–80) : modulations aléatoires, attaques contrastées, réactions bidirectionnelles plus audacieuses.
9. **Rhythmic Motion** (81–90) : trajectoires et LFO à division musicale, percussions pilotant le spectre et les matières.
10. **Mixing and Enhancement** (91–100) : effets mesurés, wet faible, largeur modérée et timbres fins.

Les programmes emblématiques sont explicitement traités : *Crystal Universe* (DNA + Morph + Reactions), *Liquid Galaxy* (DNA + quatre scènes + trajectoire), *Metal Machine* (DNA + Metal/Crystal Reactions + Matrix), *Quantum Glitch* (DNA + Matrix + trajectoire), *Ultimate Organix* (ensemble des cinq technologies, en niveau et densité volontairement modérés).

## Mesures et validation honnête

Le workflow macOS v1.3 ajoute :
- Test C++ d'exactement 100 réglages distincts, sans doublon accidentel, et des limites de feedback/gain/largeur (10 basses et 10 presets de mixage).
- Audit des **100 profils écrits**, sur des signaux d'essai reproductibles. Ce banc couvre le DSP de base et les interactions MATERIAL REACTIONS, mais **ne restitue pas entièrement** les trajectoires 4D, MATTER DNA, Modulation Matrix, transitoires ni la chaîne hôte JUCE.
- Artifact secondaire de **100 WAV synthétiques** avec `factory_preset_audio_qa.csv` (RMS, crête, marge de gain recommandée) et `factory_sound_design_manifest.csv` (100 lignes de réglages/technologies). Aucune correction de gain n'est appliquée automatiquement sur la base de ces seules mesures.
- Compilations AU/VST3 macOS ARM64, plus **vérification obligatoire de l'Artifact réel** : les deux ZIP sont publiés, téléchargés et ouverts depuis la CI avant le statut vert.

### À faire avant de dire « 100 sons validés à l'oreille »

Utiliser un lot de sources réalistes (voix chantée, guitare clean/crunch, batterie, basse, synthé) et écouter **chaque programme** sur une même source de sa catégorie. Relever crête, RMS/LUFS, distorsion, clics, stabilité et charge CPU puis comparer au bypass à niveau équivalent. Valider l'effet des cinq nouvelles technologies et l'identité musicale avant de modifier définitivement les valeurs sur base d'écoute.

**Ni le rapport numérique ni les WAV synthétiques ne constituent une écoute subjective humaine des 100 presets.** Une telle affirmation avant audition réelle serait trompeuse. Ce travail pose les 100 designs distincts, techniquement exploitables et testables ; l'ajustement artistique final reste à mener à l'oreille.
