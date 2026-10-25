#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

static juce::AudioParameterFloatAttributes attrs() { return {}; }
juce::AudioProcessorValueTreeState::ParameterLayout AuroraProcessor::makeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&](const char* id, const char* name, float lo, float hi, float def, float step = 0.01f) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(lo, hi, step), def, attrs()));
    };
    add("input", "Input Gain dB", -24, 24, 0);
    add("output", "Output Gain dB", -24, 12, 0);
    add("mix", "Global Wet Percent", 0, 100, 100, 0.1f);
    add("hpf", "High Pass Hz", 20, 150, 25, 1);
    add("lowfreq", "Low Shelf Hz", 30, 500, 110, 1);
    add("lowgain", "Low Shelf dB", -12, 12, 0);
    add("midfreq", "Low Mid Hz", 100, 3000, 450, 1);
    add("midgain", "Low Mid dB", -12, 12, 0);
    add("midq", "Low Mid Q", 0.2f, 5, 0.8f);
    add("upperfreq", "High Mid Hz", 700, 12000, 3000, 1);
    add("uppergain", "High Mid dB", -12, 12, 0);
    add("upperq", "High Mid Q", 0.2f, 5, 0.8f);
    add("highfreq", "High Shelf Hz", 2000, 18000, 10000, 1);
    add("highgain", "High Shelf dB", -12, 12, 0);
    add("threshold", "Compressor Threshold dB", -36, 0, -12);
    add("ratio", "Compressor Ratio", 1, 12, 2);
    add("attack", "Compressor Attack ms", 0.1f, 200, 25);
    add("release", "Compressor Release ms", 10, 1200, 180, 1);
    add("knee", "Compressor Knee dB", 0, 24, 6);
    add("makeup", "Compressor Makeup dB", 0, 24, 0);
    add("width", "Stereo Width Percent", 0, 200, 100, 0.1f);
    add("drive", "Saturation Drive dB", 0, 24, 0);
    add("satmix", "Saturation Mix Percent", 0, 100, 0, 0.1f);
    add("ceiling", "Limiter Ceiling dBFS", -12, 0, -1);
    add("limdrive", "Limiter Input dB", 0, 24, 0);
    add("bypass", "Bypass (0/1)", 0, 1, 0, 1);
    return {p.begin(), p.end()};
}
AuroraProcessor::AuroraProcessor()
 : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
     .withInput("Input", juce::AudioChannelSet::stereo(), true)
     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   state(*this, nullptr, "PARAMETERS", makeParameters()) {}
