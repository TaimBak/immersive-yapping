#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <stdexcept>
#include <vector>

class FreqDomainConvolver
{
public:
  // fftOrder=10 -> K=1024, blockSize=128 by your spec
  FreqDomainConvolver(const std::vector<float> &h, int fftOrder, int blockSize);

  void reset();

  // Process one block, writing output to pre-allocated buffer
  void processBlock(const float *in, float *out, int numSamples);

  int getFFTSize() const { return K; }
  int getBlockSize() const { return B; }
  int getIRLength() const { return N; }

private:
  const int fftOrder; // dsp::fft requirement
  const int K;        // FFT size
  const int B;        // processing block size
  const int N;        // IR length

  juce::dsp::FFT fft;

  // Frequency-domain working buffers (interleaved complex: 2*K floats)
  using C = juce::dsp::Complex<float>;
  std::vector<C> Hspec, Xspec, Yspec, timeC;

  // Time-domain working buffer (K real samples) and carry buffer (K)
  std::vector<float> timeK;
  std::vector<float> overlap;
  std::vector<float> newOverlap; // pre-allocated scratch for OLA
};
