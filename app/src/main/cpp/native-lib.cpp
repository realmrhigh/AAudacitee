#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include <sched.h>
#include <dlfcn.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include "AudioBuffer.h"
#include "CircularBuffer.h"
#include "ebur128.h"
#include "lame/lame.h"
#include "avst/avst.h"
#include "LockFreeQueue.h"
#include "ObjectPool.h"

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
        for (int i = 0; i < 4; ++i) {
            auto buffer = std::make_unique<std::vector<float>>(4096);
            bufferPool.release(std::move(buffer));
        }
    }

    ~AudioEngine() = default;

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

    AudioBuffer* getAudioBuffer() { return audioBuffer; }

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
    ObjectPool<std::vector<float>> bufferPool;

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
    avst::PluginHandle *pluginHandle = new avst::PluginHandle(plugin, handle, pathStr);
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
    auto output_buffer_ptr = engine.bufferPool.get();
    output_buffer_ptr->resize(frameCount);
    float* output = output_buffer_ptr->data();

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
                    auto next_output_buffer_ptr = engine.bufferPool.get();
                    next_output_buffer_ptr->resize(frameCount);
                    output = next_output_buffer_ptr->data();
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

    engine.bufferPool.release(std::move(output_buffer_ptr));


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

        // Path
        uint32_t pathLen = pluginHandle->path.length();
        chainState.insert(chainState.end(), (uint8_t*)&pathLen, (uint8_t*)&pathLen + sizeof(pathLen));
        chainState.insert(chainState.end(), pluginHandle->path.begin(), pluginHandle->path.end());

        // State
        std::vector<uint8_t> pluginState = pluginHandle->plugin->saveState();
        uint32_t stateLen = pluginState.size();
        chainState.insert(chainState.end(), (uint8_t*)&stateLen, (uint8_t*)&stateLen + sizeof(stateLen));
        chainState.insert(chainState.end(), pluginState.begin(), pluginState.end());
    }

    jbyteArray byteArray = env->NewByteArray(chainState.size());
    env->SetByteArrayRegion(byteArray, 0, chainState.size(), (const jbyte *) chainState.data());

    env->ReleaseLongArrayElements(native_handles, handles, JNI_ABORT);
    return byteArray;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1setPluginQuality(JNIEnv *env, jclass clazz, jlong native_handle, jint quality) {
    avst::PluginHandle *pluginHandle = (avst::PluginHandle *) native_handle;
    pluginHandle->plugin->setQuality(quality);
}

double getLoudnessDb() {
    AudioBuffer* audioBuffer = engine.getAudioBuffer();
    if (!audioBuffer) {
        return -70.0; // Return a default value if no audio is loaded
    }

    // This code is based on an assumed API for libebur128.
    // It needs to be verified against the actual library documentation.
    ebur128_state* st = ebur128_init(
        audioBuffer->getChannelCount(),
        audioBuffer->getSampleRate(),
        EBUR128_MODE_I
    );

    if (!st) {
        ALOGE("Failed to initialize libebur128");
        return -70.0;
    }

    ebur128_add_frames_float(st, audioBuffer->getData(), audioBuffer->getFrameCount());

    double loudness = 0.0;
    ebur128_loudness_global(st, &loudness);

    ebur128_destroy(&st);

    return loudness;
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1getLoudness(JNIEnv *env, jclass clazz) {
    return getLoudnessDb();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1normalizeLoudness(JNIEnv *env, jclass clazz, jdouble target_loudness) {
    AudioBuffer* audioBuffer = engine.getAudioBuffer();
    if (!audioBuffer) {
        return;
    }

    double currentLoudness = getLoudnessDb();
    if (currentLoudness < -70.0) { // Check for silence or error
        return;
    }

    double gainDb = target_loudness - currentLoudness;
    float gainLinear = pow(10.0, gainDb / 20.0);

    float* data = audioBuffer->getData();
    int frameCount = audioBuffer->getFrameCount();
    int channelCount = audioBuffer->getChannelCount();
    int totalSamples = frameCount * channelCount;

    for (int i = 0; i < totalSamples; ++i) {
        data[i] *= gainLinear;
    }
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_example_audioapp_avst_AvstHost_native_1loadChain(JNIEnv *env, jclass clazz, jbyteArray chain_data) {
    jbyte *data = env->GetByteArrayElements(chain_data, nullptr);
    int size = env->GetArrayLength(chain_data);

    std::vector<jlong> handles;
    uint8_t* current = (uint8_t*)data;
    uint8_t* end = current + size;

    while (current < end) {
        // Path
        uint32_t pathLen = *(uint32_t*)current;
        current += sizeof(pathLen);
        std::string path((char*)current, pathLen);
        current += pathLen;

        // State
        uint32_t stateLen = *(uint32_t*)current;
        current += sizeof(stateLen);
        std::vector<uint8_t> state(current, current + stateLen);
        current += stateLen;

        // Load plugin
        void *handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
            ALOGE("Failed to load plugin: %s", dlerror());
            continue;
        }

        avst::CreateAvstPlugin_t *createPlugin = (avst::CreateAvstPlugin_t *) dlsym(handle, "createAvstPlugin");
        if (!createPlugin) {
            ALOGE("Failed to find createAvstPlugin function: %s", dlerror());
            dlclose(handle);
            continue;
        }

        avst::IAvstPlugin *plugin = createPlugin();
        plugin->loadState(state);

        avst::PluginHandle *pluginHandle = new avst::PluginHandle(plugin, handle, path);
        engine.addPlugin(pluginHandle);
        handles.push_back((jlong)pluginHandle);
    }

    jlongArray result = env->NewLongArray(handles.size());
    env->SetLongArrayRegion(result, 0, handles.size(), handles.data());

    env->ReleaseByteArrayElements(chain_data, data, JNI_ABORT);
    return result;
}

void writeWavHeader(std::ofstream& file, int sampleRate, int channelCount, int frameCount) {
    int bitsPerSample = 16;
    int byteRate = sampleRate * channelCount * bitsPerSample / 8;
    int blockAlign = channelCount * bitsPerSample / 8;
    int subchunk2Size = frameCount * channelCount * bitsPerSample / 8;
    int chunkSize = 36 + subchunk2Size;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&chunkSize), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    int subchunk1Size = 16;
    file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    short audioFormat = 1;
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&channelCount), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&subchunk2Size), 4);
}

