#include "ParametricEQ.h"
#include <vector>
#include <string>
#include <memory>

namespace avst {

// Static member definition
constexpr int ParametricEQ::NUM_BANDS;

ParametricEQ::ParametricEQ() {
    config.sampleRate = 44100;
    config.currentInputChannels = 1;
    config.currentOutputChannels = 1;

    frequencyResponse = std::make_unique<dsp::FrequencyResponse>(NUM_BANDS, config.sampleRate);

    // Initialize bands with some default values
    bands[0] = { {}, 80.0f, 0.707f, 0.0f, dsp::BiquadFilterType::LOW_SHELF, true };
    bands[1] = { {}, 200.0f, 1.5f, 0.0f, dsp::BiquadFilterType::PEAK, true };
    bands[2] = { {}, 800.0f, 1.5f, 0.0f, dsp::BiquadFilterType::PEAK, true };
    bands[3] = { {}, 2000.0f, 1.5f, 0.0f, dsp::BiquadFilterType::PEAK, true };
    bands[4] = { {}, 5000.0f, 1.5f, 0.0f, dsp::BiquadFilterType::PEAK, true };
    bands[5] = { {}, 10000.0f, 0.707f, 0.0f, dsp::BiquadFilterType::HIGH_SHELF, true };

    for (int i = 0; i < NUM_BANDS; ++i) {
        bands[i].filter.setCoefficients(config.sampleRate, bands[i].frequency, bands[i].q, bands[i].gain);
        bands[i].filter.setType(bands[i].type);
    }
}

PluginInfo ParametricEQ::getPluginInfo() const {
    return {
        .id = "com.example.param_eq",
        .name = "Parametric EQ",
        .vendor = "aVST",
        .version = "0.0.1",
        .type = PluginType::EFFECT,
        .category = PluginCategory::EFFECT,
        .hasUI = false,
        .isSynth = false,
        .acceptsMidi = false,
        .producesMidi = false,
        .cpuUsageEstimate = 0.05f, // Increased estimate for 6 bands
        .memoryUsageKB = 256,    // Increased estimate
        .supportsBackground = true
    };
}

bool ParametricEQ::initialize(const AudioIOConfig &config) {
    this->config = config;
    frequencyResponse = std::make_unique<dsp::FrequencyResponse>(NUM_BANDS, config.sampleRate);
    for (int i = 0; i < NUM_BANDS; ++i) {
        bands[i].filter.setCoefficients(config.sampleRate, bands[i].frequency, bands[i].q, bands[i].gain);
    }
    return true;
}

void ParametricEQ::shutdown() {
    // Nothing to do here
}

bool ParametricEQ::setAudioIOConfig(const AudioIOConfig &config) {
    this->config = config;
    frequencyResponse = std::make_unique<dsp::FrequencyResponse>(NUM_BANDS, config.sampleRate);
    for (int i = 0; i < NUM_BANDS; ++i) {
        bands[i].filter.setCoefficients(config.sampleRate, bands[i].frequency, bands[i].q, bands[i].gain);
    }
    return true;
}

void ParametricEQ::processAudio(ProcessContext &context) {
    for (int j = 0; j < config.currentOutputChannels; ++j) {
        float* in = (float*)context.inputs[j];
        float* out = context.outputs[j];

        for (int k = 0; k < NUM_BANDS; ++k) {
            if (bands[k].enabled) {
                bands[k].filter.process(in, out, context.frameCount);
                in = out; // Chain the output of one filter to the input of the next
            }
        }

        // If the last filter was disabled, we need to copy the input to the output
        if (in != out) {
            memcpy(out, in, context.frameCount * sizeof(float));
        }
    }
}

void ParametricEQ::processMidiMessage(const MidiMessage &message) {
    // Not used
}

std::vector<uint8_t> ParametricEQ::saveState() const {
    std::vector<uint8_t> state;
    state.resize(sizeof(Band) * NUM_BANDS);
    memcpy(state.data(), &bands, sizeof(Band) * NUM_BANDS);
    return state;
}

bool ParametricEQ::loadState(const std::vector<uint8_t> &state) {
    if (state.size() < sizeof(Band) * NUM_BANDS) {
        return false;
    }
    memcpy(&bands, state.data(), sizeof(Band) * NUM_BANDS);
    for (int i = 0; i < NUM_BANDS; ++i) {
        bands[i].filter.setCoefficients(config.sampleRate, bands[i].frequency, bands[i].q, bands[i].gain);
        bands[i].filter.setType(bands[i].type);
    }
    return true;
}

void ParametricEQ::onLowMemory() {
    // Not used
}

int ParametricEQ::getParameterCount() const {
    return PARAM_COUNT;
}

const char *ParametricEQ::getParameterName(int index) const {
    int band = index / P_COUNT;
    int param = index % P_COUNT;
    static std::string name;

    switch (param) {
        case P_FREQ:
            name = "Band " + std::to_string(band + 1) + " Freq";
            break;
        case P_Q:
            name = "Band " + std::to_string(band + 1) + " Q";
            break;
        case P_GAIN:
            name = "Band " + std::to_string(band + 1) + " Gain";
            break;
        case P_TYPE:
            name = "Band " + std::to_string(band + 1) + " Type";
            break;
        case P_ENABLED:
            name = "Band " + std::to_string(band + 1) + " Enabled";
            break;
        default:
            name = "";
            break;
    }
    return name.c_str();
}

float ParametricEQ::getParameter(int index) const {
    int band = index / P_COUNT;
    int param = index % P_COUNT;

    if (band >= NUM_BANDS) {
        return 0.0f;
    }

    switch (param) {
        case P_FREQ: return bands[band].frequency;
        case P_Q: return bands[band].q;
        case P_GAIN: return bands[band].gain;
        case P_TYPE: return static_cast<float>(bands[band].type);
        case P_ENABLED: return bands[band].enabled ? 1.0f : 0.0f;
        default: return 0.0f;
    }
}

void ParametricEQ::setParameter(int index, float value) {
    int band = index / P_COUNT;
    int param = index % P_COUNT;

    if (band >= NUM_BANDS) {
        return;
    }

    switch (param) {
        case P_FREQ:
            bands[band].frequency = value;
            break;
        case P_Q:
            bands[band].q = value;
            break;
        case P_GAIN:
            bands[band].gain = value;
            break;
        case P_TYPE:
            bands[band].type = static_cast<dsp::BiquadFilterType>(static_cast<int>(value));
            bands[band].filter.setType(bands[band].type);
            break;
        case P_ENABLED:
            bands[band].enabled = value >= 0.5f;
            break;
    }
    bands[band].filter.setCoefficients(config.sampleRate, bands[band].frequency, bands[band].q, bands[band].gain);

    if (frequencyResponse) {
        frequencyResponse->invalidate();
    }
}

AudioIOConfig ParametricEQ::getAudioIOConfig() const {
    return config;
}

IAvstUI *ParametricEQ::getUI() {
    return nullptr; // No UI for now
}

void ParametricEQ::setQuality(int quality) {
    // Not used
}

void ParametricEQ::getFrequencyResponse(std::vector<float>& magnitudes) {
    if (frequencyResponse) {
        active_filters_cache.clear();
        for(int i = 0; i < NUM_BANDS; ++i) {
            if(bands[i].enabled) {
                active_filters_cache.push_back(bands[i].filter);
            }
        }
        frequencyResponse->calculateResponse(active_filters_cache.data(), active_filters_cache.size(), magnitudes);
    }
}

extern "C" __attribute__((visibility("default"))) IAvstPlugin *createAvstPlugin() {
    return new ParametricEQ();
}

} // namespace avst
