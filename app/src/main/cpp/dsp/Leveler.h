#ifndef AUDIOAPP_LEVELER_H
#define AUDIOAPP_LEVELER_H

#include <cmath>

namespace dsp {

class Leveler {
public:
    Leveler();
    ~Leveler() = default;

    void setParameters(double sampleRate, double targetLevelDb, double speed);
    void process(const float* in, float* out, int num_samples);
    float getGainDb() const;
    void reset();

private:
    double sampleRate;
    double targetLevel; // Linear
    double speedCoeff;  // Coefficient for RMS envelope follower

    double rms_envelope;
    float gainDb;
};

} // namespace dsp

#endif //AUDIOAPP_LEVELER_H
