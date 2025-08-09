#include "CompressorPlugin.h"
#include <vector>
#include <string>
#include <memory>

namespace avst {

CompressorPlugin::CompressorPlugin() :
    lookahead_buffer(44100),
    thresholdDb(-20.0f),
    ratio(4.0f),
    attackMs(5.0f),
    releaseMs(100.0f),
    makeupGainDb(0.0f),
    detectionMode(dsp::CompressorDetectionMode::PEAK)
{
    config.sampleRate = 44100;
    config.currentInputChannels = 1;
    config.currentOutputChannels = 1;
    compressor.setParameters(config.sampleRate, thresholdDb, ratio, attackMs, releaseMs, makeupGainDb);
}

PluginInfo CompressorPlugin::getPluginInfo() const {
    return {
        .id = "com.example.compressor",
        .name = "Compressor",
        .vendor = "aVST",
        .version = "0.0.1",
        .type = PluginType::EFFECT,
        .category = PluginCategory::EFFECT,
        .hasUI = false,
        .isSynth = false,
        .acceptsMidi = false,
        .producesMidi = false,
        .cpuUsageEstimate = 0.1f,
        .memoryUsageKB = 512,
        .supportsBackground = true
    };
}

bool CompressorPlugin::initialize(const AudioIOConfig &config) {
    this->config = config;
    compressor.setParameters(config.sampleRate, -20.0, 4.0, 5.0, 100.0, 0.0);
    lookahead_buffer.resize(config.sampleRate * 0.05); // 50ms lookahead
    return true;
}

void CompressorPlugin::shutdown() {}

bool CompressorPlugin::setAudioIOConfig(const AudioIOConfig &config) {
    this->config = config;
    compressor.setParameters(config.sampleRate, -20.0, 4.0, 5.0, 100.0, 0.0);
    lookahead_buffer.resize(config.sampleRate * 0.05); // 50ms lookahead
    return true;
}

void CompressorPlugin::processAudio(ProcessContext &context) {
    std::vector<float> gain_buffer(context.frameCount);
    std::vector<float> delayed_signal(context.frameCount);

    for (int j = 0; j < config.currentOutputChannels; ++j) {
        float* in = (float*)context.inputs[j];
        float* out = context.outputs[j];
        const float* sidechain = context.sidechain_inputs ? (const float*)context.sidechain_inputs[j] : nullptr;

        // 1. Calculate gain from non-delayed signal (or sidechain)
        compressor.calculate_gain(sidechain ? sidechain : in, gain_buffer.data(), context.frameCount);

        // 2. Write input to lookahead buffer and read delayed signal
        for (int i = 0; i < context.frameCount; ++i) {
            lookahead_buffer.write(in[i]);
            delayed_signal[i] = lookahead_buffer.read();
        }

        // 3. Apply gain to delayed signal
        compressor.apply_gain(delayed_signal.data(), gain_buffer.data(), out, context.frameCount);
    }
}

void CompressorPlugin::processMidiMessage(const MidiMessage &message) {}

std::vector<uint8_t> CompressorPlugin::saveState() const {
    std::vector<uint8_t> state(sizeof(float) * 5 + sizeof(int));
    float* p = reinterpret_cast<float*>(state.data());
    *p++ = thresholdDb;
    *p++ = ratio;
    *p++ = attackMs;
    *p++ = releaseMs;
    *p++ = makeupGainDb;
    *reinterpret_cast<int*>(p) = static_cast<int>(detectionMode);
    return state;
}

bool CompressorPlugin::loadState(const std::vector<uint8_t> &state) {
    if (state.size() < sizeof(float) * 5 + sizeof(int)) {
        return false;
    }
    const float* p = reinterpret_cast<const float*>(state.data());
    thresholdDb = *p++;
    ratio = *p++;
    attackMs = *p++;
    releaseMs = *p++;
    makeupGainDb = *p++;
    detectionMode = static_cast<dsp::CompressorDetectionMode>(*reinterpret_cast<const int*>(p));
    compressor.setParameters(config.sampleRate, thresholdDb, ratio, attackMs, releaseMs, makeupGainDb);
    compressor.setDetectionMode(detectionMode);
    return true;
}
void CompressorPlugin::onLowMemory() {}

int CompressorPlugin::getParameterCount() const { return P_COUNT; }

const char *CompressorPlugin::getParameterName(int index) const {
    static std::string name;
    switch (index) {
        case P_THRESHOLD: name = "Threshold"; break;
        case P_RATIO: name = "Ratio"; break;
        case P_ATTACK: name = "Attack"; break;
        case P_RELEASE: name = "Release"; break;
        case P_MAKEUP_GAIN: name = "Makeup Gain"; break;
        case P_DETECTION_MODE: name = "Detection Mode"; break;
        case P_GAIN_REDUCTION: name = "Gain Reduction"; break;
        default: name = ""; break;
    }
    return name.c_str();
}

float CompressorPlugin::getParameter(int index) const {
    switch (index) {
        case P_THRESHOLD: return thresholdDb;
        case P_RATIO: return ratio;
        case P_ATTACK: return attackMs;
        case P_RELEASE: return releaseMs;
        case P_MAKEUP_GAIN: return makeupGainDb;
        case P_DETECTION_MODE: return static_cast<float>(detectionMode);
        case P_GAIN_REDUCTION: return compressor.getGainReductionDb();
        default: return 0.0f;
    }
}

void CompressorPlugin::setParameter(int index, float value) {
    switch (index) {
        case P_THRESHOLD: thresholdDb = value; break;
        case P_RATIO: ratio = value; break;
        case P_ATTACK: attackMs = value; break;
        case P_RELEASE: releaseMs = value; break;
        case P_MAKEUP_GAIN: makeupGainDb = value; break;
        case P_DETECTION_MODE:
            detectionMode = static_cast<dsp::CompressorDetectionMode>((int)value);
            compressor.setDetectionMode(detectionMode);
            return; // No need to call setParameters for this
    }
    compressor.setParameters(config.sampleRate, thresholdDb, ratio, attackMs, releaseMs, makeupGainDb);
}

AudioIOConfig CompressorPlugin::getAudioIOConfig() const { return config; }
IAvstUI *CompressorPlugin::getUI() { return nullptr; }
void CompressorPlugin::setQuality(int quality) {}
void CompressorPlugin::getFrequencyResponse(std::vector<float>& magnitudes) {}

extern "C" __attribute__((visibility("default"))) IAvstPlugin *createAvstPlugin() {
    return new CompressorPlugin();
}

} // namespace avst
