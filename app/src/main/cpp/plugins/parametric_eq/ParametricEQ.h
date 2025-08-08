#ifndef AUDIOAPP_PARAMETRICEQ_H
#define AUDIOAPP_PARAMETRICEQ_H

#include "../../avst/avst.h"
#include "../../dsp/Biquad.h"
#include "../../dsp/FrequencyResponse.h"

namespace avst {

class ParametricEQ : public IAvstPlugin {
public:
    ParametricEQ();
    ~ParametricEQ() override = default;

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
    std::unique_ptr<dsp::FrequencyResponse> frequencyResponse;
    static constexpr int NUM_BANDS = 6;

    struct Band {
        dsp::Biquad filter;
        float frequency;
        float q;
        float gain;
        dsp::BiquadFilterType type;
        bool enabled;
    };

    Band bands[NUM_BANDS];
    AudioIOConfig config;
    std::vector<dsp::Biquad> active_filters_cache;

    enum Parameter {
        P_FREQ,
        P_Q,
        P_GAIN,
        P_TYPE,
        P_ENABLED,
        P_COUNT
    };

    static constexpr int PARAM_COUNT = P_COUNT * NUM_BANDS;
};

} // namespace avst

#endif //AUDIOAPP_PARAMETRICEQ_H
