#include "OrganixPresetLabView.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ui=organix::ui;
namespace
{
void style(juce::TextButton& b)
{
    b.setColour(juce::TextButton::buttonColourId,ui::inset);
    b.setColour(juce::TextButton::textColourOffId,ui::text);
}
void heading(juce::Graphics& g,const juce::String& text,
             juce::Rectangle<int> region,float points,juce::Colour colour)
{
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(points,juce::Font::bold)));
    g.drawFittedText(text,region,juce::Justification::centredLeft,1,0.84f);
}
}
bool OrganixPresetLabView::same(const Snapshot& a,const Snapshot& b) noexcept
{
    return a.getSize()==b.getSize() &&
           (a.getSize()==0 ||
            std::memcmp(a.getData(),b.getData(),a.getSize())==0);
}
OrganixPresetLabView::Snapshot OrganixPresetLabView::capture() const
{
    Snapshot memory;
    audio.getStateInformation(memory);
    return memory;
}
void OrganixPresetLabView::restore(const Snapshot& state)
{
    if(state.isEmpty())return;
    restoring=true;
    audio.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    tracked=state;
    pending=false;
    restoring=false;
    refreshLabels();
}
void OrganixPresetLabView::pushUndo(const Snapshot& previous)
{
    if(previous.isEmpty())return;
    if(undo.size()>=48)undo.erase(undo.begin());
    undo.push_back(previous);
    redo.clear();
}
void OrganixPresetLabView::flushPending()
{
    if(restoring)return;
    auto actual=capture();
    if(!same(actual,tracked))
    {
        pushUndo(tracked);
        tracked=std::move(actual);
    }
    pending=false;
    refreshLabels();
}
void OrganixPresetLabView::syncExternalChange()
{
    flushPending();
}
void OrganixPresetLabView::prepareOpen()
{
    flushPending();
    if(snapshotA.isEmpty())snapshotA=tracked;
    refreshLabels();
}
OrganixPresetLabView::OrganixPresetLabView(OrganixAudioProcessor& p):audio(p)
{
    setOpaque(true);
    for(auto* button:{&closeButton,&abButton,&storeA,&storeB,&undoButton,
                     &redoButton,&mutateButton})
    {
        style(*button);
        addAndMakeVisible(*button);
    }
    for(auto& b:lockButtons){style(b);addAndMakeVisible(b);}
    for(auto& b:storeVariation){style(b);addAndMakeVisible(b);}
    for(auto& b:recallVariation){style(b);addAndMakeVisible(b);}
    intensity.setRange(0.05,1.0,0.01);
    intensity.setValue(0.38,juce::dontSendNotification);
    intensity.setSliderStyle(juce::Slider::LinearHorizontal);
    intensity.setTextBoxStyle(juce::Slider::TextBoxRight,false,57,26);
    intensity.setColour(juce::Slider::trackColourId,ui::accentFor(0));
    intensity.setColour(juce::Slider::thumbColourId,ui::accentFor(0));
    intensity.setColour(juce::Slider::textBoxTextColourId,ui::text);
    intensity.setColour(juce::Slider::textBoxBackgroundColourId,ui::inset);
    intensity.setColour(juce::Slider::textBoxOutlineColourId,ui::border);
    intensity.textFromValueFunction=[](double value)
    {return juce::String(juce::roundToInt(value*100.0))+"%";};
    intensity.valueFromTextFunction=[](const juce::String& s)
    {return s.getDoubleValue()/100.0;};
    addAndMakeVisible(intensity);
    status.setFont(juce::Font(juce::FontOptions(11.0f)));
    status.setColour(juce::Label::textColourId,ui::secondary);
    status.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(status);

    closeButton.onClick=[this]{if(onClose)onClose();};
    abButton.onClick=[this]{toggleAB();};
    storeA.onClick=[this]{captureAB(0);};
    storeB.onClick=[this]{captureAB(1);};
    undoButton.onClick=[this]{performUndo();};
    redoButton.onClick=[this]{performRedo();};
    mutateButton.onClick=[this]{generateVariation();};
    for(int i=0;i<5;++i)
        lockButtons[static_cast<size_t>(i)].onClick=[this,i]
        {
            locks^=(1u<<i);
            refreshLabels();
        };
    for(int i=0;i<4;++i)
    {
        storeVariation[static_cast<size_t>(i)].onClick=[this,i]
        {captureVariation(i);};
        recallVariation[static_cast<size_t>(i)].onClick=[this,i]
        {recallVariation(i);};
    }
    tracked=capture();
    snapshotA=tracked;
    setFrench(true);
    startTimerHz(6);
}
OrganixPresetLabView::~OrganixPresetLabView(){stopTimer();}
void OrganixPresetLabView::setFrench(bool value)
{
    french=value;
    refreshLabels();
    repaint();
}
void OrganixPresetLabView::refreshLabels()
{
    closeButton.setButtonText(ui::word(french,"RETOUR","BACK"));
    abButton.setButtonText(activeA?"ÉCOUTER B":"ÉCOUTER A");
    storeA.setButtonText(ui::word(french,"MÉMORISER A","STORE A"));
    storeB.setButtonText(ui::word(french,"MÉMORISER B","STORE B"));
    undoButton.setButtonText(ui::word(french,"ANNULER","UNDO"));
    redoButton.setButtonText(ui::word(french,"RÉTABLIR","REDO"));
    mutateButton.setButtonText(ui::word(french,"CRÉER UNE VARIATION","CREATE VARIATION"));
    constexpr const char* frNames[]={"COUCHE A","COUCHE B","ESPACE",
                                     "MOUVEMENT","MACROS"};
    constexpr const char* enNames[]={"LAYER A","LAYER B","SPACE",
                                     "MOTION","MACROS"};
    for(int i=0;i<5;++i)
    {
        const bool on=(locks&(1u<<i))!=0;
        auto& b=lockButtons[static_cast<size_t>(i)];
        b.setButtonText((on?"[X] ":"[ ] ")+
            juce::String(french?frNames[i]:enNames[i]));
        b.setColour(juce::TextButton::buttonColourId,
                    on?juce::Colour(0xff17453f):ui::inset);
        b.setColour(juce::TextButton::textColourOffId,
                    on?ui::engaged:ui::secondary);
    }
    for(int i=0;i<4;++i)
    {
        storeVariation[static_cast<size_t>(i)].setButtonText(
            ui::word(french,"CAPTURER ","STORE ")+juce::String(i+1));
        recallVariation[static_cast<size_t>(i)].setButtonText(
            ui::word(french,"ÉCOUTER ","RECALL ")+juce::String(i+1));
        recallVariation[static_cast<size_t>(i)].setEnabled(
            !variations[static_cast<size_t>(i)].isEmpty());
    }
    undoButton.setEnabled(!undo.empty());
    redoButton.setEnabled(!redo.empty());
    abButton.setColour(juce::TextButton::buttonColourId,ui::accentFor(activeA?0:4).withAlpha(0.30f));
}
void OrganixPresetLabView::timerCallback()
{
    if(restoring)return;
    auto actual=capture();
    const auto now=juce::Time::getMillisecondCounter();
    if(same(actual,tracked))
    {
        pending=false;
        return;
    }
    if(!pending||!same(actual,pendingState))
    {
        pending=true;
        pendingState=std::move(actual);
        lastChangeTime=now;
    }
    else if(now-lastChangeTime>=350u)
    {
        pushUndo(tracked);
        tracked=pendingState;
        pending=false;
        if(isShowing())refreshLabels();
    }
}
void OrganixPresetLabView::performUndo()
{
    flushPending();
    if(undo.empty())return;
    const Snapshot before=tracked;
    const Snapshot target=undo.back();
    undo.pop_back();
    if(redo.size()>=48)redo.erase(redo.begin());
    redo.push_back(before);
    restore(target);
    status.setText(ui::word(french,"Dernière modification annulée.",
                                "Last change undone."),juce::dontSendNotification);
}
void OrganixPresetLabView::performRedo()
{
    flushPending();
    if(redo.empty())return;
    const Snapshot before=tracked;
    const Snapshot target=redo.back();
    redo.pop_back();
    if(undo.size()>=48)undo.erase(undo.begin());
    undo.push_back(before);
    restore(target);
    status.setText(ui::word(french,"Modification rétablie.",
                                "Change restored."),juce::dontSendNotification);
}
void OrganixPresetLabView::captureAB(int slot)
{
    flushPending();
    if(slot==0)snapshotA=tracked;
    else snapshotB=tracked;
    status.setText(ui::word(french,"Son mémorisé en ","Sound stored in ")+
                   (slot==0?"A":"B"),juce::dontSendNotification);
    refreshLabels();
}
void OrganixPresetLabView::toggleAB()
{
    flushPending();
    const bool nextIsA=!activeA;
    if(activeA)snapshotA=tracked;
    else snapshotB=tracked;
    const auto& target=nextIsA?snapshotA:snapshotB;
    if(target.isEmpty())
    {
        if(nextIsA)snapshotA=tracked;
        else snapshotB=tracked;
    }
    else if(!same(target,tracked))
    {
        pushUndo(tracked);
        restore(target);
    }
    activeA=nextIsA;
    status.setText(ui::word(french,"Comparaison active : ","Comparing: ")+
                   (activeA?"A":"B"),juce::dontSendNotification);
    refreshLabels();
}
void OrganixPresetLabView::captureVariation(int index)
{
    flushPending();
    variations[static_cast<size_t>(index)]=tracked;
    refreshLabels();
    status.setText(ui::word(french,"Variation mémorisée : ","Stored variation: ")+
                   juce::String(index+1),juce::dontSendNotification);
}
void OrganixPresetLabView::recallVariation(int index)
{
    flushPending();
    const auto& target=variations[static_cast<size_t>(index)];
    if(target.isEmpty())return;
    if(!same(target,tracked))pushUndo(tracked);
    restore(target);
    status.setText(ui::word(french,"Variation rappelée : ","Recalled variation: ")+
                   juce::String(index+1),juce::dontSendNotification);
}
void OrganixPresetLabView::generateVariation()
{
    flushPending();
    std::array<float,organix::variationParameters.size()> values {};
    for(size_t k=0;k<values.size();++k)
    {
        const auto* id=organix::variationParameters[k].id;
        if(auto* p=audio.parameters.getRawParameterValue(id))
            values[k]=p->load(std::memory_order_relaxed);
    }
    organix::PresetVariationEngine engine(nextSeed++);
    const int count=engine.mutate(values,
                static_cast<float>(intensity.getValue()),locks);
    if(count==0)
    {
        status.setText(ui::word(french,
            "Aucun réglage modifié : vérifiez les verrous.",
            "No parameters changed: check your locks."),
            juce::dontSendNotification);
        return;
    }
    pushUndo(tracked);
    for(size_t k=0;k<values.size();++k)
    {
        const auto& spec=organix::variationParameters[k];
        if((locks&spec.group)!=0)continue;
        if(auto* p=audio.parameters.getParameter(spec.id))
        {
            const float old=p->convertFrom0to1(p->getValue());
            if(std::abs(old-values[k])<0.001f)continue;
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(values[k]));
            p->endChangeGesture();
        }
    }
    tracked=capture();
    pending=false;
    refreshLabels();
    status.setText(juce::String(count)+ui::word(french,
        " paramètres modifiés ; niveaux d'entrée/sortie préservés.",
        " parameters changed; input/output gain preserved."),
        juce::dontSendNotification);
}
void OrganixPresetLabView::paint(juce::Graphics& g)
{
    g.fillAll(ui::background);
    const auto w=getWidth(),h=getHeight();
    auto frame=getLocalBounds().toFloat().reduced(0.8f);
    g.setColour(ui::panel);g.fillRoundedRectangle(frame,8.0f);
    g.setColour(ui::border);g.drawRoundedRectangle(frame,8.0f,1.0f);
    heading(g,"PRESET LAB",{18,15,w-153,30},21.0f,ui::text);
    heading(g,ui::word(french,
        "COMPARAISON A/B  /  HISTORIQUE  /  VARIATIONS MUSICALES",
        "A/B COMPARE  /  HISTORY  /  MUSICAL VARIATIONS"),
        {18,49,w-40,18},10.0f,ui::secondary);
    g.setColour(ui::border);g.drawLine(17.0f,154.0f,static_cast<float>(w-17),154.0f);
    heading(g,ui::word(french,"INTENSITÉ DES VARIATIONS","VARIATION INTENSITY"),
        {18,165,213,23},11.0f,ui::secondary);
    heading(g,ui::word(french,"VERROUILLAGE DES PARAMÈTRES","PARAMETER LOCKS"),
        {18,222,w-36,20},11.0f,ui::secondary);
    heading(g,ui::word(french,"QUATRE MÉMOIRES DE VARIATIONS","FOUR VARIATION MEMORIES"),
        {18,315,w-36,20},11.0f,ui::secondary);
    const int col=(w-49)/4;
    for(int i=0;i<4;++i)
    {
        const int x=17+i*(col+5);
        g.setColour(ui::inset);
        g.fillRoundedRectangle(static_cast<float>(x),348.0f,
                               static_cast<float>(col),103.0f,5.0f);
        heading(g,"V"+juce::String(i+1),{x+11,354,col-22,19},
                12.0f,ui::accentFor(i));
    }
    heading(g,ui::word(french,
        "MATERIALS, ROUTAGE ET GAINS RESTENT INCHANGÉS. L'ÉCOUTE FINALE SE FAIT DANS LE DAW.",
        "MATERIALS, ROUTING AND GAINS STAY UNCHANGED. FINAL AUDITION IS IN YOUR DAW."),
        {18,h-31,w-36,20},9.5f,ui::subdued);
}
void OrganixPresetLabView::resized()
{
    const int w=getWidth(),h=getHeight();
    closeButton.setBounds(w-115,17,99,29);
    const int gap=7,cell=(w-36-5*gap)/6;
    for(int i=0;i<6;++i)
    {
        auto* b=std::array<juce::TextButton*,6>{
            &abButton,&storeA,&storeB,&undoButton,&redoButton,&mutateButton
        }[static_cast<size_t>(i)];
        b->setBounds(18+i*(cell+gap),93,cell,34);
    }
    intensity.setBounds(224,162,std::min(302,w-255),31);
    const int lockW=(w-36-4*7)/5;
    for(int i=0;i<5;++i)
        lockButtons[static_cast<size_t>(i)].setBounds(
            18+i*(lockW+7),251,lockW,36);
    const int col=(w-49)/4;
    for(int i=0;i<4;++i)
    {
        const int x=17+i*(col+5);
        storeVariation[static_cast<size_t>(i)].setBounds(x+10,375,col-20,28);
        recallVariation[static_cast<size_t>(i)].setBounds(x+10,411,col-20,28);
    }
    status.setBounds(18,std::max(463,h-61),w-36,25);
}
