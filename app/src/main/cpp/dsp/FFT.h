#ifndef AUDIOAPP_FFT_H
#define AUDIOAPP_FFT_H

#include <vector>
#include <complex>

namespace dsp {
namespace fft {

void fft(std::vector<std::complex<double>>& x);

} // namespace fft
} // namespace dsp

#endif //AUDIOAPP_FFT_H
