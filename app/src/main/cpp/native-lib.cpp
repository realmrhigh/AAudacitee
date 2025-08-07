#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>

#define LOG_TAG "AudioApp"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class AudioEngine : public oboe::AudioStreamCallback {
public:
    oboe::Result start() {
        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Input);
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

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override {
        // Pass-through audio
        return oboe::DataCallbackResult::Continue;
    }

private:
    oboe::AudioStream *stream_ = nullptr;
    int32_t sampleRate_ = 0;
    int32_t bufferSize_ = 0;
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_audioapp_ui_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
