#ifndef AUDIOAPP_TRANSIENTSHAPER_H
#define AUDIOAPP_TRANSIENTSHAPER_H

#include <cmath>

namespace dsp {

class TransientShaper {
public:
    TransientShaper();
    ~TransientShaper() = default;

    void setParameters(double sampleRate, float attack, float sustain);
    void process(const float* in, float* out, int num_samples);
    void reset();

private:
    double sampleRate;
    float attack_gain;
    float sustain_gain;

    // Envelope followers
    double fast_envelope;
    double slow_envelope;

    // Coefficients
    double fast_attack_coeff;
    double fast_release_coeff;
    double slow_coeff;
};

} // namespace dsp

#endif //AUDIOAPP_TRANSIENTSHAPER_H
