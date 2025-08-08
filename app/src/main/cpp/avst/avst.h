#ifndef AUDIOAPP_AVST_H
#define AUDIOAPP_AVST_H

#include <cstdint>
#include <vector>
#include <chrono>
#include <string>

namespace avst {

enum class PluginType {
    INSTRUMENT,
    EFFECT
};

enum class PluginCategory {
    SYNTHESIZER,
    EFFECT,
    ANALYZER
};

struct PluginInfo {
    const char *id;
    const char *name;
    const char *vendor;
    const char *version;
    PluginType type;
    PluginCategory category;
    bool hasUI;
    bool isSynth;
    bool acceptsMidi;
    bool producesMidi;
    float cpuUsageEstimate;
    int32_t memoryUsageKB;
    bool supportsBackground;
};

struct AudioIOConfig {
    float sampleRate;
    int32_t currentOutputChannels;
    int32_t currentInputChannels;
};

struct ProcessContext {
    uint32_t frameCount;
    float **outputs;
    const float **inputs;
};

struct MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

class IAvstUI;

class IAvstPlugin {
public:
    virtual ~IAvstPlugin() = default;
    virtual PluginInfo getPluginInfo() const = 0;
    virtual bool initialize(const AudioIOConfig &config) = 0;
    virtual void shutdown() = 0;
    virtual bool setAudioIOConfig(const AudioIOConfig &config) = 0;
    virtual void processAudio(ProcessContext &context) = 0;
    virtual void processMidiMessage(const MidiMessage &message) = 0;
    virtual std::vector<uint8_t> saveState() const = 0;
    virtual bool loadState(const std::vector<uint8_t> &state) = 0;
    virtual void onLowMemory() = 0;

    virtual int getParameterCount() const = 0;
    virtual const char *getParameterName(int index) const = 0;
    virtual float getParameter(int index) const = 0;
    virtual void setParameter(int index, float value) = 0;
    virtual AudioIOConfig getAudioIOConfig() const = 0;
    virtual IAvstUI *getUI() = 0;
    virtual void setQuality(int quality) = 0;
    virtual void getFrequencyResponse(std::vector<float>& magnitudes) = 0;
};

class IAvstUI {
public:
    virtual ~IAvstUI() = default;
    virtual void *getView() = 0;
    virtual void setParameter(int index, float value) = 0;
};

typedef IAvstPlugin *(*CreateAvstPlugin_t)();

class PluginHandle {
public:
    PluginHandle(IAvstPlugin *plugin, void *handle, const std::string& path) : plugin(plugin), handle(handle), path(path), bypassed(false), cpuUsage(0.0f) {}
    IAvstPlugin *plugin;
    void *handle;
    std::string path;
    bool bypassed;
    std::atomic<float> cpuUsage;
};

} // namespace avst

#endif //AUDIOAPP_AVST_H
