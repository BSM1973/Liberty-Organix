#pragma once
#include <JuceHeader.h>
#include "OrganixUITheme.h"
#include "FactoryPresets.h"
#include <array>

namespace organix::ui
{
// GUI locale is a personal preference, independent of any project's audio state.
inline juce::String word (bool french, const char* fr, const char* en)
{
    return juce::String::fromUTF8 (french ? fr : en);
}
// Tooltip copy is shared by all rotary controls and follows FR/EN changes.
inline juce::String dialTooltip (bool french, const juce::String& label)
{
    return label + word (french,
        " - glisser pour régler ; double-clic pour réinitialiser",
        " - drag to adjust; double-click to reset");
}
inline juce::String preciseValueTooltip (bool french)
{
    return word (french, "Cliquer pour saisir une valeur précise",
                         "Click to enter an exact value");
}
inline juce::String category (int index, bool french)
{
    static constexpr std::array<const char*, 11> fr {{
        "Toutes les catégories", "Ambiances cinématiques", "Alchimie vocale",
        "Transformations de guitare", "Batteries et percussions",
        "Textures de synthétiseur", "Basses et fréquences graves",
        "Ambient et détente", "Expérimental et glitch",
        "Mouvements rythmiques", "Mixage et amélioration"
    }};
    if (! french) return index == 0 ? "All categories" : organix::categoryName (index);
    return (index >= 0 && index <= 10) ? juce::String::fromUTF8 (fr[static_cast<size_t> (index)]) : "Autre";
}
inline juce::String materialLabel (int index, bool french)
{
    static constexpr std::array<const char*, 6> fr {{
        "LIQUIDE", "CRISTAL", "PLASMA", "VAPEUR", "MÉTAL", "LUMIÈRE"
    }};
    return french ? juce::String::fromUTF8 (fr[static_cast<size_t> (juce::jlimit (0, 5, index))])
                  : juce::String (name[static_cast<size_t> (juce::jlimit (0, 5, index))]);
}
inline juce::String materialDescription (int index, bool french)
{
    static constexpr std::array<const char*, 6> fr {{
        "Délai fluide et mouvement de phase",
        "Réseau de résonances harmoniques",
        "Excitation électrique non linéaire",
        "Atmosphère granulaire vivante",
        "Résonance métallique inharmonique",
        "Mouvement harmonique prismatique"
    }};
    index = juce::jlimit (0, 5, index);
    return french ? juce::String::fromUTF8 (fr[static_cast<size_t> (index)])
                  : juce::String (description[static_cast<size_t> (index)]);
}
inline juce::String property (int material, int index, bool french)
{
    static constexpr std::array<std::array<const char*, 12>, 6> fr {{
        {{ "FLUIDITÉ", "VISCOSITÉ", "COURANT", "ONDULATION", "PROFONDEUR", "TURBULENCE", "TAILLE ONDE", "SURFACE", "PRESSION", "VITESSE FLUX", "RÉINJECTION", "FLUX STÉRÉO" }},
        {{ "CRISTALLISATION", "FACETTES", "BRILLANCE", "RÉSONANCE", "ORDRE HARM.", "SCINTILLEMENT", "PURETÉ", "CROISSANCE", "FRACTURE", "DÉSACCORD", "ÉTINCELLES", "DÉCROISSANCE" }},
        {{ "ÉNERGIE", "VOLTAGE", "DÉCHARGE", "ARC", "INSTABILITÉ", "CHALEUR", "IONISATION", "ÉTINCELLES", "DÉCALAGE FRÉQ.", "DENSITÉ PLASMA", "PULSATION", "CONFINEMENT" }},
        {{ "VAPORISATION", "DENSITÉ", "TAILLE GRAIN", "DÉRIVE", "TAILLE NUAGE", "DIFFUSION", "AIR", "BRUME", "MOUVEMENT", "DISTANCE", "DISPERSION", "DISSOLUTION" }},
        {{ "MATÉRIAU", "DURETÉ", "ÉPAISSEUR", "TENSION", "RÉSONANCE", "SONNERIE", "IMPACT", "VIBRATION", "FRACTURE", "AMORTISSEMENT", "CORPS", "SCINTILLEMENT" }},
        {{ "LUMINOSITÉ", "RAYONNEMENT", "SPECTRE", "PRISME", "RÉFRACTION", "FAISCEAU", "HALO", "ÉCLAT", "SCINTILLEMENT", "PULSATION", "VITESSE", "EXPANSION" }}
    }};
    material = juce::jlimit (0, 5, material);
    index = juce::jlimit (0, 11, index);
    return french ? juce::String::fromUTF8 (fr[static_cast<size_t> (material)][static_cast<size_t> (index)])
                  : juce::String (controls[static_cast<size_t> (material)][static_cast<size_t> (index)]);
}
inline juce::PropertiesFile::Options preferenceOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "Organix";
    options.filenameSuffix = "settings";
    options.folderName = "Liberty Music Software/Organix";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    return options;
}
inline juce::File userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Liberty Music Software").getChildFile ("Organix")
        .getChildFile ("Presets");
}
} // namespace organix::ui
