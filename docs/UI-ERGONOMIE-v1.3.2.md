# ORGANIX v1.3.2 - ERGONOMIE ET AFFICHAGE 13 POUCES

## Objectif et perimetre
- Ne pas modifier les moteurs DSP, le routage des presets ni les 100 noms d'usine.
- Garder l'interface existante : materiaux, champ sonore, 12 commandes A/B, bibliotheque,
  Reactions, Matrix, DNA, Morphing, Trajectory et Preset Lab.
- Ouvrir une fenetre *entiere*, sans scrollbar globale ni poignee de redimensionnement.
  La liste interne des 100 presets reste naturellement defilable dans la bibliotheque.

## Architecture
`OrganixViewportEditor` est le seul `juce::AudioProcessorEditor` expose au DAW.
Il encapsule l'interface precedente devenue `juce::Component`. Un seul facteur
`juce::AffineTransform::scale` est applique au contenu : le rendu, les hitboxes, les
menus, les deux couches de materiaux, les onglets, les panneaux et le browser restent
dans le meme repere logique.

`OrganixViewport::chooseViewport` utilise `Display::userArea`, en points logiques.
Il reserve 72 points horizontalement et 96 verticalement pour les marges du DAW,
choisit 1240x820 sur petit ecran et 1504x1008 seulement si l'espace le permet,
puis reduit uniformement la taille de la fenetre si necessaire. Il ne change jamais
la resolution d'ecran ni le facteur de zoom global de JUCE.

## Corrections d'interaction
- Les potentiometres associes a l'APVTS se reinitialisent par double-clic
  a la valeur par defaut du VRAI parametre, au lieu d'un 50% generique.
- Les 12 reglages de materiau de la couche A/B sont correctement rebornes
  a chaque changement de couche.
- Les valeurs affichent 1 decimale pour le pourcentage ; la saisie numerique
  prend aussi en charge la virgule francaise et ignore les saisies non numeriques.
- Les valeurs et etiquettes ont une police plus lisible. La commande B ACTIF
  dispose d'une surface cliquable agrandie.
- Les commandes du panneau des materiaux sont masquees quand Matrix occupe
  l'ecran, pour eviter une interaction avec des boutons sous-jacents.

## Tests automatises disponibles
- `ui_geometry.cpp` : non-chevauchement du layout a toutes les tailles
  logiques supportees (1240..1940, 820..1220).
- `ui_viewport.cpp` : calcul et contraintes de fenetre pour neuf tailles
  de zone de travail, en particulier 1280x800, 1280x720 et 1440x900.
- 15 tests C++ au total, plus compilation AU/VST3 et verification des Artifacts
  obligatoires dans le workflow GitHub.
- Les 15 tests et la compilation sont necessaires, pas suffisants pour valider
  le clic sur une interface graphique ni les polices de tous les DAW.

## Grille de recette manuelle - A VALIDER dans un DAW
Ne pas marquer ces lignes comme reussies sans les avoir essayees dans le plugin :

| Interface | Manipulation | Resultat attendu |
|---|---|---|
| Fenetre 13 pouces | Ouvrir sur 1280x800 puis 1440x900 | Fenetre complete, pas de scrollbar globale ni de bouton hors ecran |
| Navigation principale | Parcourir les onglets et changer FR/EN | Aucun element superpose, etiquettes entieres et accents lisibles |
| Material A/B | Choisir six moteurs puis passer de A a B | DSP et 12 potards de la bonne couche, couleurs et labels synchronises |
| Valeurs | Glisser, entrer 55,5 %, double-cliquer | Valeurs coherentes, retour au defaut du parametre, automation DAW |
| Fusion | A+B, A>B, B>A, AXB | Routage sonore audible et state recall |
| Mouvement | Libre/Sync, vitesse, division | Valeurs synchronisees au tempo, controle desactive en mode Libre |
| Reactions | Activer, direction, quantite, reponse | Reactions audibles et mesures d'activite coherentes |
| Spectral View | Avant/Apres et transitoires | Mesures liees au signal, reglage de transitoires audible |
| DNA | Activer et modifier les six commandes | Changement de comportement DSP et metriques synchronisees |
| Morph | XY, scenes A-D, capture, rappel | Mouvements previsibles sans deplacements graphiques errones |
| Trajectory | Lancer/arreter, sync, enregistrer | Courbe, transport et parametres sauvegardes |
| Matrix | 32 routes, sources, cibles, polarite | Affectation audible et mise a jour en DAW |
| Browser | Chercher, filtrer, favoris, charger, sauver | Liste et etat utilisateur conserves, aucune commande masquee |
| Preset Lab | A/B, Undo/Redo, verrouiller, varier | Operations reproductibles et sans changement de niveau non desire |
| Hosts | Live, Logic, Bitwig : charger/fermer/rappeler | Aucune coupure, popup ou fenetre tronquee |

### Limite actuelle
Il n'existe pas encore de capture d'ecran ou d'essai de clic reel sur un
MacBook 13 pouces dans la CI. Le test `ui_viewport` est une simulation du calcul
de dimensions, pas une verification physique dans Logic/Ableton/Bitwig.
