#ifndef AUDIOAPP_LEVELERPLUGIN_H
#define AUDIOAPP_LEVELERPLUGIN_H

#include "../../avst/avst.h"
#include "../../dsp/Leveler.h"

namespace avst {

class LevelerPlugin : public IAvstPlugin {
public:
    LevelerPlugin();
    ~LevelerPlugin() override = default;

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
    dsp::Leveler leveler;
    AudioIOConfig config;

    float targetLevelDb;
    float speed;

    enum Parameter {
        P_TARGET_LEVEL,
        P_SPEED,
        P_GAIN,
        P_COUNT
    };
};

} // namespace avst

#endif //AUDIOAPP_LEVELERPLUGIN_H
