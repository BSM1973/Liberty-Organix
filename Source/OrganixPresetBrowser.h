#pragma once
#include "PluginProcessor.h"
#include "OrganixI18n.h"
#include <functional>
#include <memory>
#include <vector>

// Embedded, real preset browser (not a decorative mock-up).
// Loads factory presets, scans user .organix files, supports search, three filters,
// persistent favorites, saving/loading/import/export and keyboard selection.
class OrganixPresetBrowser final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit OrganixPresetBrowser (OrganixAudioProcessor&, juce::PropertiesFile&);
    ~OrganixPresetBrowser() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setFrench (bool languageIsFrench);
    void refreshLibrary();
    std::function<void (const juce::String&)> onApplied;
    std::function<void()> onDismissed;
private:
    struct Entry
    {
        juce::String name, description, key;
        juce::File path;
        int factoryIndex = -1, category = 0, materialA = -1, materialB = -1;
        bool isUser() const noexcept { return factoryIndex < 0; }
    };
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastSelectedRow) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void rebuildResults (bool preserveSelection = true);
    void updateSelection();
    void applySelection();
    void toggleFavorite();
    void saveCurrent();
    void importPreset();
    void exportSelected();
    bool isFavorite (const Entry&) const;
    void persistFavorites();
    void setMessage (const juce::String& text);

    OrganixAudioProcessor& audio;
    juce::PropertiesFile& settings;
    bool french = true, favoritesOnly = false;
    std::vector<Entry> library, results;
    juce::StringArray favoriteKeys;
    juce::Rectangle<int> panelBounds;
    juce::TextEditor search, saveName;
    juce::ComboBox category, material, origin;
    juce::TextButton favoriteOnlyButton, favoriteButton, loadButton, saveButton,
                     importButton, exportButton, closeButton;
    juce::ListBox list;
    juce::Label countLabel, messageLabel;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrganixPresetBrowser)
};
