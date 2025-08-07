#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include "AudioBuffer.h"

#define LOG_TAG "AudioApp"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class AudioEngine : public oboe::AudioStreamCallback {
public:
    void setAudioBuffer(float *data, int sampleRate, int channelCount, int frameCount) {
        if (audioBuffer != nullptr) {
            delete audioBuffer;
        }
        audioBuffer = new AudioBuffer(data, sampleRate, channelCount, frameCount);
    }

    oboe::Result start() {
        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Output);
        builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        builder.setSharingMode(oboe::SharingMode::Exclusive);
        builder.setFormat(oboe::AudioFormat::Float);
        builder.setChannelCount(oboe::ChannelCount::Mono);
        builder.setCallback(this);

        oboe::Result result = builder.openStream(&stream_);
        if (result != oboe::Result::OK) {
            ALOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
            return result;
        }

        sampleRate_ = stream_->getSampleRate();
        bufferSize_ = stream_->getFramesPerBurst() * 2;

        result = stream_->requestStart();
        if (result != oboe::Result::OK) {
            ALOGE("Failed to start stream. Error: %s", oboe::convertToText(result));
            return result;
        }

        return oboe::Result::OK;
    }

    void stop() {
        if (stream_) {
            stream_->stop();
            stream_->close();
            stream_ = nullptr;
        }
    }

    void setPlaying(bool isPlaying) {
        this->isPlaying = isPlaying;
    }

    void setPlaybackPosition(int position) {
        playbackPosition = position;
    }

    int getPlaybackPosition() {
        return playbackPosition;
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override {
        if (isPlaying && audioBuffer != nullptr) {
            int frameCount = audioBuffer->getFrameCount();
            float *output = static_cast<float *>(audioData);
            float *input = audioBuffer->getData();
            for (int i = 0; i < numFrames; ++i) {
                if (playbackPosition < frameCount) {
                    output[i] = input[playbackPosition++];
                } else {
                    output[i] = 0;
                }
            }
        } else {
            memset(audioData, 0, numFrames * sizeof(float));
        }
        return oboe::DataCallbackResult::Continue;
    }

private:
    oboe::AudioStream *stream_ = nullptr;
    int32_t sampleRate_ = 0;
    int32_t bufferSize_ = 0;
    AudioBuffer *audioBuffer = nullptr;
    bool isPlaying = false;
    int playbackPosition = 0;
};

static AudioEngine engine;

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1create(JNIEnv *env, jclass clazz) {
    // Nothing to do here for now
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1start(JNIEnv *env, jclass clazz) {
    engine.start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1stop(JNIEnv *env, jclass clazz) {
    engine.stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setPlaying(JNIEnv *env, jclass clazz, jboolean is_playing) {
    engine.setPlaying(is_playing);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setPlaybackPosition(JNIEnv *env, jclass clazz, jint position) {
    engine.setPlaybackPosition(position);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1getPlaybackPosition(JNIEnv *env, jclass clazz) {
    return engine.getPlaybackPosition();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setAudioBuffer(JNIEnv *env, jclass clazz, jobject buffer) {
    jclass bufferClass = env->GetObjectClass(buffer);
    jmethodID getDataMethod = env->GetMethodID(bufferClass, "getData", "()[F");
    jmethodID getSampleRateMethod = env->GetMethodID(bufferClass, "getSampleRate", "()I");
    jmethodID getChannelCountMethod = env->GetMethodID(bufferClass, "getChannelCount", "()I");
    jmethodID getFrameCountMethod = env->GetMethodID(bufferClass, "getFrameCount", "()I");

    jfloatArray dataArray = (jfloatArray) env->CallObjectMethod(buffer, getDataMethod);
    jfloat *data = env->GetFloatArrayElements(dataArray, nullptr);
    jint sampleRate = env->CallIntMethod(buffer, getSampleRateMethod);
    jint channelCount = env->CallIntMethod(buffer, getChannelCountMethod);
    jint frameCount = env->CallIntMethod(buffer, getFrameCountMethod);

    engine.setAudioBuffer(data, sampleRate, channelCount, frameCount);

    env->ReleaseFloatArrayElements(dataArray, data, JNI_ABORT);
}
