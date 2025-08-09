#ifndef AUDIOAPP_AUDIOBUFFER_H
#define AUDIOAPP_AUDIOBUFFER_H

class AudioBuffer {
public:
    AudioBuffer(float *data, int sampleRate, int channelCount, int frameCount)
            : data(data), sampleRate(sampleRate), channelCount(channelCount), frameCount(frameCount), ownsData(true) {}

    ~AudioBuffer() {
        if (ownsData && data != nullptr) {
            delete[] data;
        }
    }

    float *getData() {
        return data;
    }

    int getSampleRate() {
        return sampleRate;
    }

    int getChannelCount() {
        return channelCount;
    }

    int getFrameCount() {
        return frameCount;
    }

private:
    float *data;
    int sampleRate;
    int channelCount;
    int frameCount;
    bool ownsData;
};

#endif //AUDIOAPP_AUDIOBUFFER_H
