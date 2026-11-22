#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Presets de fabrica. El 0 es "Init": todos los parametros en su valor por defecto.
namespace jah
{
struct PV { const char* id; float v; };
struct Preset { const char* name; std::vector<PV> v; };

inline const std::vector<Preset>& presets()
{
    // head: 0 British OD, 1 Modern 3Ch, 2 UK Iommi, 3 American Thick
    // pedal: 0 Off, 1 Overdrive, 2 Distortion | odmod: 0 Stock, 1 Tight, 2 Hot, 3 Clean, 4 Mid+
    // mod: 1 Chorus, 2 Flanger, 3 Phaser, 4 Tremolo, 5 Slicer, 6 Ring Mod
    static const std::vector<Preset> p = {
        { "Init", {} },

        { "British OD - Rhythm", { {"head",0},{"pedal",1},{"odmod",1},{"drive",3},{"ptone",5},{"gain",6.5f},{"bass",5.5f},{"mid",6.5f},{"treble",6},{"presence",5.5f},{"master",-6},{"gthr",-50},{"grel",80},{"rmix",10},{"rsize",3} } },
        { "British OD - Lead",   { {"head",0},{"pedal",1},{"odmod",0},{"drive",4},{"ptone",6},{"gain",7.5f},{"bass",5},{"mid",7},{"treble",6},{"presence",6},{"master",-6},{"gthr",-55},{"dmix",28},{"dtime",420},{"dfb",35},{"rmix",18},{"rsize",5} } },
        { "British OD - Clean",  { {"head",0},{"pedal",0},{"gain",2.5f},{"bass",5.5f},{"mid",5},{"treble",6},{"presence",5},{"master",-4},{"mod",1},{"mrate",2.5f},{"mdepth",4},{"mmix",30},{"rmix",20},{"rsize",5} } },

        { "Modern 3Ch - Rhythm", { {"head",1},{"pedal",1},{"odmod",1},{"drive",2},{"ptone",5},{"gain",7},{"bass",5},{"mid",4},{"treble",6.5f},{"presence",6},{"master",-6},{"gthr",-45},{"grel",60} } },
        { "Modern 3Ch - Lead",   { {"head",1},{"pedal",1},{"odmod",2},{"drive",3},{"ptone",6},{"gain",8},{"bass",5},{"mid",5.5f},{"treble",6},{"presence",6.5f},{"master",-6},{"gthr",-50},{"dmix",25},{"dtime",380},{"dfb",35},{"rmix",15} } },
        { "Modern 3Ch - Clean",  { {"head",1},{"pedal",0},{"gain",2},{"bass",5.5f},{"mid",5},{"treble",5.5f},{"presence",4.5f},{"master",-4},{"mod",3},{"mrate",2},{"mdepth",5},{"mmix",40},{"rmix",22} } },

        { "UK Iommi - Rhythm",   { {"head",2},{"pedal",1},{"odmod",4},{"drive",4},{"ptone",6},{"gain",6},{"bass",5.5f},{"mid",6},{"treble",7},{"presence",5},{"master",-5},{"gthr",-55} } },
        { "UK Iommi - Lead",     { {"head",2},{"pedal",1},{"odmod",0},{"drive",5},{"ptone",6},{"gain",7},{"bass",5},{"mid",6.5f},{"treble",7},{"presence",6},{"master",-6},{"gthr",-55},{"dmix",22},{"dtime",450},{"dfb",30},{"rmix",20} } },
        { "UK Iommi - Clean",    { {"head",2},{"pedal",0},{"gain",2},{"bass",5},{"mid",5},{"treble",6.5f},{"presence",5},{"master",-3},{"mod",1},{"mrate",2},{"mdepth",3},{"mmix",25},{"rmix",25},{"rsize",5} } },

        { "American Thick - Rhythm", { {"head",3},{"pedal",1},{"odmod",1},{"drive",3},{"ptone",5},{"gain",7},{"bass",6},{"mid",3.5f},{"treble",6},{"presence",6},{"master",-6},{"gthr",-45},{"grel",70} } },
        { "American Thick - Lead",   { {"head",3},{"pedal",1},{"odmod",2},{"drive",3},{"ptone",6},{"gain",8.5f},{"bass",5},{"mid",6},{"treble",6},{"presence",5.5f},{"master",-6},{"gthr",-50},{"dmix",26},{"dtime",400},{"dfb",35},{"rmix",15} } },
        { "American Thick - Clean",  { {"head",3},{"pedal",0},{"gain",2.5f},{"bass",5.5f},{"mid",5},{"treble",6},{"presence",5},{"master",-4},{"mod",1},{"mrate",2.5f},{"mdepth",4},{"mmix",25},{"rmix",25} } },

        { "Especial - Tremolo Surf", { {"head",1},{"pedal",0},{"gain",2},{"bass",5},{"mid",5},{"treble",6},{"presence",4},{"master",-4},{"mod",4},{"mrate",4},{"mdepth",7},{"mshape",2},{"dmix",25},{"dtime",300},{"dfb",30},{"rmix",35},{"rsize",6} } },
        { "Especial - Slicer Chug",  { {"head",3},{"pedal",1},{"odmod",2},{"drive",2},{"gain",7.5f},{"bass",5.5f},{"mid",5},{"treble",6},{"presence",5.5f},{"master",-6},{"gthr",-50},{"mod",5},{"mrate",4},{"mdepth",10},{"mshape",8},{"mpat",3} } },
    };
    return p;
}

inline int presetCount() { return (int) presets().size(); }
inline juce::String presetName (int i) { return presets()[(size_t) juce::jlimit (0, presetCount() - 1, i)].name; }

inline void applyPreset (juce::AudioProcessorValueTreeState& apvts, int index)
{
    index = juce::jlimit (0, presetCount() - 1, index);
    for (auto* prm : apvts.processor.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            r->setValueNotifyingHost (r->getDefaultValue());
    for (const auto& pv : presets()[(size_t) index].v)
        if (auto* r = apvts.getParameter (pv.id))
            r->setValueNotifyingHost (r->convertTo0to1 (pv.v));
}
}