bool exportWav(const char* path, float* audioData, int sampleRate, int channelCount, int frameCount) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        ALOGE("Failed to open file for writing: %s", path);
        return false;
    }

    writeWavHeader(file, sampleRate, channelCount, frameCount);

    std::vector<short> intBuffer(frameCount * channelCount);
    for (int i = 0; i < frameCount * channelCount; ++i) {
        intBuffer[i] = static_cast<short>(audioData[i] * 32767.0f);
    }
    file.write(reinterpret_cast<const char*>(intBuffer.data()), intBuffer.size() * sizeof(short));

    file.close();
    return true;
}

bool exportMp3(const char* path, float* audioData, int sampleRate, int channelCount, int frameCount, int bitrate) {
    FILE* file = fopen(path, "wb");
    if (!file) {
        ALOGE("Failed to open file for writing: %s", path);
        return false;
    }

    lame_t lame = lame_init();
    lame_set_in_samplerate(lame, sampleRate);
    lame_set_num_channels(lame, channelCount);
    lame_set_VBR(lame, vbr_off);
    lame_set_brate(lame, bitrate);
    lame_set_quality(lame, 2); // 2=high, 5=medium, 7=low
    lame_init_params(lame);

    int pcm_buffer_size = 1024;
    std::vector<float> pcm_buffer(pcm_buffer_size * channelCount);
    std::vector<float> left_buffer(pcm_buffer_size);
    std::vector<float> right_buffer(pcm_buffer_size);
    int mp3_buffer_size = 1.25 * pcm_buffer_size + 7200;
    std::vector<unsigned char> mp3_buffer(mp3_buffer_size);

    int read = 0;
    int write = 0;

    while (read < frameCount) {
        int to_read = std::min(pcm_buffer_size, frameCount - read);
        for(int i = 0; i < to_read; ++i) {
            if (channelCount == 1) {
                left_buffer[i] = audioData[(read + i) * channelCount];
            } else {
                left_buffer[i] = audioData[(read + i) * channelCount];
                right_buffer[i] = audioData[(read + i) * channelCount + 1];
            }
        }

        int encoded_bytes = lame_encode_buffer_float(
            lame,
            left_buffer.data(),
            channelCount == 2 ? right_buffer.data() : nullptr,
            to_read,
            mp3_buffer.data(),
            mp3_buffer_size
        );

        if (encoded_bytes < 0) {
            ALOGE("LAME encoding failed with error code: %d", encoded_bytes);
            break;
        }

        fwrite(mp3_buffer.data(), 1, encoded_bytes, file);
        read += to_read;
    }

    int encoded_bytes = lame_encode_flush(lame, mp3_buffer.data(), mp3_buffer_size);
    fwrite(mp3_buffer.data(), 1, encoded_bytes, file);

    lame_close(lame);
    fclose(file);
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1exportFile(JNIEnv *env, jclass clazz, jstring path, jint format, jdouble target_loudness, jint bitrate) {
    AudioBuffer* audioBuffer = engine.getAudioBuffer();
    if (!audioBuffer) {
        return false;
    }

    int frameCount = audioBuffer->getFrameCount();
    int channelCount = audioBuffer->getChannelCount();
    int totalSamples = frameCount * channelCount;

    // Create a temporary buffer for normalized audio
    float* tempBuffer = new float[totalSamples];
    memcpy(tempBuffer, audioBuffer->getData(), totalSamples * sizeof(float));

    if (target_loudness > -70.0) { // Apply normalization if target is not silent
        double currentLoudness = getLoudnessDb();
        if (currentLoudness > -70.0) {
            double gainDb = target_loudness - currentLoudness;
            float gainLinear = pow(10.0, gainDb / 20.0);
            for (int i = 0; i < totalSamples; ++i) {
                tempBuffer[i] *= gainLinear;
            }
        }
    }

    const char *pathStr = env->GetStringUTFChars(path, nullptr);

    bool success = false;
    if (format == 0) { // WAV
        success = exportWav(pathStr, tempBuffer, audioBuffer->getSampleRate(), channelCount, frameCount);
    } else if (format == 1) { // MP3
        success = exportMp3(pathStr, tempBuffer, audioBuffer->getSampleRate(), channelCount, frameCount, bitrate);
    }

    env->ReleaseStringUTFChars(path, pathStr);
    delete[] tempBuffer;
    return success;
}
