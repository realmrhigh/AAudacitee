#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <cstring>
#include <thread>
#include <atomic>
#include "AudioBuffer.h"
#include "CircularBuffer.h"
#include "LockFreeQueue.h"
#include "Biquad.h"
#include "Compressor.h"
#include "Leveler.h"
#include "TransientShaper.h"

#define LOG_TAG "AudioApp"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using AudioChunk = std::vector<uint8_t>;

// Converts a chunk of 16-bit stereo PCM data to two float arrays.
void convert_pcm_s16le_to_float(const std::vector<uint8_t>& pcm_s16le, float* left, float* right, int& numFrames) {
    numFrames = pcm_s16le.size() / 4; // 2 channels, 2 bytes/sample
    const int16_t* pcmData = reinterpret_cast<const int16_t*>(pcm_s16le.data());

    for (int i = 0; i < numFrames; ++i) {
        left[i] = static_cast<float>(pcmData[i * 2]) / 32768.0f;
        right[i] = static_cast<float>(pcmData[i * 2 + 1]) / 32768.0f;
    }
}

class AudioEngine : public oboe::AudioStreamCallback {
public:
    AudioEngine() : circularBuffer(8192) { // Increased buffer size for live mode
        // Initialize 6-band parametric EQ
        for (int i = 0; i < 6; i++) {
            eqBands[i].setType(dsp::BiquadFilterType::PEAK);
            eqBandEnabled[i] = true;
        }
        
        // Set default EQ frequencies
        setEQBandFrequency(0, 80.0f);   // Low shelf
        setEQBandFrequency(1, 200.0f);  // Peak
        setEQBandFrequency(2, 800.0f);  // Peak
        setEQBandFrequency(3, 2000.0f); // Peak
        setEQBandFrequency(4, 5000.0f); // Peak
        setEQBandFrequency(5, 10000.0f); // High shelf
        
        // Set shelf filters for first and last bands
        eqBands[0].setType(dsp::BiquadFilterType::LOW_SHELF);
        eqBands[5].setType(dsp::BiquadFilterType::HIGH_SHELF);
    }

    ~AudioEngine() {
        setLiveMode(false, nullptr);
    }

    void setLiveMode(bool isLive, LockFreeQueue<AudioChunk>* queue) {
        if (isLiveMode.load() == isLive) {
            return;
        }

        isLiveMode.store(isLive);
        captureQueue = queue;

        if (isLive) {
            if (!liveProcessingThread.joinable()) {
                liveProcessingThread = std::thread(&AudioEngine::liveProcessingLoop, this);
            }
        } else {
            if (liveProcessingThread.joinable()) {
                liveProcessingThread.join();
            }
        }
    }

