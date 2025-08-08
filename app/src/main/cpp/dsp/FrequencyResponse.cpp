#include "FrequencyResponse.h"
#include "FFT.h"
#include <cmath>
#include <complex>

namespace dsp {

FrequencyResponse::FrequencyResponse(int num_bands, double sample_rate) :
    num_bands(num_bands),
    sample_rate(sample_rate),
    fft_size(2048),
    cache_dirty(true)
{
    impulse.resize(fft_size, 0.0f);
    impulse[0] = 1.0f;
    impulse_response.resize(fft_size);
    magnitudes_cache.resize(fft_size / 2 + 1);
}

void FrequencyResponse::invalidate() {
    cache_dirty = true;
}

void FrequencyResponse::calculateResponse(const Biquad* filters, int num_filters, std::vector<float>& magnitudes) {
    if (!cache_dirty) {
        magnitudes = magnitudes_cache;
        return;
    }

    // 1. Process impulse through filters
    std::fill(impulse_response.begin(), impulse_response.end(), 0.0f);
    for (int i = 0; i < fft_size; ++i) {
        float sample = impulse[i];
        for (int j = 0; j < num_filters; ++j) {
            // Create a temporary copy of the filter to not modify its state
            Biquad temp_filter = filters[j];
            sample = temp_filter.process(sample);
        }
        impulse_response[i] = sample;
    }

    // 2. Prepare for FFT
    std::vector<std::complex<double>> fft_buffer(fft_size);
    for (int i = 0; i < fft_size; ++i) {
        fft_buffer[i] = {impulse_response[i], 0.0};
    }

    // 3. Perform FFT
    fft::fft(fft_buffer);

    // 4. Calculate magnitudes
    magnitudes.resize(fft_size / 2 + 1);
    for (int i = 0; i < fft_size / 2 + 1; ++i) {
        magnitudes[i] = std::abs(fft_buffer[i]);
    }

    // 5. Smooth the response
    smooth(magnitudes);

    // 6. Update cache
    magnitudes_cache = magnitudes;
    cache_dirty = false;
}

void FrequencyResponse::smooth(std::vector<float>& magnitudes) {
    if (magnitudes.size() < 3) {
        return;
    }
    std::vector<float> smoothed = magnitudes;
    for (size_t i = 1; i < magnitudes.size() - 1; ++i) {
        smoothed[i] = (magnitudes[i-1] + magnitudes[i] + magnitudes[i+1]) / 3.0f;
    }
    magnitudes = smoothed;
}

} // namespace dsp
