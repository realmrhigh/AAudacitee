#include "LevelerPlugin.h"
#include <vector>
#include <string>
#include <memory>

namespace avst {

LevelerPlugin::LevelerPlugin() :
    targetLevelDb(-12.0f),
    speed(0.5f)
{
    config.sampleRate = 44100;
    config.currentInputChannels = 1;
    config.currentOutputChannels = 1;
    leveler.setParameters(config.sampleRate, targetLevelDb, speed);
}

PluginInfo LevelerPlugin::getPluginInfo() const {
    return {
        .id = "com.example.leveler",
        .name = "Leveler",
        .vendor = "aVST",
        .version = "0.0.1",
        .type = PluginType::EFFECT,
        .category = PluginCategory::EFFECT,
        .hasUI = false,
        .isSynth = false,
        .acceptsMidi = false,
        .producesMidi = false,
        .cpuUsageEstimate = 0.05f,
        .memoryUsageKB = 128,
        .supportsBackground = true
    };
}

bool LevelerPlugin::initialize(const AudioIOConfig &config) {
    this->config = config;
    leveler.setParameters(config.sampleRate, targetLevelDb, speed);
    return true;
}

void LevelerPlugin::shutdown() {}

bool LevelerPlugin::setAudioIOConfig(const AudioIOConfig &config) {
    this->config = config;
    leveler.setParameters(config.sampleRate, targetLevelDb, speed);
    return true;
}

void LevelerPlugin::processAudio(ProcessContext &context) {
    for (int j = 0; j < config.currentOutputChannels; ++j) {
        float* in = (float*)context.inputs[j];
        float* out = context.outputs[j];
        leveler.process(in, out, context.frameCount);
    }
}

void LevelerPlugin::processMidiMessage(const MidiMessage &message) {}

std::vector<uint8_t> LevelerPlugin::saveState() const {
    std::vector<uint8_t> state(sizeof(float) * 2);
    float* p = reinterpret_cast<float*>(state.data());
    *p++ = targetLevelDb;
    *p++ = speed;
    return state;
}

bool LevelerPlugin::loadState(const std::vector<uint8_t> &state) {
    if (state.size() < sizeof(float) * 2) {
        return false;
    }
    const float* p = reinterpret_cast<const float*>(state.data());
    targetLevelDb = *p++;
    speed = *p++;
    leveler.setParameters(config.sampleRate, targetLevelDb, speed);
    return true;
}
void LevelerPlugin::onLowMemory() {}

int LevelerPlugin::getParameterCount() const { return P_COUNT; }

const char *LevelerPlugin::getParameterName(int index) const {
    static std::string name;
    switch (index) {
        case P_TARGET_LEVEL: name = "Target Level"; break;
        case P_SPEED: name = "Speed"; break;
        case P_GAIN: name = "Gain"; break;
        default: name = ""; break;
    }
    return name.c_str();
}

float LevelerPlugin::getParameter(int index) const {
    switch (index) {
        case P_TARGET_LEVEL: return targetLevelDb;
        case P_SPEED: return speed;
        case P_GAIN: return leveler.getGainDb();
        default: return 0.0f;
    }
}

void LevelerPlugin::setParameter(int index, float value) {
    switch (index) {
        case P_TARGET_LEVEL: targetLevelDb = value; break;
        case P_SPEED: speed = value; break;
        default: return;
    }
    leveler.setParameters(config.sampleRate, targetLevelDb, speed);
}

AudioIOConfig LevelerPlugin::getAudioIOConfig() const { return config; }
IAvstUI *LevelerPlugin::getUI() { return nullptr; }
void LevelerPlugin::setQuality(int quality) {}
void LevelerPlugin::getFrequencyResponse(std::vector<float>& magnitudes) {}

extern "C" __attribute__((visibility("default"))) IAvstPlugin *createAvstPlugin() {
    return new LevelerPlugin();
}

} // namespace avst
