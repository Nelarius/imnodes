#pragma once

#include <imgui.h>
#include <imnodes.h>

#include <algorithm>
#include <vector>
#include <string>

using namespace std;

enum DSPBlockType_ {
    Bass,
    IIR,
    Mixdown,
    LookAheadDelay,
    SignalGenerator,
    RoomIIR,
    SonarSpatial,
    EQ,
    Crossover,
    GlobalGain,
    InputGainOffset,
    Loudness,
    ProxyGain,
    InputCompressor,
    GainFB,
    Squancher1,
    DynamicHighPass,
    GravityExcursionCtrl,
    GravityLimiter,
    ThermalLimiter,
    GravitySoftClip,
    OutputGain,
    DACCompGain,

};
 
enum ControlBlockType_ {
    GainFBCtrl,
    SpeakerSizeXOverCtrl,
    SpeakerSizeGainControl,
    ToneHandler
};

class Block {

    int id;
    string name;
    string input;
    string output;

    bool is_active = true;

    public:

        Block();

        Block(const int i, string n) : id(i), name(n) {}
        Block(const int i, string n, string in, string o) : id(i), name(n), input(in), output(o) {}

        void show();
        void bypass();
        void deleteBlock(Block block);
};