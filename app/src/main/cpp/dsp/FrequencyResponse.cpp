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
    fft_buffer.resize(fft_size);
    smoothing_buffer.resize(fft_size / 2 + 1);
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
    std::vector<Biquad> temp_filters;
    for(int i = 0; i < num_filters; ++i) {
        temp_filters.push_back(filters[i]);
        temp_filters.back().reset();
    }

    std::vector<float> temp_buf1 = impulse;
    std::vector<float> temp_buf2(fft_size);

    float* in_ptr = temp_buf1.data();
    float* out_ptr = temp_buf2.data();

    for (int j = 0; j < num_filters; ++j) {
        temp_filters[j].process(in_ptr, out_ptr, fft_size);
        std::swap(in_ptr, out_ptr);
    }

    // The final result is in in_ptr
    memcpy(impulse_response.data(), in_ptr, fft_size * sizeof(float));

    // 2. Prepare for FFT
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
    smoothing_buffer = magnitudes;
    for (size_t i = 1; i < magnitudes.size() - 1; ++i) {
        magnitudes[i] = (smoothing_buffer[i-1] + smoothing_buffer[i] + smoothing_buffer[i+1]) / 3.0f;
    }
}

} // namespace dsp
