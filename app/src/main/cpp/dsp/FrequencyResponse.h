#ifndef AUDIOAPP_FREQUENCYRESPONSE_H
#define AUDIOAPP_FREQUENCYRESPONSE_H

#include "Biquad.h"
#include <vector>

namespace dsp {

class FrequencyResponse {
public:
    FrequencyResponse(int num_bands, double sample_rate);
    ~FrequencyResponse() = default;

    void calculateResponse(const Biquad* filters, int num_filters, std::vector<float>& magnitudes);
    void invalidate();

private:
    void smooth(std::vector<float>& magnitudes);

    bool cache_dirty;
    std::vector<float> magnitudes_cache;
    int num_bands;
    double sample_rate;
    int fft_size;
    std::vector<float> impulse;
    std::vector<float> impulse_response;
};

} // namespace dsp

#endif //AUDIOAPP_FREQUENCYRESPONSE_H
