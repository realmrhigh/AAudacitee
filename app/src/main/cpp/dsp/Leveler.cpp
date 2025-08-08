#include "Leveler.h"
#include <algorithm>

namespace dsp {

Leveler::Leveler() :
    sampleRate(44100.0),
    targetLevel(1.0), // 0 dB
    speedCoeff(0.0),
    rms_envelope(0.0),
    gainDb(0.0f)
{
}

void Leveler::setParameters(double sampleRate, double targetLevelDb, double speed) {
    this->sampleRate = sampleRate;
    this->targetLevel = pow(10, targetLevelDb / 20.0);

    // Map speed (0-1) to a time constant (e.g., 50ms to 5000ms)
    double timeMs = 50.0 + (1.0 - speed) * 4950.0;
    this->speedCoeff = exp(-1.0 / (timeMs * sampleRate * 0.001));
}

void Leveler::process(const float* in, float* out, int num_samples) {
    for (int i = 0; i < num_samples; ++i) {
        // RMS detection
        double input_squared = in[i] * in[i];
        rms_envelope = speedCoeff * rms_envelope + (1 - speedCoeff) * input_squared;

        double current_level = sqrt(rms_envelope);

        // Gain computation
        double target_gain = 1.0;
        if (current_level > 1e-6) { // Avoid division by zero
            target_gain = targetLevel / current_level;
        }

        // Smooth the gain
        double current_gain_linear = pow(10, gainDb / 20.0);
        current_gain_linear = speedCoeff * current_gain_linear + (1 - speedCoeff) * target_gain;

        gainDb = 20 * log10(current_gain_linear);
        out[i] = in[i] * current_gain_linear;
    }
}

float Leveler::getGainDb() const {
    return gainDb;
}

void Leveler::reset() {
    rms_envelope = 0.0;
    gainDb = 0.0f;
}

} // namespace dsp
