# ORGANIX v0.4 — Navigateur de presets et interface bilingue

## Règle principale
Français au premier lancement. L'utilisateur peut passer en anglais avec le sélecteur **FR · Français / EN · English** en haut. Son choix est enregistré via `juce::PropertiesFile`. Le logiciel est toujours commercialisé sous le nom Organix ; les noms d'origine des 100 presets d'usine sont conservés.

## Nouveau routage
Le menu déroulant de la v0.3 est remplacé par quatre tuiles (2 × 2) :
- Parallèle — A+B
- Série A→B — A>B
- Série B→A — B>A
- Fusion croisée — A×B

Chaque tuile modifie **exactement** l'ancien paramètre automatisable `routing`, sans créer de paramètre incompatible avec les projets existants. Les changements venant d'un DAW sont reflétés par la synchronisation GUI.

## Navigateur natif
Ouverture par le bouton Bibliothèque ou par le nom de preset dans la barre d'en-tête.

- 100 presets d'usine accessibles dans une liste défilante (noms, catégories, matériaux A/B).
- Recherche instantanée par nom, description, catégorie ou matériau.
- Filtres indépendants : 10 catégories, 6 matériaux, usine/personnel/tout.
- Favoris persistants, filtre « Favoris uniquement ».
- Cliquer une ligne, puis « Charger », ou double-cliquer la ligne.
- Enregistrer l'état courant sous un nom de preset personnel.
- Charger un preset personnel depuis la bibliothèque.
- Importer un fichier .organix, ou exporter un preset personnel au format .organix.
- Identité des presets personnels sauvegardée dans l'état de l'hôte pour réaffichage au réouverture.

## Stockage
Dossier utilisateur macOS : `~/Library/Application Support/Liberty Music Software/Organix/Presets` (déterminé par l'API JUCE). Les favoris et la préférence de langue utilisent le stockage normal JUCE des préférences applicatives. Aucune modification manuelle de ce dossier n'est nécessaire.

## Limitations de cette première version
- Les filtres par matériau concernent les presets d'usine, dont la catégorie et les moteurs sont explicitement connus. Les fichiers utilisateur n'ont pas encore d'index de métadonnées matériau.
- La traduction couvre l'interface générale, les six moteurs, les 72 noms de commandes, les catégories et le navigateur. Les **noms des presets d'usine et leurs descriptions originales** ne sont pas traduits automatiquement.
- Le routage sonore conserve le DSP de la v0.3.
- Les bancs personnels et l'explorateur de fichiers doivent encore être testés visuellement dans un véritable DAW avant diffusion commerciale.

## Validation
- C++ : smoke DSP, presets, 72 contrôles, 756 géométries.
- JUCE : compilation macOS ARM64 des AU et VST3 et test des nouveaux composants (routing, navigateur, FileChooser).
- DAW : chargement, sauvegarde, rappel du preset personnel, filtre, favoris après réouverture, changement FR/EN et états automatisés du routage.
- Tests requis avant fusion sur `main`. Ne jamais présenter un workflow en attente comme un build vert.
