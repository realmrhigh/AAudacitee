#ifndef AUDIOAPP_COMPRESSOR_H
#define AUDIOAPP_COMPRESSOR_H

#include <cmath>
#include <vector>

namespace dsp {

enum class CompressorDetectionMode {
    PEAK,
    RMS
};

class Compressor {
public:
    Compressor();
    ~Compressor() = default;

    void setParameters(double sampleRate, double thresholdDb, double ratio, double attackMs, double releaseMs, double makeupGainDb);
    void setDetectionMode(CompressorDetectionMode mode);
    void calculate_gain(const float* sidechain, float* gain_buffer, int num_samples);
    void apply_gain(const float* in, const float* gain_buffer, float* out, int num_samples);
    float getGainReductionDb() const;
    void reset();

private:
    void calculateCoefficients();

    double sampleRate;
    double threshold; // Linear
    double ratio;
    double attack;    // Coefficient
    double release;   // Coefficient
    double makeupGain; // Linear

    CompressorDetectionMode detectionMode;
    double envelope;
    double gain_db;
    float gainReductionDb;
};

} // namespace dsp

#endif //AUDIOAPP_COMPRESSOR_H