float AuroraProcessor::gain(const char* id) const { return state.getRawParameterValue(id)->load(); }
void AuroraProcessor::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    fs = sampleRate;
    compEnvelope.fill(0.f);
    for (auto* bank : {&lowShelf, &highShelf, &lowMid, &highMid, &hp})
        for (auto& f : *bank) f.reset();
    delayLength = std::max(1, (int)std::ceil(fs * 0.005));
    delayBuffer.setSize(2, delayLength + std::max(1, maximumBlockSize));
    delayBuffer.clear();
    delayWrite = 0;
    // This release uses instantaneous peak limiting; no latency is reported.
    setLatencySamples(0);
}
void AuroraProcessor::updateFilters()
{
    auto sr = fs;
    auto low = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, gain("lowfreq"), 0.707f, juce::Decibels::decibelsToGain(gain("lowgain")));
    auto high = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, std::min((float)(sr*0.45),gain("highfreq")), 0.707f, juce::Decibels::decibelsToGain(gain("highgain")));
    auto lm = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, gain("midfreq"), gain("midq"), juce::Decibels::decibelsToGain(gain("midgain")));
    auto hm = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, std::min((float)(sr*0.45),gain("upperfreq")), gain("upperq"), juce::Decibels::decibelsToGain(gain("uppergain")));
    auto highpass = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, gain("hpf"));
    for (int c=0;c<2;++c) { *lowShelf[c].coefficients=*low; *highShelf[c].coefficients=*high;
        *lowMid[c].coefficients=*lm; *highMid[c].coefficients=*hm; *hp[c].coefficients=*highpass; }
}
void AuroraProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples(), channels = std::min(2, buffer.getNumChannels());
    if (channels < 2) return;
    if (gain("bypass") >= 0.5f) return;
    updateFilters();
    float peakIn=0, peakOut=0; double sumSq=0;
    const float in = juce::Decibels::decibelsToGain(gain("input"));
    const float out = juce::Decibels::decibelsToGain(gain("output"));
    const float wet = gain("mix")*0.01f;
    const float width = gain("width")*0.01f;
    const float drive = juce::Decibels::decibelsToGain(gain("drive"));
    const float satmix = gain("satmix")*0.01f;
    const float threshold = gain("threshold"), ratio = gain("ratio"), knee = gain("knee");
    const float makeup = juce::Decibels::decibelsToGain(gain("makeup"));
    const float att = std::exp(-1.f/(float)(fs*gain("attack")*0.001f));
    const float rel = std::exp(-1.f/(float)(fs*gain("release")*0.001f));
    const float limitDrive = juce::Decibels::decibelsToGain(gain("limdrive"));
    const float ceiling = juce::Decibels::decibelsToGain(gain("ceiling"));
    auto* l = buffer.getWritePointer(0); auto* r = buffer.getWritePointer(1);
    for (int i=0;i<n;++i)
    {
        const float dryL=l[i], dryR=r[i];
        peakIn=std::max(peakIn,std::max(std::abs(dryL),std::abs(dryR)));
        float x[2]={dryL*in,dryR*in};
        for (int c=0;c<2;++c)
        {
            x[c]=hp[c].processSample(x[c]);
            x[c]=lowShelf[c].processSample(x[c]);
            x[c]=lowMid[c].processSample(x[c]);
            x[c]=highMid[c].processSample(x[c]);
            x[c]=highShelf[c].processSample(x[c]);
            const float level=std::abs(x[c]);
            compEnvelope[c]=(level>compEnvelope[c]?att:rel)*compEnvelope[c]
                          +(1.f-(level>compEnvelope[c]?att:rel))*level;
            const float db=juce::Decibels::gainToDecibels(compEnvelope[c],-120.f);
            const float over=db-threshold;
            float compressed=0;
            if (knee>0 && over>-knee*0.5f && over<knee*0.5f)
                compressed=(1.f/ratio-1.f)*std::pow(over+knee*0.5f,2)/(2*knee);
            else if (over>=knee*0.5f) compressed=(1.f/ratio-1.f)*over;
            x[c]*=juce::Decibels::decibelsToGain(compressed)*makeup;
            const float saturated=std::tanh(x[c]*drive)/std::max(0.0001f,std::tanh(drive));
            x[c]=x[c]*(1-satmix)+saturated*satmix;
        }
        const float mid=(x[0]+x[1])*0.5f, side=(x[0]-x[1])*0.5f*width;
        x[0]=mid+side; x[1]=mid-side;
        // Sample-peak safety limiter (not true-peak; no oversampling).
        x[0]*=limitDrive; x[1]*=limitDrive;
        const float pk=std::max(std::abs(x[0]),std::abs(x[1]));
        const float limiterGain=pk>ceiling?ceiling/pk:1.f;
        x[0]*=limiterGain; x[1]*=limiterGain;
        l[i]=(dryL*(1-wet)+x[0]*wet)*out;
        r[i]=(dryR*(1-wet)+x[1]*wet)*out;
        peakOut=std::max(peakOut,std::max(std::abs(l[i]),std::abs(r[i])));
        sumSq+=(double)l[i]*l[i]+(double)r[i]*r[i];
    }
    inputPeak.store(juce::Decibels::gainToDecibels(peakIn,-100.f));
    outputPeak.store(juce::Decibels::gainToDecibels(peakOut,-100.f));
    rmsLevel.store(juce::Decibels::gainToDecibels((float)std::sqrt(sumSq/std::max(1,2*n)),-100.f));
}
void AuroraProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml=state.copyState().createXml()) copyXmlToBinary(*xml,dest);
}
void AuroraProcessor::setStateInformation(const void* data,int size)
{
    if (auto xml=getXmlFromBinary(data,size))
        if (xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AuroraProcessor(); }
