#include <imgui.h>
#include <imnodes.h>

#include <algorithm>
#include <vector>

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

public:
    int id;
    const char* name;
    char* input;
    char* output;
    bool param1;
    float param2;

    bool is_active = true;

    // constructors to init
    Block(const int i, const char* n) : id(i), name(n) {}
    Block(const int i, const char* n, char* in, char* o) : id(i), name(n), input(in), output(o) {}

    void show(Block block);
    void bypass(); // gets called by block -> pop up -> edit menu
    void deleteBlock(Block block);

};