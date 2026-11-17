#include "OrganixPresetBrowser.h"
#include <algorithm>

namespace ui = organix::ui;
namespace
{
void configure (juce::TextButton& button, bool primary = false)
{
    button.setColour (juce::TextButton::buttonColourId, primary ? juce::Colour (0xff153d43) : ui::surface);
    button.setColour (juce::TextButton::textColourOffId, primary ? ui::accentFor (0) : ui::text);
}
void configure (juce::ComboBox& combo)
{
    combo.setColour (juce::ComboBox::backgroundColourId, ui::inset);
    combo.setColour (juce::ComboBox::textColourId, ui::text);
    combo.setColour (juce::ComboBox::outlineColourId, ui::border);
}
void configure (juce::TextEditor& editor)
{
    editor.setColour (juce::TextEditor::backgroundColourId, ui::inset);
    editor.setColour (juce::TextEditor::textColourId, ui::text);
    editor.setColour (juce::TextEditor::outlineColourId, ui::border);
    editor.setColour (juce::TextEditor::focusedOutlineColourId, ui::accentFor (0));
    editor.setColour (juce::TextEditor::highlightColourId, ui::accentFor (0).withAlpha (0.27f));
}
}

OrganixPresetBrowser::OrganixPresetBrowser (OrganixAudioProcessor& p, juce::PropertiesFile& prefs)
    : audio (p), settings (prefs), lab(p), list ("ORGANIX_PRESET_LIBRARY", this)
{
    favoriteKeys = juce::StringArray::fromTokens (settings.getValue ("favorites"), "|", "");
    favoriteKeys.removeEmptyStrings();
    list.setRowHeight (48);
    list.setMultipleSelectionEnabled (false);
    list.setColour (juce::ListBox::backgroundColourId, ui::surface);
    list.setColour (juce::ListBox::outlineColourId, ui::border);
    list.setOutlineThickness (1);
    list.setWantsKeyboardFocus (true);

    configure (search);
    search.onTextChange = [this] { rebuildResults(); };
    configure (saveName);
    saveName.setReturnKeyStartsNewLine (false);

    for (auto* c : { &category, &material, &origin, &character })
    {
        configure (*c);
        addAndMakeVisible (*c);
        c->onChange = [this] { rebuildResults(); };
    }
    for (auto* b : { &favoriteOnlyButton, &favoriteButton, &loadButton, &saveButton,
                      &importButton, &exportButton, &closeButton, &labButton })
    {
        configure (*b);
        addAndMakeVisible (*b);
    }
    configure (loadButton, true);
    configure (saveButton, true);
    favoriteOnlyButton.onClick = [this]
    {
        favoritesOnly = ! favoritesOnly;
        rebuildResults();
    };
    favoriteButton.onClick = [this] { toggleFavorite(); };
    loadButton.onClick = [this] { applySelection(); };
    saveButton.onClick = [this] { saveCurrent(); };
    importButton.onClick = [this] { importPreset(); };
    exportButton.onClick = [this] { exportSelected(); };
    closeButton.onClick = [this] { if (onDismissed) onDismissed(); };
    labButton.onClick = [this] { setLabOpen(true); };
    lab.onClose = [this] { setLabOpen(false); };
    for (auto* l : { &countLabel, &messageLabel })
    {
        l->setColour (juce::Label::textColourId, ui::secondary);
        l->setFont (juce::Font (juce::FontOptions (11.0f)));
        addAndMakeVisible (*l);
    }
    addAndMakeVisible (search);
    addAndMakeVisible (saveName);
    addAndMakeVisible (list);
    addAndMakeVisible (lab);
    lab.setVisible(false);
    setFrench (true);
}
OrganixPresetBrowser::~OrganixPresetBrowser()
{
    chooser.reset();
    list.setModel (nullptr);
}
void OrganixPresetBrowser::setFrench (bool value)
{
    french = value;
    const auto rememberedCategory = category.getSelectedId();
    const auto rememberedMaterial = material.getSelectedId();
    const auto rememberedOrigin = origin.getSelectedId();
    const auto rememberedCharacter=character.getSelectedId();
    category.clear (juce::dontSendNotification);
    for (int i = 0; i <= 10; ++i)
        category.addItem (ui::category (i, french), i + 1);
    category.setSelectedId (rememberedCategory > 0 ? rememberedCategory : 1, juce::dontSendNotification);
    material.clear (juce::dontSendNotification);
    material.addItem (ui::word (french, "Tous les matériaux", "All materials"), 1);
    for (int i = 0; i < 6; ++i)
        material.addItem (ui::materialLabel (i, french), i + 2);
    material.setSelectedId (rememberedMaterial > 0 ? rememberedMaterial : 1, juce::dontSendNotification);
    origin.clear (juce::dontSendNotification);
    origin.addItem (ui::word (french, "Toute la bibliothèque", "Entire library"), 1);
    origin.addItem (ui::word (french, "Presets d'usine", "Factory presets"), 2);
    origin.addItem (ui::word (french, "Mes presets", "My presets"), 3);
    origin.setSelectedId (rememberedOrigin > 0 ? rememberedOrigin : 1, juce::dontSendNotification);
    character.clear(juce::dontSendNotification);
    for(int i=0;i<=11;++i)
        character.addItem(juce::String::fromUTF8(organix::characterLabel(
            static_cast<organix::PresetCharacter>(i),french)),i+1);
    character.setSelectedId(rememberedCharacter>0?rememberedCharacter:1,
                             juce::dontSendNotification);
    lab.setFrench(french);
    search.setTextToShowWhenEmpty (ui::word (french, "Rechercher un nom, un style, un matériau...",
                                                      "Search name, category or material..."), ui::subdued);
    saveName.setTextToShowWhenEmpty (ui::word (french, "Nom de mon preset...", "My preset name..."), ui::subdued);
    favoriteOnlyButton.setButtonText (ui::word (french, "FAV Favoris uniquement", "FAV Favorites only"));
    favoriteButton.setButtonText (ui::word (french, "+ Favori", "+ Favorite"));
    loadButton.setButtonText (ui::word (french, "CHARGER", "LOAD"));
    saveButton.setButtonText (ui::word (french, "ENREGISTRER", "SAVE"));
    importButton.setButtonText (ui::word (french, "IMPORTER", "IMPORT"));
    exportButton.setButtonText (ui::word (french, "EXPORTER", "EXPORT"));
    closeButton.setButtonText (ui::word (french, "FERMER  X", "CLOSE  X"));
    labButton.setButtonText("PRESET LAB");
    refreshLibrary();
}
void OrganixPresetBrowser::refreshLibrary()
{
    library.clear();
    library.reserve (125);
    for (int i = 0; i < static_cast<int> (organix::factoryPresets.size()); ++i)
    {
        const auto& p = organix::factoryPresets[static_cast<size_t> (i)];
        Entry e;
        e.factoryIndex = i;
        e.category = p.category;
        e.materialA = static_cast<int> (p.a);
        e.materialB = p.b;
        e.name = p.name;
        e.description = p.description;
        e.key = "factory:" + juce::String (i);
        library.push_back (std::move (e));
    }
    auto directory = ui::userPresetFolder();
    if (directory.exists() && directory.isDirectory())
    {
        juce::Array<juce::File> files;
        directory.findChildFiles (files, juce::File::findFiles, false, "*.organix");
        for (const auto& file : files)
        {
            Entry e;
            e.path = file;
            e.name = file.getFileNameWithoutExtension();
            e.description = ui::word (french, "Preset personnel  -  état complet des paramètres",
                                              "User preset  -  complete parameter state");
            e.key = "user:" + file.getFileName();
            library.push_back (std::move (e));
        }
    }
    rebuildResults (false);
}
bool OrganixPresetBrowser::isFavorite (const Entry& entry) const
{
    return favoriteKeys.contains (entry.key);
}
void OrganixPresetBrowser::persistFavorites()
{
    settings.setValue ("favorites", favoriteKeys.joinIntoString ("|"));
    settings.saveIfNeeded();
}
void OrganixPresetBrowser::rebuildResults (bool preserveSelection)
{
    juce::String selectionKey;
    const int selected = list.getSelectedRow();
    if (preserveSelection && selected >= 0 && selected < static_cast<int> (results.size()))
        selectionKey = results[static_cast<size_t> (selected)].key;
    results.clear();
    const auto query = search.getText().trim();
    const int categoryId = category.getSelectedId() - 1; // 0: all, 1..10: category
    const int materialId = material.getSelectedId() - 2; // -1: all, 0..5: material
    const int characterId=character.getSelectedId()-1;
    const int originId = origin.getSelectedId();         // 1: all, 2: factory, 3: user
    for (const auto& entry : library)
    {
        if (favoritesOnly && ! isFavorite (entry)) continue;
        if (originId == 2 && entry.isUser()) continue;
        if (originId == 3 && ! entry.isUser()) continue;
        if (categoryId > 0 && entry.category != categoryId) continue;
        if (materialId >= 0 && entry.materialA != materialId && entry.materialB != materialId) continue;
        if(characterId>0)
        {
            if(entry.isUser())continue; // user presets have no character metadata
            const auto& factory=organix::factoryPresets[
                static_cast<size_t>(entry.factoryIndex)];
            if(!organix::matchesCharacter(factory,
                static_cast<organix::PresetCharacter>(characterId)))continue;
        }
        if (query.isNotEmpty())
        {
            const auto tags = ui::category (entry.category, french) + " " +
                              (entry.materialA >= 0 ? ui::materialLabel (entry.materialA, french) : juce::String()) + " " +
                              (entry.materialB >= 0 ? ui::materialLabel (entry.materialB, french) : juce::String());
            juce::String soundTags;
            if(!entry.isUser())
                for(int ch=1;ch<=11;++ch)
                    if(organix::matchesCharacter(
                           organix::factoryPresets[static_cast<size_t>(entry.factoryIndex)],
                           static_cast<organix::PresetCharacter>(ch)))
                        soundTags+=" "+juce::String::fromUTF8(
                            organix::characterLabel(
                                static_cast<organix::PresetCharacter>(ch),french));
            if (! entry.name.containsIgnoreCase (query) &&
                ! entry.description.containsIgnoreCase (query) &&
                ! tags.containsIgnoreCase (query) &&
                ! soundTags.containsIgnoreCase (query)) continue;
        }
        results.push_back (entry);
    }
    list.updateContent();
    list.deselectAllRows();
    int selectIndex = -1;
    for (int i = 0; i < static_cast<int> (results.size()); ++i)
    {
        if (results[static_cast<size_t> (i)].key == selectionKey)
        {
            selectIndex = i;
            break;
        }
    }
    if (selectIndex >= 0) list.selectRow (selectIndex, false, true);
    else if (! results.empty()) list.selectRow (0, false, true);
    updateSelection();
    repaint();
}
int OrganixPresetBrowser::getNumRows()
{
    return static_cast<int> (results.size());
}
void OrganixPresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= static_cast<int> (results.size())) return;
    const auto& e = results[static_cast<size_t> (row)];
    const auto accent = e.materialA >= 0 ? ui::accentFor (e.materialA) : ui::engaged;
    auto b = juce::Rectangle<float> (1.0f, 1.0f, static_cast<float> (width - 3), static_cast<float> (height - 3));
    g.setColour (selected ? ui::inset.brighter (0.25f) : (row % 2 == 0 ? ui::surface : ui::panel));
    g.fillRoundedRectangle (b, 5.0f);
    if (selected)
    {
        g.setColour (accent.withAlpha (0.72f));
        g.drawRoundedRectangle (b, 5.0f, 1.0f);
        g.fillRoundedRectangle (3.0f, 7.0f, 3.0f, static_cast<float> (height - 14), 1.5f);
    }
    g.setColour (isFavorite (e) ? ui::accentFor (5) : ui::subdued);
    g.setFont (juce::Font (juce::FontOptions (14.0f)));
    g.drawFittedText (isFavorite (e) ? "F" : "+", { 12, 10, 24, 25 },
                      juce::Justification::centred, 1);
    const int rightBadge = 130;
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawFittedText (e.name, { 44, 5, juce::jmax (100, width - rightBadge - 54), 21 },
                      juce::Justification::centredLeft, 1, 0.85f);
    g.setColour (ui::secondary);
    g.setFont (juce::Font (juce::FontOptions (10.5f)));
    const auto detail = e.isUser() ? ui::word (french, "PERSONNEL  /  PARAMÈTRES SAUVEGARDÉS",
                                                     "USER  /  SAVED PARAMETERS")
                                   : ui::category (e.category, french) + "   /   " +
                                     ui::materialLabel (e.materialA, french);
    g.drawFittedText (detail, { 45, 25, juce::jmax (100, width - rightBadge - 54), 17 },
                      juce::Justification::centredLeft, 1, 0.85f);
    g.setColour (accent);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawFittedText (e.isUser() ? ui::word (french, "MON PRESET", "USER PRESET") :
                          juce::String ("A: ") + ui::materialLabel (e.materialA, french) +
                          (e.materialB >= 0 ? "  /  B: " + ui::materialLabel (e.materialB, french) : ""),
                      { width - rightBadge - 11, 9, rightBadge, 30 },
                      juce::Justification::centredRight, 1, 0.75f);
}
void OrganixPresetBrowser::selectedRowsChanged (int)
{
    updateSelection();
}
void OrganixPresetBrowser::listBoxItemDoubleClicked (int, const juce::MouseEvent&)
{
    applySelection();
}
void OrganixPresetBrowser::updateSelection()
{
    const int selected = list.getSelectedRow();
    const bool valid = selected >= 0 && selected < static_cast<int> (results.size());
    loadButton.setEnabled (valid);
    favoriteButton.setEnabled (valid);
    exportButton.setEnabled (valid && results[static_cast<size_t> (selected)].isUser());
    if (valid)
        favoriteButton.setButtonText (isFavorite (results[static_cast<size_t> (selected)])
                        ? ui::word (french, "FAV Retirer favori", "FAV Unfavorite")
                        : ui::word (french, "+ Ajouter favori", "+ Add favorite"));
    countLabel.setText (juce::String (results.size()) + " / " + juce::String (library.size()) +
                        ui::word (french, " presets", " presets"), juce::dontSendNotification);
    favoriteOnlyButton.setColour (juce::TextButton::textColourOffId,
                                  favoritesOnly ? ui::accentFor (5) : ui::secondary);
}
void OrganixPresetBrowser::setMessage (const juce::String& message)
{
    messageLabel.setText (message, juce::dontSendNotification);
}
void OrganixPresetBrowser::applySelection()
{
    const int selected = list.getSelectedRow();
    if (selected < 0 || selected >= static_cast<int> (results.size())) return;
    const auto entry = results[static_cast<size_t> (selected)];
    if (entry.isUser())
    {
        juce::MemoryBlock buffer;
        if (! entry.path.existsAsFile() || ! entry.path.loadFileAsData (buffer) || buffer.isEmpty())
        {
            setMessage (ui::word (french, "Lecture impossible : fichier de preset absent ou vide.",
                                          "Cannot read preset: missing or empty file."));
            return;
        }
        audio.setStateInformation (buffer.getData(), static_cast<int> (buffer.getSize()));
        audio.parameters.state.setProperty ("customPresetName", entry.name, nullptr);
    }
    else audio.loadFactoryPreset (entry.factoryIndex);
    lab.syncExternalChange();
    if (onApplied) onApplied (entry.name);
}
void OrganixPresetBrowser::toggleFavorite()
{
    const int selected = list.getSelectedRow();
    if (selected < 0 || selected >= static_cast<int> (results.size())) return;
    const auto key = results[static_cast<size_t> (selected)].key;
    if (favoriteKeys.contains (key)) favoriteKeys.removeString (key);
    else favoriteKeys.addIfNotAlreadyThere (key);
    persistFavorites();
    rebuildResults();
}
void OrganixPresetBrowser::saveCurrent()
{
    const auto raw = saveName.getText().trim();
    const auto legal = juce::File::createLegalFileName (raw);
    if (legal.isEmpty() || raw.isEmpty())
    {
        setMessage (ui::word (french, "Indique un nom avant d'enregistrer.",
                                      "Enter a name before saving."));
        return;
    }
    auto folder = ui::userPresetFolder();
    if (folder.createDirectory().failed())
    {
        setMessage (ui::word (french, "Impossible de créer le dossier de presets.",
                                      "Cannot create the preset folder."));
        return;
    }
    audio.parameters.state.setProperty ("customPresetName", legal, nullptr);
    juce::MemoryBlock buffer;
    audio.getStateInformation (buffer);
    if (buffer.isEmpty())
    {
        setMessage (ui::word (french, "L'état du plugin est vide.",
                                      "The plugin state is empty."));
        return;
    }
    const auto destination = folder.getNonexistentChildFile (legal, ".organix");
    if (! destination.replaceWithData (buffer.getData(), buffer.getSize()))
    {
        setMessage (ui::word (french, "Échec de la sauvegarde sur disque.",
                                      "Could not write the preset file."));
        return;
    }
    saveName.clear();
    refreshLibrary();
    setMessage (ui::word (french, "Preset enregistré : ", "Saved preset: ") + destination.getFileNameWithoutExtension());
}
void OrganixPresetBrowser::importPreset()
{
    chooser = std::make_unique<juce::FileChooser> (
        ui::word (french, "Importer un preset Organix", "Import an Organix preset"),
        juce::File(), "*.organix");
    auto safe = juce::Component::SafePointer<OrganixPresetBrowser> (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode |
                          juce::FileBrowserComponent::canSelectFiles,
        [safe] (const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            const auto source = fc.getResult();
            if (! source.existsAsFile()) return;
            juce::MemoryBlock state;
            if (! source.loadFileAsData (state) || state.isEmpty())
            {
                safe->setMessage (ui::word (safe->french, "Fichier invalide.", "Invalid file."));
                return;
            }
            auto folder = ui::userPresetFolder();
            if (folder.createDirectory().failed()) return;
            const auto dest = folder.getNonexistentChildFile (
                juce::File::createLegalFileName (source.getFileNameWithoutExtension()), ".organix");
            if (source.copyFileTo (dest))
            {
                safe->refreshLibrary();
                safe->setMessage (ui::word (safe->french, "Preset importé.", "Preset imported."));
            }
            else safe->setMessage (ui::word (safe->french, "Import impossible.", "Import failed."));
        });
}
void OrganixPresetBrowser::exportSelected()
{
    const int selected = list.getSelectedRow();
    if (selected < 0 || selected >= static_cast<int> (results.size())) return;
    const auto source = results[static_cast<size_t> (selected)].path;
    if (! source.existsAsFile()) return;
    chooser = std::make_unique<juce::FileChooser> (
        ui::word (french, "Exporter le preset Organix", "Export Organix preset"),
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
            .getChildFile (source.getFileName()), "*.organix");
    auto safe = juce::Component::SafePointer<OrganixPresetBrowser> (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode |
                          juce::FileBrowserComponent::canSelectFiles |
                          juce::FileBrowserComponent::warnAboutOverwriting,
        [safe, source] (const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            auto destination = fc.getResult();
            if (destination == juce::File()) return;
            if (! destination.hasFileExtension (".organix"))
                destination = destination.withFileExtension (".organix");
            if (source.copyFileTo (destination))
                safe->setMessage (ui::word (safe->french, "Preset exporté.", "Preset exported."));
            else safe->setMessage (ui::word (safe->french, "Export impossible.", "Export failed."));
        });
}
void OrganixPresetBrowser::setLabOpen(bool open)
{
    labOpen=open;
    lab.setVisible(open);
    labButton.setVisible(!open);
    for(auto* component:{
        static_cast<juce::Component*>(&search),
        static_cast<juce::Component*>(&saveName),
        static_cast<juce::Component*>(&category),
        static_cast<juce::Component*>(&material),
        static_cast<juce::Component*>(&origin),
        static_cast<juce::Component*>(&character),
        static_cast<juce::Component*>(&favoriteOnlyButton),
        static_cast<juce::Component*>(&favoriteButton),
        static_cast<juce::Component*>(&loadButton),
        static_cast<juce::Component*>(&saveButton),
        static_cast<juce::Component*>(&importButton),
        static_cast<juce::Component*>(&exportButton),
        static_cast<juce::Component*>(&closeButton),
        static_cast<juce::Component*>(&list),
        static_cast<juce::Component*>(&countLabel),
        static_cast<juce::Component*>(&messageLabel)
    }) component->setVisible(!open);
    if(open)
    {
        lab.prepareOpen();
        lab.toFront(false);
    }
    resized();
    repaint();
}
void OrganixPresetBrowser::paint (juce::Graphics& g)
{
    g.fillAll (ui::background.withAlpha (0.97f));
    g.setColour (ui::surface);
    g.fillRoundedRectangle (panelBounds.toFloat(), 12.0f);
    g.setColour (ui::border);
    g.drawRoundedRectangle (panelBounds.toFloat().reduced (0.5f), 12.0f, 1.0f);
    g.setColour (ui::accentFor (0));
    g.fillRoundedRectangle (static_cast<float> (panelBounds.getX() + 18),
                            static_cast<float> (panelBounds.getY() + 19), 4.0f, 40.0f, 2.0f);
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (23.0f, juce::Font::bold)));
    g.drawFittedText (ui::word (french, "BIBLIOTHÈQUE DE PRESETS", "PRESET LIBRARY"),
                      { panelBounds.getX() + 33, panelBounds.getY() + 15, 650, 33 },
                      juce::Justification::centredLeft, 1);
    g.setColour (ui::secondary);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawFittedText (ui::word (french, "ORGANIX  /  RECHERCHE  /  COLLECTIONS  /  FAVORIS  /  PRESETS PERSONNELS",
                                         "ORGANIX  /  SEARCH  /  COLLECTIONS  /  FAVORITES  /  USER PRESETS"),
                      { panelBounds.getX() + 34, panelBounds.getY() + 48, 740, 18 },
                      juce::Justification::centredLeft, 1);
    g.setColour (ui::line);
    const int separator = saveName.getY() - 13;
    g.drawLine (static_cast<float> (panelBounds.getX() + 20),
                static_cast<float> (separator),
                static_cast<float> (panelBounds.getRight() - 20),
                static_cast<float> (separator), 1.0f);
}
void OrganixPresetBrowser::resized()
{
    panelBounds = getLocalBounds().withSizeKeepingCentre (
        juce::jmin (1040, juce::jmax (760, getWidth() - 84)),
        juce::jmin (710, juce::jmax (550, getHeight() - 76)));
    auto p = panelBounds.reduced (23, 16);
    const int x = p.getX(), w = p.getWidth(), y = p.getY();
    closeButton.setBounds (p.getRight() - 118, y + 5, 118, 30);
    labButton.setBounds(p.getRight()-250,y+5,119,30);
    lab.setBounds(panelBounds.reduced(18,16));

    search.setBounds (x + 2, y + 76, juce::jmax (320, w - 230), 35);
    favoriteOnlyButton.setBounds (search.getRight() + 11, y + 76,
                                  p.getRight() - search.getRight() - 11, 35);

    const int gap = 9;
    const int colW=(w-3*gap)/4;
    category.setBounds(x+2,y+122,colW-2,35);
    material.setBounds(x+colW+gap,y+122,colW-2,35);
    origin.setBounds(x+2*(colW+gap),y+122,colW-2,35);
    character.setBounds(x+3*(colW+gap),y+122,
                        w-3*(colW+gap)-2,35);
    countLabel.setBounds (x + 3, y + 163, w - 6, 23);

    const int bottomY = panelBounds.getBottom() - 102;
    list.setBounds (x, y + 192, w, juce::jmax (150, bottomY - (y + 192) - 17));

    const int footerY = bottomY + 4;
    const int compact = juce::jmax (94, (w - 48) / 6);
    favoriteButton.setBounds (x, footerY, compact + 5, 31);
    loadButton.setBounds (x + compact + 12, footerY, compact, 31);
    importButton.setBounds (x + 2 * compact + 20, footerY, compact, 31);
    exportButton.setBounds (x + 3 * compact + 28, footerY, compact, 31);
    saveName.setBounds (x, footerY + 39, juce::jmax (250, w - compact - 12), 32);
    saveButton.setBounds (saveName.getRight() + 10, footerY + 39,
                          p.getRight() - saveName.getRight() - 10, 32);
    messageLabel.setBounds (x + 4, footerY + 74, w - 8, 18);
}
