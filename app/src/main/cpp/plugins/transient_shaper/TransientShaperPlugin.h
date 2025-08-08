#ifndef AUDIOAPP_TRANSIENTSHAPERPLUGIN_H
#define AUDIOAPP_TRANSIENTSHAPERPLUGIN_H

#include "../../avst/avst.h"
#include "../../dsp/TransientShaper.h"

namespace avst {

class TransientShaperPlugin : public IAvstPlugin {
public:
    TransientShaperPlugin();
    ~TransientShaperPlugin() override = default;

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
    dsp::TransientShaper shaper;
    AudioIOConfig config;

    float attackDb;
    float sustainDb;

    enum Parameter {
        P_ATTACK,
        P_SUSTAIN,
        P_COUNT
    };
};

} // namespace avst

#endif //AUDIOAPP_TRANSIENTSHAPERPLUGIN_H
