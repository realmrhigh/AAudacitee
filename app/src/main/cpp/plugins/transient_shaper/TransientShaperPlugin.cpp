#include "TransientShaperPlugin.h"
#include <vector>
#include <string>
#include <memory>

namespace avst {

TransientShaperPlugin::TransientShaperPlugin() :
    attackDb(0.0f),
    sustainDb(0.0f)
{
    config.sampleRate = 44100;
    config.currentInputChannels = 1;
    config.currentOutputChannels = 1;
    shaper.setParameters(config.sampleRate, attackDb, sustainDb);
}

PluginInfo TransientShaperPlugin::getPluginInfo() const {
    return {
        .id = "com.example.transient_shaper",
        .name = "Transient Shaper",
        .vendor = "aVST",
        .version = "0.0.1",
        .type = PluginType::EFFECT,
        .category = PluginCategory::EFFECT,
        .hasUI = false,
        .isSynth = false,
        .acceptsMidi = false,
        .producesMidi = false,
        .cpuUsageEstimate = 0.1f,
        .memoryUsageKB = 128,
        .supportsBackground = true
    };
}

bool TransientShaperPlugin::initialize(const AudioIOConfig &config) {
    this->config = config;
    shaper.setParameters(config.sampleRate, attackDb, sustainDb);
    return true;
}

void TransientShaperPlugin::shutdown() {}

bool TransientShaperPlugin::setAudioIOConfig(const AudioIOConfig &config) {
    this->config = config;
    shaper.setParameters(config.sampleRate, attackDb, sustainDb);
    return true;
}

void TransientShaperPlugin::processAudio(ProcessContext &context) {
    for (int j = 0; j < config.currentOutputChannels; ++j) {
        float* in = (float*)context.inputs[j];
        float* out = context.outputs[j];
        shaper.process(in, out, context.frameCount);
    }
}

void TransientShaperPlugin::processMidiMessage(const MidiMessage &message) {}

std::vector<uint8_t> TransientShaperPlugin::saveState() const {
    std::vector<uint8_t> state(sizeof(float) * 2);
    float* p = reinterpret_cast<float*>(state.data());
    *p++ = attackDb;
    *p++ = sustainDb;
    return state;
}

bool TransientShaperPlugin::loadState(const std::vector<uint8_t> &state) {
    if (state.size() < sizeof(float) * 2) {
        return false;
    }
    const float* p = reinterpret_cast<const float*>(state.data());
    attackDb = *p++;
    sustainDb = *p++;
    shaper.setParameters(config.sampleRate, attackDb, sustainDb);
    return true;
}
void TransientShaperPlugin::onLowMemory() {}

int TransientShaperPlugin::getParameterCount() const { return P_COUNT; }

const char *TransientShaperPlugin::getParameterName(int index) const {
    static std::string name;
    switch (index) {
        case P_ATTACK: name = "Attack"; break;
        case P_SUSTAIN: name = "Sustain"; break;
        default: name = ""; break;
    }
    return name.c_str();
}

float TransientShaperPlugin::getParameter(int index) const {
    switch (index) {
        case P_ATTACK: return attackDb;
        case P_SUSTAIN: return sustainDb;
        default: return 0.0f;
    }
}

void TransientShaperPlugin::setParameter(int index, float value) {
    switch (index) {
        case P_ATTACK: attackDb = value; break;
        case P_SUSTAIN: sustainDb = value; break;
        default: return;
    }
    shaper.setParameters(config.sampleRate, attackDb, sustainDb);
}

AudioIOConfig TransientShaperPlugin::getAudioIOConfig() const { return config; }
IAvstUI *TransientShaperPlugin::getUI() { return nullptr; }
void TransientShaperPlugin::setQuality(int quality) {}
void TransientShaperPlugin::getFrequencyResponse(std::vector<float>& magnitudes) {}

extern "C" __attribute__((visibility("default"))) IAvstPlugin *createAvstPlugin() {
    return new TransientShaperPlugin();
}

} // namespace avst
