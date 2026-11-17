#include "OrganixModMatrixView.h"
#include <algorithm>
#include <cmath>

namespace ui=organix::ui;
namespace
{
juce::String utf8 (const char* p) { return juce::String::fromUTF8(p); }
}

void OrganixModMatrixView::styleLabel (juce::Label& l)
{
    l.setFont(juce::Font(juce::FontOptions(10.0f)));
    l.setColour(juce::Label::textColourId,ui::secondary);
    l.setJustificationType(juce::Justification::centredLeft);
}
void OrganixModMatrixView::styleCombo (juce::ComboBox& c)
{
    c.setColour(juce::ComboBox::backgroundColourId,ui::inset);
    c.setColour(juce::ComboBox::textColourId,ui::text);
    c.setColour(juce::ComboBox::outlineColourId,ui::border);
}
void OrganixModMatrixView::styleSlider (juce::Slider& c)
{
    c.setSliderStyle(juce::Slider::LinearHorizontal);
    c.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
    c.setColour(juce::Slider::trackColourId,ui::accentFor(0));
    c.setColour(juce::Slider::thumbColourId,ui::accentFor(0));
    c.setColour(juce::Slider::textBoxTextColourId,ui::text);
    c.setColour(juce::Slider::textBoxBackgroundColourId,ui::inset);
    c.setColour(juce::Slider::textBoxOutlineColourId,ui::border);
}
void OrganixModMatrixView::drawText(juce::Graphics& g,const juce::String& t,
                                   juce::Rectangle<int> r,float size,
                                   juce::Colour colour,bool strong,
                                   juce::Justification alignment)
{
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(size,strong?juce::Font::bold:juce::Font::plain)));
    g.drawFittedText(t,r,alignment,1,0.86f);
}
juce::String OrganixModMatrixView::slotStem(int index)
{
    return "mm"+juce::String(index+1).paddedLeft('0',2);
}
float OrganixModMatrixView::read(const juce::String& id) const noexcept
{
    if(auto* v=audio.parameters.getRawParameterValue(id))
        return v->load(std::memory_order_relaxed);
    return 0.0f;
}
void OrganixModMatrixView::changeToggle(const juce::String& id,bool value)
{
    if(auto* p=audio.parameters.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(value?1.0f:0.0f));
        p->endChangeGesture();
    }
}
OrganixModMatrixView::OrganixModMatrixView (OrganixAudioProcessor& p):audio(p)
{
    setOpaque(true);
    for(auto* b:{&masterToggle,&tempoToggle,&slotToggle,&demoButton,&closeButton})
        addAndMakeVisible(*b);
    for(auto* c:{&source,&target,&polarity,&division})
    {
        styleCombo(*c);
        addAndMakeVisible(*c);
    }
    for(auto* l:{&sourceLabel,&targetLabel,&polarityLabel,&depthLabel,
                 &masterLabel,&envelopeLabel})
    {
        styleLabel(*l);
        addAndMakeVisible(*l);
    }
    for(auto* slider:{&slotDepth,&masterDepth,&randomRate})
    {
        styleSlider(*slider);
        addAndMakeVisible(*slider);
    }
    for(auto& slider:lfoRates)
    {
        styleSlider(slider);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,52,24);
        addAndMakeVisible(slider);
    }
    for(auto& slider:envelopeSliders)
    {
        styleSlider(slider);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,24);
        addAndMakeVisible(slider);
    }
    masterToggle.onClick=[this]
    {
        changeToggle("matrixEnable",read("matrixEnable")<0.5f);
        refreshState();
    };
    tempoToggle.onClick=[this]
    {
        changeToggle("matrixTempoSync",read("matrixTempoSync")<0.5f);
        refreshState();
    };
    slotToggle.onClick=[this]
    {
        const auto name=slotStem(selected)+"Enable";
        changeToggle(name,read(name)<0.5f);
        refreshState();
    };
    closeButton.onClick=[this] { if(onClose) onClose(); };
    demoButton.onClick=[this] { applyDemo(); };
    depthLink=nullptr;
    masterToggle.setComponentID ("midi:matrixEnable");
    tempoToggle.setComponentID ("midi:matrixTempoSync");
    masterDepth.setComponentID ("midi:matrixMasterDepth");
    randomRate.setComponentID ("midi:matrixRandomRate");
    division.setComponentID ("midi:matrixBeatDivision");
    masterLink=std::make_unique<SliderAttachment>(
        audio.parameters,"matrixMasterDepth",masterDepth);
    randomLink=std::make_unique<SliderAttachment>(
        audio.parameters,"matrixRandomRate",randomRate);
    divisionLink=std::make_unique<ComboAttachment>(
        audio.parameters,"matrixBeatDivision",division);
    for(int i=0;i<4;++i)
    {
        const auto name=juce::String("matrixLfo")+juce::String(i+1)+"Rate";
        lfoRates[static_cast<size_t>(i)].setComponentID ("midi:" + name);
        lfoLinks[static_cast<size_t>(i)]=std::make_unique<SliderAttachment>(
            audio.parameters,name,lfoRates[static_cast<size_t>(i)]);
    }
    for(int i=0;i<2;++i)
    {
        const auto stem=juce::String("matrixEnv")+juce::String(i+1);
        envelopeSliders[static_cast<size_t>(i*2)].setComponentID ("midi:" + stem + "Attack");
        envelopeSliders[static_cast<size_t>(i*2+1)].setComponentID ("midi:" + stem + "Decay");
        envLinks[static_cast<size_t>(i*2)]=std::make_unique<SliderAttachment>(
            audio.parameters,stem+"Attack",envelopeSliders[static_cast<size_t>(i*2)]);
        envLinks[static_cast<size_t>(i*2+1)]=std::make_unique<SliderAttachment>(
            audio.parameters,stem+"Decay",envelopeSliders[static_cast<size_t>(i*2+1)]);
    }
    slotDepth.textFromValueFunction=[](double v)
    {
        return juce::String(v*100.0,0)+"%";
    };
    slotDepth.valueFromTextFunction=[](const juce::String& v)
    {
        return v.getDoubleValue()/100.0;
    };
    masterDepth.textFromValueFunction=[](double v)
    {
        return juce::String(v*100.0,0)+"%";
    };
    masterDepth.valueFromTextFunction=[](const juce::String& v)
    {
        return v.getDoubleValue()/100.0;
    };
    setFrench(true);
    bindSlot(0);
    startTimerHz(20);
}
OrganixModMatrixView::~OrganixModMatrixView()
{
    stopTimer();
    depthLink.reset();
    sourceLink.reset();targetLink.reset();polarityLink.reset();
    divisionLink.reset();masterLink.reset();randomLink.reset();
    for(auto& x:lfoLinks)x.reset();
    for(auto& x:envLinks)x.reset();
}
void OrganixModMatrixView::bindSlot(int index)
{
    selected=juce::jlimit(0,organix::MatrixSettings::slotCount-1,index);
    depthLink.reset();
    sourceLink.reset();
    targetLink.reset();
    polarityLink.reset();
    const auto stem=slotStem(selected);
    slotToggle.setComponentID ("midi:" + stem + "Enable");
    source.setComponentID ("midi:" + stem + "Source");
    target.setComponentID ("midi:" + stem + "Target");
    polarity.setComponentID ("midi:" + stem + "Polarity");
    slotDepth.setComponentID ("midi:" + stem + "Depth");
    sourceLink=std::make_unique<ComboAttachment>(audio.parameters,stem+"Source",source);
    targetLink=std::make_unique<ComboAttachment>(audio.parameters,stem+"Target",target);
    polarityLink=std::make_unique<ComboAttachment>(audio.parameters,stem+"Polarity",polarity);
    depthLink=std::make_unique<SliderAttachment>(audio.parameters,stem+"Depth",slotDepth);
    refreshState();
    repaint();
}
void OrganixModMatrixView::setFrench(bool value)
{
    french=value;
    const auto fill=[&](juce::ComboBox& c,int count,auto getName)
    {
        const int chosen=c.getSelectedId();
        c.clear(juce::dontSendNotification);
        for(int i=0;i<count;++i)
            c.addItem(utf8(getName(i,french)),i+1);
        c.setSelectedId(chosen>0?chosen:1,juce::dontSendNotification);
    };
    fill(source,organix::MatrixSettings::sourceCount,
         organix::ModulationMatrix::sourceName);
    fill(target,organix::MatrixSettings::destinationCount,
         organix::ModulationMatrix::destinationName);
    const int polarityId=polarity.getSelectedId();
    polarity.clear(juce::dontSendNotification);
    polarity.addItem(ui::word(french,"UNIPOLAIRE","UNIPOLAR"),1);
    polarity.addItem(ui::word(french,"BIPOLAIRE","BIPOLAR"),2);
    polarity.setSelectedId(polarityId>0?polarityId:1,juce::dontSendNotification);
    const int divisionId=division.getSelectedId();
    division.clear(juce::dontSendNotification);
    constexpr const char* divisions[]={"1/1","1/2","1/4","1/8","1/16","1/8T","1/16T"};
    for(int i=0;i<7;++i)division.addItem(divisions[i],i+1);
    division.setSelectedId(divisionId>0?divisionId:3,juce::dontSendNotification);
    sourceLabel.setText(ui::word(french,"SOURCE DE MODULATION","MODULATION SOURCE"),
                        juce::dontSendNotification);
    targetLabel.setText(ui::word(french,"DESTINATION DSP","DSP DESTINATION"),
                        juce::dontSendNotification);
    polarityLabel.setText(ui::word(french,"POLARITÉ","POLARITY"),
                          juce::dontSendNotification);
    depthLabel.setText(ui::word(french,"PROFONDEUR SIGNÉE","SIGNED DEPTH"),
                       juce::dontSendNotification);
    masterLabel.setText(ui::word(french,"INTENSITÉ GLOBALE","MASTER DEPTH"),
                        juce::dontSendNotification);
    envelopeLabel.setText(ui::word(french,"ENVELOPPES DÉCLENCHÉES PAR L'AUDIO",
                                         "AUDIO-TRIGGERED ENVELOPES"),
                           juce::dontSendNotification);
    demoButton.setButtonText(ui::word(french,"EXEMPLE SONORE","AUDIO DEMO"));
    closeButton.setButtonText(ui::word(french,"RETOUR","BACK"));
    refreshState();
    repaint();
}
void OrganixModMatrixView::refreshState()
{
    const bool on=read("matrixEnable")>0.5f;
    const bool sync=read("matrixTempoSync")>0.5f;
    const bool slotOn=read(slotStem(selected)+"Enable")>0.5f;
    masterToggle.setButtonText(on?ui::word(french,"MATRICE ACTIVE","MATRIX ON")
                                 :ui::word(french,"ACTIVER MATRICE","ENABLE MATRIX"));
    tempoToggle.setButtonText(sync?"SYNC":ui::word(french,"LIBRE","FREE"));
    slotToggle.setButtonText(slotOn?ui::word(french,"LIAISON ACTIVE","ROUTE ON")
                                    :ui::word(french,"ACTIVER LIAISON","ENABLE ROUTE"));
    masterToggle.setColour(juce::TextButton::buttonColourId,
                           on?juce::Colour(0xff17443e):ui::inset);
    slotToggle.setColour(juce::TextButton::buttonColourId,
                         slotOn?juce::Colour(0xff17443e):ui::inset);
    division.setEnabled(sync);
    for(auto& dial:lfoRates)dial.updateText();
}
void OrganixModMatrixView::applyDemo()
{
    struct Route {int source,target;float depth;int polarity;};
    // A musical starting point, deliberately invoked by the user's click.
    constexpr Route defaults[] = {
        {1,2,0.55f,1}, // LFO1 -> A Motion (bipolar)
        {7,24,0.62f,0}, // Input level -> B Energy
        {8,15,0.37f,0}, // Transient -> A Shape 09
        {12,37,0.32f,1}, // Random glide -> Fusion
        {9,23,0.44f,0}, // Spectral air -> B Density
        {15,3,0.29f,1} // Macro Energy -> A Tone
    };
    for(int i=0;i<6;++i)
    {
        const auto stem=slotStem(i);
        const auto set=[this](const juce::String& id,float value)
        {
            if(auto* p=audio.parameters.getParameter(id))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(p->convertTo0to1(value));
                p->endChangeGesture();
            }
        };
        set(stem+"Source",static_cast<float>(defaults[i].source));
        set(stem+"Target",static_cast<float>(defaults[i].target));
        set(stem+"Depth",defaults[i].depth);
        set(stem+"Polarity",static_cast<float>(defaults[i].polarity));
        set(stem+"Enable",1.0f);
    }
    changeToggle("matrixEnable",true);
    bindSlot(0);
}
void OrganixModMatrixView::timerCallback()
{
    if(!isShowing())return;
    refreshState();
    repaint();
}
void OrganixModMatrixView::paint(juce::Graphics& g)
{
    g.fillAll(ui::background);
    const auto full=getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(ui::panel);
    g.fillRoundedRectangle(full,8.0f);
    g.setColour(ui::border);
    g.drawRoundedRectangle(full,8.0f,1.0f);
    const int sideX=getWidth()-316;
    g.setColour(ui::inset);
    g.fillRoundedRectangle(static_cast<float>(sideX),95.0f,306.0f,
                           static_cast<float>(getHeight()-106),7.0f);
    drawText(g,"MODULATION MATRIX PRO",
             {18,13,sideX-190,29},19.0f,ui::text,true);
    drawText(g,ui::word(french,
             "32 LIAISONS / 20 SOURCES / 40 DESTINATIONS DSP",
             "32 ROUTES / 20 SOURCES / 40 DSP TARGETS"),
             {19,45,sideX-230,18},10.0f,ui::secondary,true);
    for(int i=0;i<5;++i)
    {
        const int x=18+i*(sideX-38)/5;
        const int w=(sideX-38)/5-6;
        drawText(g,i<4?"LFO "+juce::String(i+1):
                 ui::word(french,"ALÉATOIRE","RANDOM"),
                 {x,88,w,17},9.5f,ui::secondary,true);
    }
    drawText(g,ui::word(french,"AFFECTATIONS ACTIVES / ACTIVITÉ AUDIO",
                                    "ACTIVE ROUTES / AUDIO ACTIVITY"),
             {19,151,sideX-40,19},10.3f,ui::secondary,true);
    const int rowY=184,rowH=25;
    const int colW=(sideX-35)/2;
    for(int k=0;k<organix::MatrixSettings::slotCount;++k)
    {
        const int x=17+(k/16)*colW, y=rowY+(k%16)*rowH;
        const int width=colW-7;
        const auto stem=slotStem(k);
        const bool enabled=read(stem+"Enable")>0.5f;
        const int src=juce::jlimit(0,organix::MatrixSettings::sourceCount-1,
                                  static_cast<int>(read(stem+"Source")));
        const int dst=juce::jlimit(0,organix::MatrixSettings::destinationCount-1,
                                  static_cast<int>(read(stem+"Target")));
        const float depth=read(stem+"Depth");
        const float mod=audio.matrixSlotActivity[static_cast<size_t>(k)].load(
            std::memory_order_relaxed);
        const auto rect=juce::Rectangle<float>(static_cast<float>(x),
                                static_cast<float>(y),
                                static_cast<float>(width),23.0f);
        g.setColour(k==selected?juce::Colour(0xff1d4250):ui::surface);
        g.fillRoundedRectangle(rect,3.0f);
        if(k==selected)
        {
            g.setColour(ui::accentFor(0));
            g.drawRoundedRectangle(rect,3.0f,1.0f);
        }
        const auto colour=enabled?ui::accentFor(k%2==0?0:3):ui::subdued;
        drawText(g,juce::String(k+1).paddedLeft('0',2),
                 {x+5,y+2,21,18},10.0f,colour,true);
        drawText(g,utf8(organix::ModulationMatrix::sourceName(src,french)),
                 {x+29,y+2,92,18},9.8f,enabled?ui::text:ui::subdued);
        drawText(g,">",{x+123,y+2,14,18},11.0f,ui::secondary,true);
        drawText(g,utf8(organix::ModulationMatrix::destinationName(dst,french)),
                 {x+138,y+2,width-206,18},9.8f,
                 enabled?ui::text:ui::subdued);
        drawText(g,juce::String(juce::roundToInt(depth*100.0f))+"%",
                 {x+width-69,y+1,35,20},9.7f,colour,true,
                 juce::Justification::centredRight);
        g.setColour(ui::line);
        g.fillRoundedRectangle(static_cast<float>(x+width-30),
                               static_cast<float>(y+8),24.0f,6.0f,2.0f);
        if(enabled)
        {
            g.setColour(mod<0?ui::accentFor(4):ui::accentFor(0));
            const float absMod=juce::jlimit(0.0f,1.0f,std::abs(mod));
            g.fillRoundedRectangle(static_cast<float>(x+width-30),
                static_cast<float>(y+8),24.0f*absMod,6.0f,2.0f);
        }
    }
    drawText(g,ui::word(french,
            "Cliquez sur une liaison pour éditer source, destination, profondeur et polarité.",
            "Click a route to edit source, destination, depth and polarity."),
            {18,getHeight()-37,sideX-38,23},9.6f,ui::subdued);
    drawText(g,ui::word(french,"ÉDITION DE LA LIAISON","EDIT SELECTED ROUTE"),
             {sideX+14,113,sideX>0?277:250,22},13.0f,ui::text,true);
    drawText(g,juce::String(selected+1).paddedLeft('0',2)+" / 32",
             {sideX+218,113,72,22},12.0f,ui::accentFor(0),true,
             juce::Justification::centredRight);
    drawText(g,ui::word(french,
            "Source / destination réellement connectées au DSP",
            "Source and destination connected to actual DSP"),
            {sideX+16,136,275,15},9.0f,ui::subdued);
}
void OrganixModMatrixView::mouseDown(const juce::MouseEvent& e)
{
    const int sideX=getWidth()-316;
    const int rowY=184,rowH=25,colW=(sideX-35)/2;
    const auto p=e.position;
    if(p.y<rowY||p.y>=rowY+16*rowH||p.x<17||p.x>=sideX-15)
        return;
    const int column=static_cast<int>((p.x-17)/static_cast<float>(colW));
    if(column<0||column>1)return;
    const int row=static_cast<int>((p.y-rowY)/rowH);
    bindSlot(column*16+row);
}
void OrganixModMatrixView::resized()
{
    const int sideX=getWidth()-316;
    const int groupW=(sideX-38)/5;
    masterToggle.setBounds(sideX+16,12,155,28);
    closeButton.setBounds(getWidth()-113,12,96,28);
    demoButton.setBounds(sideX-155,12,134,28);
    masterLabel.setBounds(20,67,140,18);
    masterDepth.setBounds(151,67,182,27);
    tempoToggle.setBounds(sideX-281,64,77,27);
    division.setBounds(sideX-196,64,130,27);
    for(int i=0;i<4;++i)
        lfoRates[static_cast<size_t>(i)].setBounds(
            18+i*groupW,109,groupW-7,28);
    randomRate.setBounds(18+4*groupW,109,groupW-7,28);

    const int x=sideX+17,w=273;
    sourceLabel.setBounds(x,161,w,16);
    source.setBounds(x,180,w,30);
    targetLabel.setBounds(x,216,w,16);
    target.setBounds(x,235,w,30);
    polarityLabel.setBounds(x,273,w,16);
    polarity.setBounds(x,292,w,30);
    depthLabel.setBounds(x,329,w,18);
    slotDepth.setBounds(x,349,w,34);
    slotToggle.setBounds(x,393,w,32);
    envelopeLabel.setBounds(x,444,w,19);
    for(int i=0;i<4;++i)
        envelopeSliders[static_cast<size_t>(i)].setBounds(x,474+33*i,w,29);
}
