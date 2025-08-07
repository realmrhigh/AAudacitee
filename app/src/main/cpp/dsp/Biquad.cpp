#include "Biquad.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dsp {

Biquad::Biquad() :
    type(BiquadFilterType::PEAK),
    sampleRate(44100.0),
    frequency(1000.0),
    q(0.707),
    gain(0.0),
    a0(1.0), a1(0.0), a2(0.0), b0(1.0), b1(0.0), b2(0.0),
    x1(0.0), x2(0.0), y1(0.0), y2(0.0)
{
    calculateCoefficients();
}

void Biquad::setType(BiquadFilterType type) {
    this->type = type;
    calculateCoefficients();
}

void Biquad::setCoefficients(double sampleRate, double frequency, double q, double gain) {
    this->sampleRate = sampleRate;
    this->frequency = frequency;
    this->q = q;
    this->gain = gain;
    calculateCoefficients();
}

void Biquad::calculateCoefficients() {
    double norm;
    double V = pow(10, fabs(gain) / 20.0);
    double K = tan(M_PI * frequency / sampleRate);
    double K_squared = K * K;

    switch (type) {
        case BiquadFilterType::LOWPASS:
            norm = 1 / (1 + K / q + K_squared);
            b0 = K_squared * norm;
            b1 = 2 * b0;
            b2 = b0;
            a1 = 2 * (K_squared - 1) * norm;
            a2 = (1 - K / q + K_squared) * norm;
            break;

        case BiquadFilterType::HIGHPASS:
            norm = 1 / (1 + K / q + K_squared);
            b0 = 1 * norm;
            b1 = -2 * b0;
            b2 = b0;
            a1 = 2 * (K_squared - 1) * norm;
            a2 = (1 - K / q + K_squared) * norm;
            break;

        case BiquadFilterType::PEAK:
            if (gain >= 0) { // Boost
                norm = 1 / (1 + 1/q * K + K_squared);
                b0 = (1 + V/q * K + K_squared) * norm;
                b1 = 2 * (K_squared - 1) * norm;
                b2 = (1 - V/q * K + K_squared) * norm;
                a1 = b1;
                a2 = (1 - 1/q * K + K_squared) * norm;
            } else { // Cut
                norm = 1 / (1 + V/q * K + K_squared);
                b0 = (1 + 1/q * K + K_squared) * norm;
                b1 = 2 * (K_squared - 1) * norm;
                b2 = (1 - 1/q * K + K_squared) * norm;
                a1 = b1;
                a2 = (1 - V/q * K + K_squared) * norm;
            }
            break;

        case BiquadFilterType::LOW_SHELF:
            if (gain >= 0) { // Boost
                norm = 1 / (1 + sqrt(2) * K + K_squared);
                b0 = (1 + sqrt(2*V) * K + V * K_squared) * norm;
                b1 = 2 * (V * K_squared - 1) * norm;
                b2 = (1 - sqrt(2*V) * K + V * K_squared) * norm;
                a1 = 2 * (K_squared - 1) * norm;
                a2 = (1 - sqrt(2) * K + K_squared) * norm;
            } else { // Cut
                norm = 1 / (1 + sqrt(2*V) * K + V * K_squared);
                b0 = (1 + sqrt(2) * K + K_squared) * norm;
                b1 = 2 * (K_squared - 1) * norm;
                b2 = (1 - sqrt(2) * K + K_squared) * norm;
                a1 = 2 * (V * K_squared - 1) * norm;
                a2 = (1 - sqrt(2*V) * K + V * K_squared) * norm;
            }
            break;

        case BiquadFilterType::HIGH_SHELF:
            if (gain >= 0) { // Boost
                norm = 1 / (1 + sqrt(2) * K + K_squared);
                b0 = (V + sqrt(2*V) * K + K_squared) * norm;
                b1 = 2 * (K_squared - V) * norm;
                b2 = (V - sqrt(2*V) * K + K_squared) * norm;
                a1 = 2 * (K_squared - 1) * norm;
                a2 = (1 - sqrt(2) * K + K_squared) * norm;
            } else { // Cut
                norm = 1 / (V + sqrt(2*V) * K + K_squared);
                b0 = (1 + sqrt(2) * K + K_squared) * norm;
                b1 = 2 * (K_squared - 1) * norm;
                b2 = (1 - sqrt(2) * K + K_squared) * norm;
                a1 = 2 * ((K_squared/V) - 1) * norm;
                a2 = (1 - (sqrt(2)/sqrt(V)) * K + (K_squared/V)) * norm;
            }
            break;
    }
}

float Biquad::process(float in) {
    double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = in;
    y2 = y1;
    y1 = out;
    return out;
}

void Biquad::reset() {
    x1 = x2 = y1 = y2 = 0.0;
}

} // namespace dsp
