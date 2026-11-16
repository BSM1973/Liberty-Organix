# ORGANIX v0.7 — QUAD MORPH / MORPHING QUATRE SCÈNES

## Objectif
Quatre scènes sonores indépendantes **A, B, C, D**, placées aux coins d'un pad XY. Le musicien peut capturer quatre sons du plugin, puis effectuer un voyage continu entre leurs identités. La priorité est un véritable traitement audio et des changements de moteur sans coupure, pas un simple curseur graphique.

## Usage
1. Sélectionner un matériau A/B et régler son son sur la page principale.
2. Ouvrir **MORPH** dans l'en-tête.
3. Cliquer **CAPTURER A** pour mémoriser le son actuel dans le coin supérieur gauche.
4. Revenir à l'interface principale, préparer un second son, puis **CAPTURER B**. Recommencer pour C et D.
5. Dans MORPH, cliquer **ACTIVER MORPH** (désactivé par défaut). Déplacer le point sur le pad XY ou employer **ÉCOUTER A/B/C/D** pour rejoindre directement un coin.
6. Automatiser les paramètres hôtes `morphX` et `morphY` dans le DAW. Les positions sont lissées, même lorsque l'automation saute brutalement d'une valeur à l'autre.

## Traitement audio
- Quatre scènes, chacune conservant ses deux matériaux, leur activation, leur ordre/routage, les 24 réglages de forme et les paramètres DSP usuels.
- Les **paramètres continus** (amount/motion/tone/feedback, 24 shape controls, énergie, densité, fusion, balance, dry/wet, width) suivent une interpolation bilinéaire des quatre scènes.
- Les **choix discrets de moteurs et le routage** ne sont pas interpolés : les quatre paires de moteurs JUCE natifs traitent le son en parallèle, puis leurs sorties sont mélangées selon les quatre coefficients XY (la somme vaut 1).
- Pour une nouvelle capture qui change le matériau, deux banques DSP préparées à l'avance permettent un fondu d'environ 85 ms entre l'ancien et le nouveau moteur, sans allocation audio.
- Le passage du mode de traitement classique au morphing s'effectue également en fondu, avec environ 85 ms de transition. Le pad XY est lissé sur environ 80 ms pour réduire les clics d'automation.
- Les moteurs des quatre scènes reçoivent le signal d'entrée même quand leur poids est nul afin de maintenir leur mémoire DSP/tails lors du mouvement.
- **MATTER DNA reste disponible** : la mesure de l'attaque, du spectre et de l'énergie peut moduler chaque scène de morphing, si activée.

## Compatibilité et sauvegarde
- `morphEnable` vaut OFF par défaut. Les projets antérieurs ne doivent pas changer de son.
- Chaque scène est sauvegardée dans le ValueTree de l'APVTS avec 44 valeurs de capture (quatre identités discrètes, 36 réglages de moteur et quatre globaux).
- Des mailboxes constituées de valeurs atomiques transmettent les captures au thread audio ; ce dernier ne lit jamais directement le ValueTree et ne réalise aucune allocation à la capture.
- Lors du chargement d'un ancien projet sans scènes, le plugin crée quatre scènes identiques à partir des paramètres courants.
- FR par défaut, EN disponible, aucun symbole décoratif incompatible dans le sélecteur de langue.

## Tests automatisés
- 4 coins XY et 10 201 positions vérifiées pour garantir que les coefficients sont positifs et totalisent 1.
- Aller-retour des 44 valeurs d'une scène, nettoyage des données invalides/non finies.
- Interpolation vérifiable des valeurs continues, rendu réel de quatre paires de moteurs en stéréo, capture/réaffectation d'un moteur pendant l'audio, fondu terminé et sortie finie.
- Test d'absence de chevauchement dans les tailles prises en charge, contrôle UTF-8 et compilation JUCE macOS ARM64 AU/VST3.

## Limites actuelles
- La v0.7 utilise **quatre scènes, chacune avec deux moteurs**, donc jusqu'à huit moteurs actifs durant le morphing. Le coût CPU et la mémoire augmentent ; la charge doit être contrôlée dans les vrais DAW.
- Pas de morphing spectral avec phase commune ; les traitements parallèles sont mixés dans le domaine audio, et des annulations de phase peuvent se produire sur des signaux fortement corrélés.
- Les changements de matériel et réglages exigent encore une validation d'écoute A/B, y compris des captures répétées avec feedback élevé.
- L'interface premium précédente reste conservée, avec une page MORPH supplémentaire ; aucun changement ne doit être fusionné dans `main` tant que le build AU/VST3 n'est pas vert.