    void liveProcessingLoop() {
        ALOGI("Starting live processing loop.");
        while (isLiveMode.load()) {
            if (!captureQueue) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            AudioChunk chunk;
            if (captureQueue->pop(chunk)) {
                int numFrames = chunk.size() / 4;
                if (numFrames > 0) {
                    std::vector<float> left(numFrames);
                    std::vector<float> right(numFrames);

                    convert_pcm_s16le_to_float(chunk, left.data(), right.data(), numFrames);

                    processDSP(left.data(), right.data(), numFrames);

                    for (int i = 0; i < numFrames; ++i) {
                        circularBuffer.write(left[i]);
                        circularBuffer.write(right[i]);
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        ALOGI("Exiting live processing loop.");
    }

    AudioBuffer* getAudioBuffer() { return audioBuffer; }

    void setAudioBuffer(float *data, int sampleRate, int channelCount, int frameCount) {
        if (audioBuffer != nullptr) {
            delete audioBuffer;
        }
        audioBuffer = new AudioBuffer(data, sampleRate, channelCount, frameCount);
        playbackPosition = 0;
        
        // Update DSP components with new sample rate
        updateDSPSampleRate(sampleRate);
        
        ALOGI("AudioBuffer set in engine: %d frames, %d channels, %d Hz", frameCount, channelCount, sampleRate);
    }

    void updateDSPSampleRate(int sampleRate) {
        // Update EQ bands
        for (int i = 0; i < 6; i++) {
            eqBands[i].setCoefficients(sampleRate, eqFrequencies[i], eqQValues[i], eqGains[i]);
        }
        
        // Update compressor
        compressor.setParameters(sampleRate, compressorThreshold, compressorRatio, 
                               compressorAttack, compressorRelease, compressorMakeupGain);
        
        // Update leveler
        leveler.setParameters(sampleRate, levelerTarget, levelerSpeed);
        
        // Update transient shaper
        transientShaper.setParameters(sampleRate, transientAttack, transientSustain);
    }

    void setBufferSize(int bufferSize) {
        if (stream_) {
            stream_->setBufferSizeInFrames(bufferSize);
        }
    }

    // EQ Control Functions
    void setEQBandEnabled(int bandIndex, bool enabled) {
        if (bandIndex >= 0 && bandIndex < 6) {
            eqBandEnabled[bandIndex] = enabled;
            ALOGI("EQ Band %d enabled: %s", bandIndex, enabled ? "true" : "false");
        }
    }
    
    void setEQBandFrequency(int bandIndex, float frequency) {
        if (bandIndex >= 0 && bandIndex < 6 && audioBuffer != nullptr) {
            eqFrequencies[bandIndex] = frequency;
            eqBands[bandIndex].setCoefficients(audioBuffer->getSampleRate(), frequency, eqQValues[bandIndex], eqGains[bandIndex]);
            ALOGI("EQ Band %d frequency: %.1f Hz", bandIndex, frequency);
        }
    }
    
    void setEQBandGain(int bandIndex, float gainDb) {
        if (bandIndex >= 0 && bandIndex < 6 && audioBuffer != nullptr) {
            eqGains[bandIndex] = gainDb;
            eqBands[bandIndex].setCoefficients(audioBuffer->getSampleRate(), eqFrequencies[bandIndex], eqQValues[bandIndex], gainDb);
            ALOGI("EQ Band %d gain: %.1f dB", bandIndex, gainDb);
        }
    }
    
    void setEQBandQ(int bandIndex, float q) {
        if (bandIndex >= 0 && bandIndex < 6 && audioBuffer != nullptr) {
            eqQValues[bandIndex] = q;
            eqBands[bandIndex].setCoefficients(audioBuffer->getSampleRate(), eqFrequencies[bandIndex], q, eqGains[bandIndex]);
            ALOGI("EQ Band %d Q: %.1f", bandIndex, q);
        }
    }
    
    // Compressor Control Functions
    void setCompressorThreshold(float thresholdDb) {
        compressorThreshold = thresholdDb;
        if (audioBuffer != nullptr) {
            compressor.setParameters(audioBuffer->getSampleRate(), thresholdDb, compressorRatio, 
                                   compressorAttack, compressorRelease, compressorMakeupGain);
        }
        ALOGI("Compressor threshold: %.1f dB", thresholdDb);
    }
    
    void setCompressorRatio(float ratio) {
        compressorRatio = ratio;
        if (audioBuffer != nullptr) {
            compressor.setParameters(audioBuffer->getSampleRate(), compressorThreshold, ratio, 
                                   compressorAttack, compressorRelease, compressorMakeupGain);
        }
        ALOGI("Compressor ratio: %.1f:1", ratio);
    }
    
    void setCompressorAttack(float attackMs) {
        compressorAttack = attackMs;
        if (audioBuffer != nullptr) {
            compressor.setParameters(audioBuffer->getSampleRate(), compressorThreshold, compressorRatio, 
                                   attackMs, compressorRelease, compressorMakeupGain);
        }
        ALOGI("Compressor attack: %.1f ms", attackMs);
    }
    
    void setCompressorRelease(float releaseMs) {
        compressorRelease = releaseMs;
        if (audioBuffer != nullptr) {
            compressor.setParameters(audioBuffer->getSampleRate(), compressorThreshold, compressorRatio, 
                                   compressorAttack, releaseMs, compressorMakeupGain);
        }
        ALOGI("Compressor release: %.1f ms", releaseMs);
    }
    
    // Leveler Control Functions
    void setLevelerTarget(float targetDb) {
        levelerTarget = targetDb;
        if (audioBuffer != nullptr) {
            leveler.setParameters(audioBuffer->getSampleRate(), targetDb, levelerSpeed);
        }
        ALOGI("Leveler target: %.1f dB", targetDb);
    }
    
    void setLevelerSpeed(float speed) {
        levelerSpeed = speed;
        if (audioBuffer != nullptr) {
            leveler.setParameters(audioBuffer->getSampleRate(), levelerTarget, speed);
        }
        ALOGI("Leveler speed: %.1fx", speed);
    }
    
    // Transient Shaper Control Functions
    void setTransientAttack(float attackDb) {
        transientAttack = attackDb;
        if (audioBuffer != nullptr) {
            transientShaper.setParameters(audioBuffer->getSampleRate(), attackDb, transientSustain);
        }
        ALOGI("Transient attack: %.1f dB", attackDb);
    }
    
    void setTransientSustain(float sustainDb) {
        transientSustain = sustainDb;
        if (audioBuffer != nullptr) {
            transientShaper.setParameters(audioBuffer->getSampleRate(), transientAttack, sustainDb);
        }
        ALOGI("Transient sustain: %.1f dB", sustainDb);
    }
    
    // Enable/disable functions
    void setCompressorEnabled(bool enabled) {
        compressorEnabled = enabled;
    }
    
    void setLevelerEnabled(bool enabled) {
        levelerEnabled = enabled;
    }
    
    void setTransientShaperEnabled(bool enabled) {
        transientShaperEnabled = enabled;
    }
    
    void setMasterVolume(float volume) {
        masterVolume = volume;
        ALOGI("Master volume: %.2f", volume);
    }

    oboe::Result start() {
        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Output);
        builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        builder.setSharingMode(oboe::SharingMode::Exclusive);
        builder.setFormat(oboe::AudioFormat::Float);
        builder.setChannelCount(oboe::ChannelCount::Stereo);
        builder.setCallback(static_cast<oboe::AudioStreamCallback*>(this));

        oboe::Result result = builder.openStream(&stream_);
        if (result != oboe::Result::OK) {
            ALOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
            return result;
        }

        // Log latency
        auto latencyResult = stream_->calculateLatencyMillis();
        if (latencyResult) {
            double latency = latencyResult.value();
            ALOGI("Latency: %f ms", latency);
        } else {
            ALOGE("Failed to calculate latency");
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
        ALOGI("setPlaying called with: %s", isPlaying ? "true" : "false");
        this->isPlaying = isPlaying;
        ALOGI("Playback set to: %s", isPlaying ? "PLAYING" : "STOPPED");
    }

    void setPlaybackPosition(int position) {
        playbackPosition = position;
        ALOGI("Playback position set to: %d", position);
    }

    int getPlaybackPosition() {
        return playbackPosition;
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override {
        if (isLiveMode.load()) {
            float *output = static_cast<float *>(audioData);
            for (int i = 0; i < numFrames * stream_->getChannelCount(); ++i) {
                float sample = 0.0f;
                circularBuffer.read(sample);
                output[i] = sample * masterVolume;
            }
            return oboe::DataCallbackResult::Continue;
        }

        // Fast exit if not playing to avoid unnecessary processing
        if (!isPlaying || audioBuffer == nullptr) {
            memset(audioData, 0, numFrames * oboeStream->getChannelCount() * sizeof(float));
            return oboe::DataCallbackResult::Continue;
        }

        float *output = static_cast<float *>(audioData);
        
        int frameCount = audioBuffer->getFrameCount();
        int channelCount = audioBuffer->getChannelCount();
        int outputChannels = oboeStream->getChannelCount();
        float* audioDataPtr = audioBuffer->getData();
        
        // Additional safety checks
        if (audioDataPtr == nullptr || frameCount <= 0 || channelCount <= 0 || outputChannels <= 0) {
            memset(audioData, 0, numFrames * outputChannels * sizeof(float));
            return oboe::DataCallbackResult::Continue;
        }
        
        int totalSamples = frameCount * channelCount;
        
        // Create temporary buffers for DSP processing
        float leftBuffer[numFrames];
        float rightBuffer[numFrames];
        
        // Fill buffers with audio data
        for (int i = 0; i < numFrames; ++i) {
            // Check if we've reached the end of the audio
            if (playbackPosition >= frameCount) {
                // Fill remaining frames with silence
                for (int j = i; j < numFrames; ++j) {
                    leftBuffer[j] = 0.0f;
                    rightBuffer[j] = 0.0f;
                }
                break;
            }
            
            if (channelCount == 1) {
                // Mono: duplicate to both channels
                if (playbackPosition < totalSamples) {
                    float sample = audioDataPtr[playbackPosition];
                    leftBuffer[i] = sample;
                    rightBuffer[i] = sample;
                } else {
                    leftBuffer[i] = 0.0f;
                    rightBuffer[i] = 0.0f;
                }
            } else if (channelCount == 2) {
                // Stereo: copy left and right channels with bounds checking
                int leftIndex = playbackPosition * 2;
                int rightIndex = playbackPosition * 2 + 1;
                if (rightIndex < totalSamples) {
                    leftBuffer[i] = audioDataPtr[leftIndex];
                    rightBuffer[i] = audioDataPtr[rightIndex];
                } else {
                    leftBuffer[i] = 0.0f;
                    rightBuffer[i] = 0.0f;
                }
            }
            playbackPosition++;
        }
        
        // Apply DSP processing to the buffers
        processDSP(leftBuffer, rightBuffer, numFrames);
        
        // Apply master volume and copy to output
        for (int i = 0; i < numFrames; ++i) {
            if (outputChannels == 1) {
                // Mix down to mono
                output[i] = (leftBuffer[i] + rightBuffer[i]) * 0.5f * masterVolume;
            } else if (outputChannels == 2) {
                // Stereo output
                output[i * 2] = leftBuffer[i] * masterVolume;        // Left
                output[i * 2 + 1] = rightBuffer[i] * masterVolume;   // Right
            }
        }
        
        return oboe::DataCallbackResult::Continue;
    }

private:
    void processDSP(float* left, float* right, int numFrames) {
        // Apply EQ bands
        for (int band = 0; band < 6; band++) {
            if (eqBandEnabled[band]) {
                eqBands[band].process(left, left, numFrames);
                eqBands[band].process(right, right, numFrames);
            }
        }
        
        // Apply compressor if enabled
        if (compressorEnabled) {
            // Calculate gain reduction
            float gainBuffer[numFrames];
            compressor.calculate_gain(left, gainBuffer, numFrames);
            compressor.apply_gain(left, gainBuffer, left, numFrames);
            
            compressor.calculate_gain(right, gainBuffer, numFrames);
            compressor.apply_gain(right, gainBuffer, right, numFrames);
        }
        
        // Apply leveler if enabled
        if (levelerEnabled) {
            leveler.process(left, left, numFrames);
            leveler.process(right, right, numFrames);
        }
        
        // Apply transient shaper if enabled
        if (transientShaperEnabled) {
            transientShaper.process(left, left, numFrames);
            transientShaper.process(right, right, numFrames);
        }
    }
    oboe::AudioStream *stream_ = nullptr;
    int32_t sampleRate_ = 0;
    int32_t bufferSize_ = 0;
    AudioBuffer *audioBuffer = nullptr;
    bool isPlaying = false;
    std::atomic<int> playbackPosition;
    CircularBuffer<float> circularBuffer;
    
    // DSP Components
    dsp::Biquad eqBands[6];
    dsp::Compressor compressor;
    dsp::Leveler leveler;
    dsp::TransientShaper transientShaper;
    
    // EQ Parameters
    bool eqBandEnabled[6] = {true, true, true, true, true, true};
    float eqFrequencies[6] = {80.0f, 200.0f, 800.0f, 2000.0f, 5000.0f, 10000.0f};
    float eqGains[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float eqQValues[6] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    
    // Compressor Parameters
    bool compressorEnabled = false;
    float compressorThreshold = -12.0f;
    float compressorRatio = 4.0f;
    float compressorAttack = 10.0f;
    float compressorRelease = 100.0f;
    float compressorMakeupGain = 0.0f;
    
    // Leveler Parameters
    bool levelerEnabled = false;
    float levelerTarget = -12.0f;
    float levelerSpeed = 1.0f;
    
    // Transient Shaper Parameters
    bool transientShaperEnabled = false;
    float transientAttack = 0.0f;
    float transientSustain = 0.0f;
    
    // Master Volume
    float masterVolume = 1.0f;

    // Live processing members
    std::atomic<bool> isLiveMode{false};
    LockFreeQueue<AudioChunk>* captureQueue = nullptr;
    std::thread liveProcessingThread;
};

static AudioEngine engine;

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1create(JNIEnv *env, jclass clazz) {
    ALOGI("JNI native_create called");
    // Initialize any required state here if needed
    ALOGI("JNI native_create completed");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setLiveMode(JNIEnv *env, jclass clazz, jboolean is_live, jlong queue_handle) {
    auto* queue = reinterpret_cast<LockFreeQueue<AudioChunk>*>(queue_handle);
    engine.setLiveMode(is_live, queue);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1start(JNIEnv *env, jclass clazz) {
    ALOGI("JNI native_start called");
    try {
        oboe::Result result = engine.start();
        if (result == oboe::Result::OK) {
            ALOGI("Audio engine started successfully");
        } else {
            ALOGE("Failed to start audio engine: %s", oboe::convertToText(result));
        }
    } catch (...) {
        ALOGE("Exception in JNI native_start");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1stop(JNIEnv *env, jclass clazz) {
    engine.stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setPlaying(JNIEnv *env, jclass clazz, jboolean is_playing) {
    ALOGI("JNI setPlaying called with: %s", is_playing ? "true" : "false");
    try {
        engine.setPlaying(is_playing);
        ALOGI("JNI setPlaying completed successfully");
    } catch (...) {
        ALOGE("Exception in JNI setPlaying");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setPlaybackPosition(JNIEnv *env, jclass clazz, jint position) {
    engine.setPlaybackPosition(position);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1getPlaybackPosition(JNIEnv *env, jclass clazz) {
    try {
        int position = engine.getPlaybackPosition();
        // Only log occasionally to reduce spam
        static int logCounter = 0;
        if (++logCounter % 50 == 0) {  // Log every 50th call
            ALOGI("JNI getPlaybackPosition returning: %d", position);
        }
        return position;
    } catch (...) {
        ALOGE("Exception in JNI getPlaybackPosition");
        return 0;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setAudioBuffer(JNIEnv *env, jclass clazz, jobject buffer) {
    if (buffer == nullptr) {
        ALOGE("AudioBuffer is null");
        return;
    }
    
    jclass bufferClass = env->GetObjectClass(buffer);
    if (bufferClass == nullptr) {
        ALOGE("Failed to get AudioBuffer class");
        return;
    }
    
    jmethodID getDataMethod = env->GetMethodID(bufferClass, "getData", "()[F");
    jmethodID getSampleRateMethod = env->GetMethodID(bufferClass, "getSampleRate", "()I");
    jmethodID getChannelCountMethod = env->GetMethodID(bufferClass, "getChannelCount", "()I");
    jmethodID getFrameCountMethod = env->GetMethodID(bufferClass, "getFrameCount", "()I");

    if (getDataMethod == nullptr || getSampleRateMethod == nullptr || 
        getChannelCountMethod == nullptr || getFrameCountMethod == nullptr) {
        ALOGE("Failed to get AudioBuffer methods");
        return;
    }

    jfloatArray dataArray = (jfloatArray) env->CallObjectMethod(buffer, getDataMethod);
    if (dataArray == nullptr) {
        ALOGE("AudioBuffer data array is null");
        return;
    }
    
    jint sampleRate = env->CallIntMethod(buffer, getSampleRateMethod);
    jint channelCount = env->CallIntMethod(buffer, getChannelCountMethod);
    jint frameCount = env->CallIntMethod(buffer, getFrameCountMethod);
    
    if (sampleRate <= 0 || channelCount <= 0 || frameCount <= 0) {
        ALOGE("Invalid audio parameters: sampleRate=%d, channelCount=%d, frameCount=%d", 
              sampleRate, channelCount, frameCount);
        return;
    }

    // Get the data and length
    jsize arrayLength = env->GetArrayLength(dataArray);
    if (arrayLength <= 0) {
        ALOGE("Audio data array is empty");
        return;
    }
    
    jfloat *data = env->GetFloatArrayElements(dataArray, nullptr);
    if (data == nullptr) {
        ALOGE("Failed to get float array elements");
        return;
    }

    ALOGI("Setting audio buffer: sampleRate=%d, channelCount=%d, frameCount=%d, arrayLength=%d", 
          sampleRate, channelCount, frameCount, arrayLength);

    // Copy the data to ensure it persists after JNI call
    float *dataCopy = new float[arrayLength];
    memcpy(dataCopy, data, arrayLength * sizeof(float));
    
    engine.setAudioBuffer(dataCopy, sampleRate, channelCount, frameCount);

    // Release the original data (we made our own copy)
    env->ReleaseFloatArrayElements(dataArray, data, JNI_ABORT);
    
    ALOGI("Audio buffer set successfully");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setBufferSize(JNIEnv *env, jclass clazz, jint buffer_size) {
    engine.setBufferSize(buffer_size);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setEQBandEnabled(JNIEnv *env, jclass clazz, jint bandIndex, jboolean enabled) {
    engine.setEQBandEnabled(bandIndex, enabled);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setEQBandFrequency(JNIEnv *env, jclass clazz, jint bandIndex, jfloat frequency) {
    engine.setEQBandFrequency(bandIndex, frequency);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setEQBandGain(JNIEnv *env, jclass clazz, jint bandIndex, jfloat gainDb) {
    engine.setEQBandGain(bandIndex, gainDb);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setEQBandQ(JNIEnv *env, jclass clazz, jint bandIndex, jfloat q) {
    engine.setEQBandQ(bandIndex, q);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setMasterVolume(JNIEnv *env, jclass clazz, jfloat volume) {
    engine.setMasterVolume(volume);
}

// Compressor Controls
extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setCompressorEnabled(JNIEnv *env, jclass clazz, jboolean enabled) {
    engine.setCompressorEnabled(enabled);
    ALOGI("Compressor enabled: %s", enabled ? "true" : "false");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setCompressorThreshold(JNIEnv *env, jclass clazz, jfloat thresholdDb) {
    engine.setCompressorThreshold(thresholdDb);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setCompressorRatio(JNIEnv *env, jclass clazz, jfloat ratio) {
    engine.setCompressorRatio(ratio);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setCompressorAttack(JNIEnv *env, jclass clazz, jfloat attackMs) {
    engine.setCompressorAttack(attackMs);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setCompressorRelease(JNIEnv *env, jclass clazz, jfloat releaseMs) {
    engine.setCompressorRelease(releaseMs);
}

// Leveler Controls
extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setLevelerEnabled(JNIEnv *env, jclass clazz, jboolean enabled) {
    engine.setLevelerEnabled(enabled);
    ALOGI("Leveler enabled: %s", enabled ? "true" : "false");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setLevelerTarget(JNIEnv *env, jclass clazz, jfloat targetDb) {
    engine.setLevelerTarget(targetDb);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setLevelerSpeed(JNIEnv *env, jclass clazz, jfloat speed) {
    engine.setLevelerSpeed(speed);
}

// Transient Shaper Controls
extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setTransientShaperEnabled(JNIEnv *env, jclass clazz, jboolean enabled) {
    engine.setTransientShaperEnabled(enabled);
    ALOGI("Transient Shaper enabled: %s", enabled ? "true" : "false");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setTransientAttack(JNIEnv *env, jclass clazz, jfloat attackDb) {
    engine.setTransientAttack(attackDb);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioEngine_native_1setTransientSustain(JNIEnv *env, jclass clazz, jfloat sustainDb) {
    engine.setTransientSustain(sustainDb);
}
double getLoudnessDb() {
    // TODO: Implement with proper libebur128 library
    // AudioBuffer* audioBuffer = engine.getAudioBuffer();
    // if (!audioBuffer) {
    //     return -70.0; // Return a default value if no audio is loaded
    // }
    // 
    // // This code is based on an assumed API for libebur128.
    // // It needs to be verified against the actual library documentation.
    // ebur128_state* st = ebur128_init(
    //     audioBuffer->getChannelCount(),
    //     audioBuffer->getSampleRate(),
    //     EBUR128_MODE_I
    // );
    
    ALOGI("getLoudnessDb() called - returning placeholder value");
    return -23.0; // Return a reasonable placeholder value

    // TODO: Enable when proper libebur128 is available
    // if (!st) {
    //     ALOGE("Failed to initialize libebur128");
    //     return -70.0;
    // }
    // 
    // ebur128_add_frames_float(st, audioBuffer->getData(), audioBuffer->getFrameCount());
    // 
    // double loudness = 0.0;
    // ebur128_loudness_global(st, &loudness);
    // 
    // ebur128_destroy(&st);
    // 
    // return loudness;
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
    // TODO: Implement MP3 export with proper LAME library
    ALOGI("MP3 export requested for %s - not yet implemented", path);
    ALOGE("MP3 export not available - LAME library not configured");
    return false;
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

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_audioapp_audio_AudioStreamQueue_native_1create(JNIEnv *env, jobject thiz, jint size) {
    return reinterpret_cast<jlong>(new LockFreeQueue<AudioChunk>(size));
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audioapp_audio_AudioStreamQueue_native_1destroy(JNIEnv *env, jobject thiz, jlong handle) {
    delete reinterpret_cast<LockFreeQueue<AudioChunk>*>(handle);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_audioapp_audio_AudioStreamQueue_native_1push(JNIEnv *env, jobject thiz, jlong handle, jbyteArray data, jint size) {
    auto* queue = reinterpret_cast<LockFreeQueue<AudioChunk>*>(handle);
    if (!queue) {
        return false;
    }
    jbyte* elements = env->GetByteArrayElements(data, nullptr);
    if (!elements) {
        return false;
    }
    AudioChunk chunk(size);
    memcpy(chunk.data(), elements, size);
    bool result = queue->push(chunk);
    env->ReleaseByteArrayElements(data, elements, JNI_ABORT);
    return result;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_audioapp_audio_AudioStreamQueue_native_1pop(JNIEnv *env, jobject thiz, jlong handle, jbyteArray data) {
    auto* queue = reinterpret_cast<LockFreeQueue<AudioChunk>*>(handle);
    if (!queue) {
        return -1;
    }
    AudioChunk chunk;
    if (queue->pop(chunk)) {
        jsize len = env->GetArrayLength(data);
        if (len < chunk.size()) {
            // Buffer provided by Java is too small
            return -2;
        }
        env->SetByteArrayRegion(data, 0, chunk.size(), reinterpret_cast<const jbyte*>(chunk.data()));
        return chunk.size();
    }
    return 0; // Queue was empty
}
