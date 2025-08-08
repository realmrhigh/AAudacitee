#ifndef AUDIOAPP_BIQUAD_H
#define AUDIOAPP_BIQUAD_H

#include <cmath>

#if defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

namespace dsp {

enum class BiquadFilterType {
    LOWPASS,
    HIGHPASS,
    PEAK,
    LOW_SHELF,
    HIGH_SHELF
};

class Biquad {
public:
    Biquad();
    ~Biquad() = default;

    void setType(BiquadFilterType type);
    void setCoefficients(double sampleRate, double frequency, double q, double gain);
    void process(float* in, float* out, int num_samples);
    void reset();

private:
    void calculateCoefficients();

    BiquadFilterType type;
    double sampleRate;
    double frequency;
    double q;
    double gain;

    // Coefficients
    double a0, a1, a2, b0, b1, b2;

    // State variables
    double x1, x2, y1, y2;
};

} // namespace dsp

#endif //AUDIOAPP_BIQUAD_H
