# ORGANIX v1.0 — MODULATION MATRIX PRO

## Fonction principale

Organix possède **32 affectations réellement raccordées aux moteurs DSP**, avec 20 sources et 40 destinations. La matrice est **désactivée par défaut** : les sessions Organix v0.9 et les 100 presets d'usine conservent leur son si l'utilisateur ne l'active pas.

## 32 liaisons sauvegardées et automatisables

Chaque ligne MM 01..MM 32 a cinq paramètres APVTS propres :
- `mmXXEnable`: active/désactive la liaison.
- `mmXXSource`: choisit l'une des 20 sources (plus OFF).
- `mmXXTarget`: choisit l'une des 40 destinations DSP (plus OFF).
- `mmXXDepth`: profondeur signée de -100 à +100 %.
- `mmXXPolarity`: unipolaire ou bipolaire.

Ces paramètres sont sauvés avec l'état du plugin et les presets utilisateur. Une même destination accepte plusieurs liaisons simultanées ; leurs contributions sont additionnées puis bornées. Tous les réglages d'une session sans matrice restent compatibles.

## 20 sources disponibles

- Quatre LFO sinusoïdaux indépendants, chacun réglable en fréquence. Mode libre en Hz ou synchronisation au tempo hôte, avec choix de division musicale commune.
- Deux enveloppes AD déclenchées **par les attaques du signal audio**, chacune avec attaque et décroissance indépendantes. Elles ne sont pas de faux curseurs : elles calculent une enveloppe dans la callback audio.
- Quatre suiveurs audio : niveau RMS réel, transitoires, énergie spectrale aiguë (« souffle ») et mouvement spectral. Les deux derniers réutilisent l'analyse FFT de MATTER DNA, même lorsque l'animation DNA est inactive.
- Deux sources aléatoires, retenue (Sample & Hold) et glissement (interpolation entre deux états aléatoires). Vitesse de génération réglable.
- Les huit macros existantes (Transformer, Mouvement, Énergie, Densité, Espace, Harmoniques, Évolution, Fusion).

## 40 destinations DSP réellement modifiées

- 18 pour la couche A et 18 pour la couche B : Amount, Motion, Tone, Feedback, Density, Energy et les 12 commandes de forme.
- Les six premières commandes de forme alimentent directement le DSP. Les commandes de forme 07..12 reçoivent également une correspondance sonore supplémentaire afin que leur modulation ne soit pas purement graphique.
- Quatre destinations globales : Fusion, Équilibre, Dry/Wet, Largeur stéréo.
- Le DSP classique réévalue les valeurs modulées toutes les 128 trames. En Morphing 4D, les réglages des quatre scènes et leurs sorties sont affectés ; MATTER DNA et MATERIAL REACTIONS continuent de fonctionner. Le contrôle XY et les trajectoires de la v0.8 restent indépendants de la matrice.

## Lissage et temps réel

- Aucune allocation ni accès fichier/UI dans le traitement de la matrice ; taille fixe de 32 liaisons.
- Calcul de chaque segment de 128 échantillons à partir du **signal d'entrée avant modification**. Les suiveurs et générateurs restent déterministes et continus entre blocs audio.
- Lissage par liaison d'environ 12 ms, plafonnement de la somme par destination, limites de feedback et bornage stéréo pour diminuer les risques de clics ou de valeurs incontrôlées.
- Les sources LFO/enveloppe/rand sont calculées pour chaque segment de contrôle ; ce n'est **pas une modulation de paramètres à chaque échantillon**, distinction importante pour le niveau de promesse.
- Les lectures de paramètres hôtes sont atomiques ; les indicateurs d'activité sont publiés depuis l'audio via 32 atomiques non bloquantes.

## Interface native premium FR/EN

Cliquer **MATRICE** dans l'inspecteur de droite (quatrième onglet) ouvre un vrai espace de travail plein format sous la barre supérieure, sans modifier la disposition des commandes existantes. La fenêtre affiche les 32 lignes (16 par colonne), sélection de ligne, source, destination, profondeur positive/négative, polarité et activation individuelle. Les barres de niveau montrent l'activité DSP réelle de chaque liaison.

Les quatre vitesses LFO, le mode FREE/SYNC, la division, la vitesse aléatoire, les deux paires Attack/Decay, la profondeur globale et l'activation générale ont leurs propres contrôles natifs. **EXEMPLE SONORE** crée six routes audibles ; **RETOUR** referme le workspace. Les libellés sont en français par défaut et peuvent être basculés en anglais. L'onglet de changement de langue FR/EN conserve de simples lettres sans symboles parasites.

## Vérifications obligatoires

- Dixième test pur C++ : 32 routes à la fois, LFO positif/négatif, ADSR simplifié (AD) réel, sources audio et aléatoires, toutes les 40 destinations, conservation stricte des anciens paramètres lorsque désactivée, résistance à NaN et à la saturation.
- Test des 756 dimensions de l'UI pour les quatre onglets, les 32 lignes, le panneau d'édition et les commandes d'enveloppe sans chevauchement.
- Vérification UTF-8/NFC des nouveaux textes.
- Build JUCE AU/VST3 macOS ARM64 et **Artifact obligatoire réellement téléchargé et vérifié par GitHub Actions**. Pas de fusion avant la validation des gates du workflow.
- Tests d'écoute et de performance dans de vrais DAW encore nécessaires pour confirmer les transitions et les coûts CPU lorsque huit moteurs 4D sont actifs.

## Limites assumées

L'enveloppe est un AD déclenché par un transitoire audio, pas encore un ADSR MIDI complet. Les quatre LFO sont sinusoïdaux dans cette version (courbes LFO éditables, steps et graphes de modulation à ajouter ensuite). Les destinations sont les paramètres normalisés internes, pas des sorties MIDI CC. Les 32 lignes de matrice sont indépendantes de la capture A/B/C/D : elles s'appliquent globalement au Morphing 4D.
