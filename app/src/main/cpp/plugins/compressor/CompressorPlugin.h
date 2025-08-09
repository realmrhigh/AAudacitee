#ifndef AUDIOAPP_COMPRESSORPLUGIN_H
#define AUDIOAPP_COMPRESSORPLUGIN_H

#include "../../avst/avst.h"
#include "../../dsp/Compressor.h"
#include "../../CircularBuffer.h"

namespace avst {

class CompressorPlugin : public IAvstPlugin {
public:
    CompressorPlugin();
    ~CompressorPlugin() override = default;

    // IAvstPlugin interface
    PluginInfo getPluginInfo() const override;
    bool initialize(const AudioIOConfig &config) override;
    void shutdown() override;
    bool setAudioIOConfig(const AudioIOConfig &config) override;
    void processAudio(ProcessContext &context) override;
    void processMidiMessage(const MidiMessage &message) override;
    std::vector<uint8_t> saveState() const override;
    bool loadState(const std::vector<uint8_t> &state) override;
    void onLowMemory() override;
    int getParameterCount() const override;
    const char *getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;
    AudioIOConfig getAudioIOConfig() const override;
    IAvstUI *getUI() override;
    void setQuality(int quality) override;
    void getFrequencyResponse(std::vector<float>& magnitudes) override;

private:
    dsp::Compressor compressor;
    AudioIOConfig config;
    CircularBuffer<float> lookahead_buffer;

    float thresholdDb;
    float ratio;
    float attackMs;
    float releaseMs;
    float makeupGainDb;
    dsp::CompressorDetectionMode detectionMode;

    enum Parameter {
        P_THRESHOLD,
        P_RATIO,
        P_ATTACK,
        P_RELEASE,
        P_MAKEUP_GAIN,
        P_DETECTION_MODE,
        P_GAIN_REDUCTION,
        P_COUNT
    };
};

} // namespace avst

#endif //AUDIOAPP_COMPRESSORPLUGIN_H
