#include "FreqDomainConvolver.h"

FreqDomainConvolver::FreqDomainConvolver(const std::vector<float> &h,
                                         int fftOrder = 10, int blockSize = 128)
    : fftOrder(fftOrder), K(1 << fftOrder), B(blockSize), N((int)h.size()),
      fft(fftOrder), Hspec(K), Xspec(K), Yspec(K), timeC(K), timeK(K, 0.0f)
{
  jassert(N > 0);
  jassert(K >= B + N - 1); // OLA no-aliasing requirement

  overlap.assign((size_t)(N - 1), 0.0f);
  newOverlap.assign((size_t)(N - 1), 0.0f);

  // Precompute H(k) = FFT{ h padded to K }
  for (int n = 0; n < K; ++n)
    timeC[n] = (n < N ? juce::dsp::Complex<float>(h[n], 0.0f)
                      : juce::dsp::Complex<float>(0.0f, 0.0f));

  fft.perform(timeC.data(), Hspec.data(), false); // forward

  double Henergy = 0.0;

  for (int k = 0; k < K; ++k)
    Henergy += std::norm(Hspec[k]);

  jassert(Henergy > 0.0); // IR actually made it into the spectrum
}

void FreqDomainConvolver::reset()
{
  std::fill(overlap.begin(), overlap.end(), 0.0f);
  overlap.assign((size_t)(N - 1), 0.0f);
}

void FreqDomainConvolver::processBlock(const float *in, float *out,
                                       int numSamples)
{
  // Ensure B is valid size (not smaller)
  jassert(numSamples > 0 && numSamples <= B);

  // 1. TD -> FD (FFT)
  for (int n = 0; n < K; ++n)
    timeC[n] = (n < numSamples ? C(in[n], 0.0f) : C(0.0f, 0.0f));

  fft.perform(timeC.data(), Xspec.data(), false);

  // 2. Multiply w/ IR
  for (int k = 0; k < K; ++k)
    Yspec[k] = Xspec[k] * Hspec[k]; // <- Hspec done during concolver init

  // 3. FD -> TD (IFFT)
  fft.perform(Yspec.data(), timeC.data(), true);

  // Split output into two segments
  const int valid = numSamples + N - 1; // sample <= K
  const int head = numSamples;          // samples to output now
  const int tail = valid - head;        // samples to carry
  jassert(valid <= K && tail >= 0 && tail <= (N - 1));

  // Ensure overlap is exactly N-1 long (clipping prevention)
  if ((int)overlap.size() != N - 1)
    overlap.assign((size_t)(N - 1), 0.0f);

  // 4. Build output head = current head + previous overlap
  for (int n = 0; n < head; ++n)
  {
    const float cur = timeC[n].real();
    const float prv = (n < (int)overlap.size()) ? overlap[(size_t)n] : 0.0f;
    out[n] = cur + prv;
  }

  // 5. Build new overlap => shifted old overlap tail + current tail
  std::fill(newOverlap.begin(), newOverlap.end(), 0.0f);

  // Copy shifted remainder of previous overlap that wasn't emitted due to B
  // being smaller than N - 1
  for (int t = 0; t < N - 1 - head; ++t) // only if head < N - 1
    newOverlap[(size_t)t] = overlap[(size_t)(t + head)];

  // Sum with current block's tail
  for (int t = 0; t < tail; ++t)
    newOverlap[(size_t)t] += timeC[head + t].real();

  overlap.swap(newOverlap);
}
