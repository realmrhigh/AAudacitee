#include "Compressor.h"
#include <algorithm>

namespace dsp {

Compressor::Compressor() :
    sampleRate(44100.0),
    threshold(1.0), // 0 dB
    ratio(1.0),
    attack(0.0),
    release(0.0),
    makeupGain(1.0),
    detectionMode(CompressorDetectionMode::PEAK),
    envelope(0.0),
    gain_db(0.0),
    gainReductionDb(0.0f)
{
}

void Compressor::setParameters(double sampleRate, double thresholdDb, double ratio, double attackMs, double releaseMs, double makeupGainDb) {
    this->sampleRate = sampleRate;
    this->threshold = pow(10, thresholdDb / 20.0);
    this->ratio = ratio;
    this->makeupGain = pow(10, makeupGainDb / 20.0);

    // Coefficients are calculated based on time constants
    this->attack = exp(-1.0 / (attackMs * sampleRate * 0.001));
    this->release = exp(-1.0 / (releaseMs * sampleRate * 0.001));
}

void Compressor::setDetectionMode(CompressorDetectionMode mode) {
    this->detectionMode = mode;
}

void Compressor::calculate_gain(const float* sidechain, float* gain_buffer, int num_samples) {
    for (int i = 0; i < num_samples; ++i) {
        double input_level;
        if (detectionMode == CompressorDetectionMode::PEAK) {
            input_level = std::abs(sidechain[i]);
        } else { // RMS
            double input_squared = sidechain[i] * sidechain[i];
            envelope = release * envelope + (1 - release) * input_squared;
            input_level = sqrt(envelope);
        }

        double target_gain_db = 0.0;
        if (input_level > threshold) {
            double input_level_db = 20 * log10(input_level);
            double threshold_db = 20 * log10(threshold);
            target_gain_db = (1.0 / ratio - 1.0) * (input_level_db - threshold_db);
        }

        if (target_gain_db < gain_db) { // Attack
            gain_db = attack * gain_db + (1 - attack) * target_gain_db;
        } else { // Release
            gain_db = release * gain_db + (1 - release) * target_gain_db;
        }

        gain_buffer[i] = pow(10, gain_db / 20.0);
        gainReductionDb = gain_db;
    }
}

void Compressor::apply_gain(const float* in, const float* gain_buffer, float* out, int num_samples) {
    for (int i = 0; i < num_samples; ++i) {
        out[i] = in[i] * gain_buffer[i] * makeupGain;
    }
}

float Compressor::getGainReductionDb() const {
    return gainReductionDb;
}

void Compressor::reset() {
    envelope = 0.0;
    gain_db = 0.0;
    gainReductionDb = 0.0f;
}

void Compressor::calculateCoefficients() {
    // This is now handled in setParameters
}

} // namespace dsp
