# ORGANIX v0.8 — TRAJECTORY DESIGNER

## Objectif
Tracer le mouvement du son à la souris, puis animer réellement les quatre scènes sonores A/B/C/D par un déplacement XY. Modes CERCLE, SPIRALE, ZIGZAG et DESSIN LIBRE. Français au premier lancement, contrôle FR/EN conservé.

## Utilisation
1. Capturer quatre sons différents dans les scènes A/B/C/D de MORPH.
2. Cliquer sur l'onglet **MORPH** en haut du plugin ; le champ XY central et le panneau TRAJECTORY DESIGNER s'affichent.
3. Choisir CERCLE, SPIRALE ou ZIGZAG, puis **ACTIVER** : le curseur parcourt réellement les quatre scènes, ce qui transforme le son.
4. Pour un trajet personnalisé, cliquer **DESSINER**, puis déplacer la souris dans le champ central. Relâcher pour mémoriser le tracé. Le mode DESSIN LIBRE est sélectionné et la lecture est activée automatiquement.
5. Choisir **LIBRE** pour une vitesse en Hz, ou **SYNC** pour synchroniser la trajectoire au tempo hôte et sélectionner une division musicale.
6. Utiliser **AVANT / ARRIÈRE** pour changer le sens. **PROFONDEUR** règle la proportion de mouvement appliquée aux valeurs du pad.
7. Automatiser ces commandes depuis le DAW. Tous les réglages du designer sont des paramètres hôtes. La courbe personnalisée est sauvegardée avec le preset et le projet.

## DSP réel
- Une trajectoire fournit un point normalisé XY à chaque échantillon audio.
- Ces coordonnées alimentent les poids bilinéaires des quatre scènes du moteur Morph 4D. Les réglages sont interpolés par bloc, tandis que les quatre sorties DSP sont mélangées au rythme de l'échantillon.
- Le mode trajectoire active automatiquement la voie audio du morphing, sans obliger l'utilisateur à activer MORPH séparément.
- Le lissage XY sur environ 7 ms limite les discontinuités ; la transition du traitement classique vers Morph 4D est déjà fondue sur environ 85 ms.
- Les formes fermées évitent les sauts en boucle. Le dessin libre est interpolé de manière cyclique, y compris entre le dernier et le premier point.
- Le dessin libre utilise un maximum de 64 points et ne nécessite aucun conteneur dynamique dans la callback audio.
- La boîte atomique des points est relue uniquement aux frontières de blocs ; aucun verrou, accès disque ou allocation dans la callback audio.
- Les quatre scènes indépendantes restent utilisables avec MATTER DNA activé.

## Sauvegarde
Les paramètres `trajectoryEnable`, `trajectoryShape`, `trajectorySpeed`, `trajectoryFree`, `trajectoryDivision`, `trajectoryReverse` et `trajectoryDepth` sont conservés dans APVTS.

Les points personnalisés sont sérialisés sous `trajectoryPoints` dans l'état du plugin, puis transférés au DSP par une boîte atomique à version. Les presets anciens sans trajectoire restent compatibles ; la lecture est OFF par défaut.

## Contrôles automatiques
- Huitième test C++ consacré aux quatre modes, bouclage, inversion, vitesse libre/synchronisée, profondeur et changement effectif des poids Morph 4D.
- Vérification du placement du panneau de droite avec le champ central et les autres pages, en tailles 1240×820 à 1940×1220.
- Vérification anti-caractères abîmés UTF-8/NFC des nouveaux textes.
- Compilation macOS AU/VST3 ARM64 avant fusion sur `main`.

## Limites de la v0.8
L'édition manuelle des points, la quantification des segments, le time-stretch d'une trajectoire et les courbes Bézier restent à développer. La lecture est bouclée et sa vitesse est basée sur le tempo du DAW lorsque SYNC est choisi, sans arrimage obligatoire à la position de lecture absolue du transport. Le fondu amortit les changements mais doit encore être validé à l'écoute, particulièrement avec des feedbacks longs.
