#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include <sched.h>
#include <dlfcn.h>
#include <chrono>
#include "AudioBuffer.h"
#include "CircularBuffer.h"
#include "avst/avst.h"
#include "LockFreeQueue.h"

#define LOG_TAG "AudioApp"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct ParameterChange {
    int pluginIndex;
    int paramIndex;
    float value;
};

class AudioEngine : public oboe::AudioStreamCallback {
public:
    AudioEngine() : circularBuffer(1024), paramQueue(128) {
        for (int i = 0; i < 2; ++i) {
            bufferPool.push_back(new float[4096]);
        }
    }

    ~AudioEngine() {
        for (auto buffer : bufferPool) {
            delete[] buffer;
        }
    }

    void addPlugin(avst::PluginHandle *plugin) {
        plugins.push_back(plugin);
    }

    void removePlugin(avst::PluginHandle *plugin) {
        for (int i = 0; i < plugins.size(); ++i) {
            if (plugins[i] == plugin) {
                plugins.erase(plugins.begin() + i);
                break;
            }
        }
    }

    void setParameter(int pluginIndex, int paramIndex, float value) {
        ParameterChange change = {pluginIndex, paramIndex, value};
        paramQueue.push(change);
    }

    void setAudioBuffer(float *data, int sampleRate, int channelCount, int frameCount) {
        if (audioBuffer != nullptr) {
            delete audioBuffer;
        }
        audioBuffer = new AudioBuffer(data, sampleRate, channelCount, frameCount);
    }

    void setBufferSize(int bufferSize) {
        if (stream_) {
            stream_->setBufferSizeInFrames(bufferSize);
        }
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

        // Set CPU affinity
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

        // Set thread priority
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &param);

        // Log latency
        double latency = stream_->calculateLatencyMillis();
        ALOGI("Latency: %f ms", latency);

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
        ParameterChange change;
        while(paramQueue.pop(change)) {
            if (change.pluginIndex < plugins.size()) {
                plugins[change.pluginIndex]->plugin->setParameter(change.paramIndex, change.value);
            }
        }

        if (isPlaying && audioBuffer != nullptr) {
            int frameCount = audioBuffer->getFrameCount();
            float *output = static_cast<float *>(audioData);
            for (int i = 0; i < numFrames; ++i) {
                float sample;
                if (circularBuffer.read(sample)) {
                    output[i] = sample;
                } else {
                    if (playbackPosition < frameCount) {
                        output[i] = audioBuffer->getData()[playbackPosition++];
                    } else {
                        output[i] = 0;
                    }
                }
            }
        } else {
            memset(audioData, 0, numFrames * sizeof(float));
        }
        return oboe::DataCallbackResult::Continue;
    }

    std::vector<avst::PluginHandle *> plugins;
    std::vector<float *> bufferPool;
    int currentBuffer = 0;
