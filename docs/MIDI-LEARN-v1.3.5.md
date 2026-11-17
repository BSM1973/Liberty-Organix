# ORGANIX v1.3.5 — MIDI Learn

## Utilisation (français)
1. Dans le DAW, acheminer le MIDI du contrôleur vers l'instance ORGANIX.
2. Effectuer un **clic droit sur un potentiomètre** ou une commande des pages
   Principale, ADN, Morph, Trajectoire, Réactions ou Matrice.
3. Choisir « Apprendre un CC MIDI ». Le bouton « MIDI » affiche « CC... ».
4. Déplacer un potentiomètre ou fader matériel émettant un **CC 1 à 119**.
5. La commande JUCE reçoit les valeurs normalisées 0–127, via l'APVTS.
6. Clic droit de nouveau : l'affectation « CH X / CC Y » est visible et peut
   être oubliée. Le bouton MIDI permet aussi d'effacer toutes les affectations.

Le menu du bouton MIDI affiche **tous les paramètres automatisables**, y
compris les réglages des panneaux fermés. Les 16 canaux MIDI sont indépendants.
Le plugin ne consomme ni ne supprime les messages MIDI entrants.

## Sauvegarde et projet DAW
Les affectations CC sont enregistrées par ID de paramètre (non par index),
dans l'état XML JUCE `midiLearnMappingsV1`. Elles sont préservées au
rappel de presets et lors d'une sauvegarde/recharge de projet Ableton, Logic
ou Bitwig, sous réserve du fonctionnement normal de la restauration d'état hôte.

## Temps réel
Le traitement audio reçoit les CC sans allocation ni prise de verrou :
seules les valeurs normalisées et leur statut sont publiés dans des atomiques.
Un timer du **processeur** effectue les notifications hôte sur le thread UI,
jusqu'à 60 fois par seconde, même quand l'éditeur est fermé.
Cette cadence évite de notifier le DAW depuis le callback audio, avec en
contrepartie une latence de contrôle de l'ordre d'une trame de timer et
l'absence de garantie en mode rendu hors ligne sans boucle de messages.

## Validation
Le test `organix_midi_learn` exerce le vrai `OrganixAudioProcessor` :
apprentissage, 2 canaux distincts, CC persistants, rappel preset, stockage de
session, réouverture sans éditeur, oubli et annulation. Ce test ne prouve
pas que chaque DAW transfère les CC au plugin ; cette validation reste
une recette séparée à réaliser physiquement dans les hôtes.

## English
Right-click a control and choose "Learn MIDI CC", then turn a physical MIDI
knob (CC 1–119). The top "MIDI" menu lists all host-automatable controls.
Mappings are per MIDI channel, saved in host project state, and can be cleared.
For offline renders, host-message-thread scheduling needs explicit testing.
