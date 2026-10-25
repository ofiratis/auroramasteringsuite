#include "PluginEditor.h"
AuroraEditor::AuroraEditor(AuroraProcessor& p) : juce::AudioProcessorEditor(&p), processor(p)
{
    const std::pair<const char*,const char*> items[] = {
        {"input","INPUT"},{"output","OUTPUT"},{"mix","WET"},{"hpf","HPF"},
        {"lowfreq","LOW FREQ"},{"lowgain","LOW GAIN"},{"midfreq","MID FREQ"},
        {"midgain","MID GAIN"},{"midq","MID Q"},{"upperfreq","HIGH MID F"},
        {"uppergain","HIGH MID G"},{"upperq","HIGH MID Q"},
        {"highfreq","HIGH FREQ"},{"highgain","HIGH GAIN"},
        {"threshold","THRESHOLD"},{"ratio","RATIO"},{"attack","ATTACK"},
        {"release","RELEASE"},{"knee","KNEE"},{"makeup","MAKEUP"},
        {"width","WIDTH"},{"drive","DRIVE"},{"satmix","SAT MIX"},
        {"ceiling","CEILING"},{"limdrive","LIMIT IN"},{"bypass","BYPASS"}};
    for (auto [id, name] : items)
    {
        auto c=std::make_unique<Control>();
        c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,75,19);
        c->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff50d6d0));
        c->slider.setColour(juce::Slider::thumbColourId,juce::Colours::white);
        c->label.setText(name,juce::dontSendNotification);
        c->label.setJustificationType(juce::Justification::centred);
        c->label.setColour(juce::Label::textColourId,juce::Colour(0xffc6d9e3));
        addAndMakeVisible(c->slider); addAndMakeVisible(c->label);
        c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,c->slider);
        controls.push_back(std::move(c));
    }
    meters.setJustificationType(juce::Justification::centredLeft);
    meters.setColour(juce::Label::textColourId,juce::Colours::white);
    addAndMakeVisible(meters);
    setSize(1020,770);
    startTimerHz(12);
}
void AuroraEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101a27));
    g.setColour(juce::Colour(0xff50d6d0));
    g.setFont(juce::Font(juce::FontOptions(30.f,juce::Font::bold)));
    g.drawText("AURORA  /  MASTERING SUITE",20,12,getWidth()-40,43,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9aaebd));
    g.setFont(14.f);
    g.drawText("EQ  •  DYNAMICS  •  SATURATION  •  STEREO  •  PEAK LIMITER",22,54,getWidth()-40,25,juce::Justification::centredLeft);
    const char* sections[]={"LEVEL / FILTER","TONE SHAPING","DYNAMICS","STEREO / COLOR / LIMIT"};
    for(int i=0;i<4;++i) {
        const int y=90+i*158;
        g.setColour(juce::Colour(0xff192b3b));
        g.fillRoundedRectangle(12.f,(float)y,(float)getWidth()-24,150.f,8.f);
        g.setColour(juce::Colour(0xff50d6d0));
        g.setFont(12.f);
        g.drawText(sections[i],24,y+4,240,20,juce::Justification::centredLeft);
    }
}
void AuroraEditor::resized()
{
    // 4 sections, with controls grouped by index.
    const int starts[]={0,4,14,20,26};
    for(int row=0;row<4;++row) {
        int count=starts[row+1]-starts[row];
        int cell=(getWidth()-40)/count;
        for(int j=0;j<count;++j) {
            auto& c=*controls[(size_t)(starts[row]+j)];
            int x=20+j*cell,y=115+row*158;
            c.label.setBounds(x,y,cell-4,22);
            c.slider.setBounds(x,y+22,cell-4,100);
        }
    }
    meters.setBounds(22,728,getWidth()-44,32);
}
void AuroraEditor::timerCallback()
{
    meters.setText(juce::String::formatted("INPUT PEAK  %.1f dBFS       OUTPUT PEAK  %.1f dBFS       OUTPUT RMS  %.1f dBFS",
        processor.inputPeak.load(),processor.outputPeak.load(),processor.rmsLevel.load()),juce::dontSendNotification);
}
juce::AudioProcessorEditor* AuroraProcessor::createEditor() { return new AuroraEditor(*this); }