private:
    oboe::AudioStream *stream_ = nullptr;
    int32_t sampleRate_ = 0;
    int32_t bufferSize_ = 0;
    AudioBuffer *audioBuffer = nullptr;
    bool isPlaying = false;
    std::atomic<int> playbackPosition;
    CircularBuffer circularBuffer;
    LockFreeQueue<ParameterChange> paramQueue;
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

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setBufferSize(JNIEnv *env, jclass clazz, jint buffer_size) {
    engine.setBufferSize(buffer_size);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1loadPlugin(JNIEnv *env, jclass clazz, jstring path) {
    const char *pathStr = env->GetStringUTFChars(path, nullptr);
    void *handle = dlopen(pathStr, RTLD_LAZY);
    env->ReleaseStringUTFChars(path, pathStr);

    if (!handle) {
        ALOGE("Failed to load plugin: %s", dlerror());
        return 0;
    }

    avst::CreateAvstPlugin_t *createPlugin = (avst::CreateAvstPlugin_t *) dlsym(handle, "createAvstPlugin");
    if (!createPlugin) {
        ALOGE("Failed to find createAvstPlugin function: %s", dlerror());
        dlclose(handle);
        return 0;
    }

    avst::IAvstPlugin *plugin = createPlugin();
    avst::PluginHandle *pluginHandle = new avst::PluginHandle(plugin, handle);
    engine.addPlugin(pluginHandle);
    return (jlong) pluginHandle;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1unloadPlugin(JNIEnv *env, jclass clazz, jlong native_handle) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    engine.removePlugin(pluginHandle);
    delete pluginHandle->plugin;
    dlclose(pluginHandle->handle);
    delete pluginHandle;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1getParameterCount(JNIEnv *env, jclass clazz, jlong native_handle) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    return pluginHandle->plugin->getParameterCount();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1getParameterName(JNIEnv *env, jclass clazz, jlong native_handle, jint index) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    const char *name = pluginHandle->plugin->getParameterName(index);
    return env->NewStringUTF(name);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1getParameter(JNIEnv *env, jclass clazz, jlong native_handle, jint index) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    return pluginHandle->plugin->getParameter(index);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1setParameter(JNIEnv *env, jclass clazz, jlong native_handle, jint index, jfloat value) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    int pluginIndex = -1;
    for (int i = 0; i < engine.plugins.size(); ++i) {
        if (engine.plugins[i] == pluginHandle) {
            pluginIndex = i;
            break;
        }
    }
    if (pluginIndex != -1) {
        engine.setParameter(pluginIndex, index, value);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1process(JNIEnv *env, jclass clazz, jlongArray native_handles, jfloatArray buffer, jint sample_rate) {
    jlong *handles = env->GetLongArrayElements(native_handles, nullptr);
    jfloat *data = env->GetFloatArrayElements(buffer, nullptr);
    int count = env->GetArrayLength(native_handles);
    int frameCount = env->GetArrayLength(buffer);

    avst::AudioIOConfig config = {
            .sampleRate = (float) sample_rate,
            .currentOutputChannels = 1,
            .currentInputChannels = 1
    };

    float *input = data;
    float *output = engine.bufferPool[engine.currentBuffer];
    engine.currentBuffer = (engine.currentBuffer + 1) % engine.bufferPool.size();
    int currentChannels = 1;

    for (int i = 0; i < count; i++) {
        avst::PluginHandle *pluginHandle = (avst::PluginHandle *) handles[i];
        avst::AudioIOConfig pluginConfig = pluginHandle->plugin->getAudioIOConfig();

        if (pluginConfig.currentInputChannels != currentChannels) {
            ALOGE("Plugin %d has incompatible input channels", i);
            // Here I should handle the error, but for now I will just log it.
        }

        if (!pluginHandle->bypassed) {
            try {
                avst::ProcessContext context = {
                        .frameCount = (uint32_t) frameCount,
                        .outputs = &output,
                        .inputs = (const float **) &input
                };
                pluginHandle->plugin->processAudio(context);
                input = output;
                if (i < count - 1) {
                    output = engine.bufferPool[engine.currentBuffer];
                    engine.currentBuffer = (engine.currentBuffer + 1) % engine.bufferPool.size();
                }
            } catch (const std::exception &e) {
                ALOGE("Plugin %d threw an exception: %s", i, e.what());
                pluginHandle->bypassed = true;
            }
        }
        currentChannels = pluginConfig.currentOutputChannels;
    }

    if (input != data) {
        memcpy(data, input, frameCount * sizeof(float));
    }

    env->ReleaseLongArrayElements(native_handles, handles, JNI_ABORT);
    env->ReleaseFloatArrayElements(buffer, data, 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_avst_Plugin_native_1setBypass(JNIEnv *env, jclass clazz, jlong native_handle, jboolean bypass) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    pluginHandle->bypassed = bypass;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1getCpuUsage(JNIEnv *env, jclass clazz, jlong native_handle) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    return pluginHandle->cpuUsage;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1savePreset(JNIEnv *env, jclass clazz, jlong native_handle) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    std::vector<uint8_t> state = pluginHandle->plugin->saveState();
    jbyteArray byteArray = env->NewByteArray(state.size());
    env->SetByteArrayRegion(byteArray, 0, state.size(), (const jbyte *) state.data());
    return byteArray;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1loadPreset(JNIEnv *env, jclass clazz, jlong native_handle, jbyteArray preset) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    jbyte *data = env->GetByteArrayElements(preset, nullptr);
    int size = env->GetArrayLength(preset);
    std::vector<uint8_t> state(data, data + size);
    pluginHandle->plugin->loadState(state);
    env->ReleaseByteArrayElements(preset, data, JNI_ABORT);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1saveChain(JNIEnv *env, jclass clazz, jlongArray native_handles) {
    jlong *handles = env->GetLongArrayElements(native_handles, nullptr);
    int count = env->GetArrayLength(native_handles);

    std::vector<uint8_t> chainState;
    for (int i = 0; i < count; i++) {
        avst::PluginHandle *pluginHandle = (avst::PluginHandle *) handles[i];
        std::vector<uint8_t> pluginState = pluginHandle->plugin->saveState();
        chainState.insert(chainState.end(), pluginState.begin(), pluginState.end());
    }

    jbyteArray byteArray = env->NewByteArray(chainState.size());
    env->SetByteArrayRegion(byteArray, 0, chainState.size(), (const jbyte *) chainState.data());
    return byteArray;
}
