#include "TransientShaper.h"
#include <algorithm>

namespace dsp {

TransientShaper::TransientShaper() :
    sampleRate(44100.0),
    attack_gain(1.0f),
    sustain_gain(1.0f),
    fast_envelope(0.0),
    slow_envelope(0.0),
    fast_attack_coeff(0.0),
    fast_release_coeff(0.0),
    slow_coeff(0.0)
{
}

void TransientShaper::setParameters(double sampleRate, float attackDb, float sustainDb) {
    this->sampleRate = sampleRate;
    this->attack_gain = pow(10, attackDb / 20.0);
    this->sustain_gain = pow(10, sustainDb / 20.0);

    // Coefficients for envelope followers
    // These values would typically be tuned for the desired response
    fast_attack_coeff = exp(-1.0 / (0.001 * sampleRate)); // 1ms attack
    fast_release_coeff = exp(-1.0 / (0.050 * sampleRate)); // 50ms release
    slow_coeff = exp(-1.0 / (0.100 * sampleRate)); // 100ms
}

void TransientShaper::process(const float* in, float* out, int num_samples) {
    for (int i = 0; i < num_samples; ++i) {
        double input_abs = std::abs(in[i]);

        // Fast envelope follower
        if (input_abs > fast_envelope) {
            fast_envelope = fast_attack_coeff * fast_envelope + (1 - fast_attack_coeff) * input_abs;
        } else {
            fast_envelope = fast_release_coeff * fast_envelope + (1 - fast_release_coeff) * input_abs;
        }

        // Slow envelope follower
        slow_envelope = slow_coeff * slow_envelope + (1 - slow_coeff) * input_abs;

        // Transient detection and gain shaping
        double gain = 1.0;
        if (slow_envelope > 1e-6) {
            double ratio = fast_envelope / slow_envelope;
            if (ratio > 1.0) { // Transient
                gain = attack_gain;
            } else { // Sustain
                gain = sustain_gain;
            }
        }

        // For simplicity, we apply the gain directly. A real implementation
        // would smooth the gain to avoid artifacts.
        out[i] = in[i] * gain;
    }
}

void TransientShaper::reset() {
    fast_envelope = 0.0;
    slow_envelope = 0.0;
}

} // namespace dsp
