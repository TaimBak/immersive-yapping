#pragma once

#include <stdexcept>
#include <vector>

class TimeDomainConvolver
{
public:
  TimeDomainConvolver(const std::vector<float> &ir);
  void reset();
  float processSample(float x);
  void processBlock(const float *in, float *out, std::size_t numSamples);

private:
  std::vector<float> ir;          // Impulse response coefficients
  std::size_t irSize;             // Length of IR
  std::vector<float> delayBuffer; // Circular buffer for input history
  std::size_t writeIndex;         // Current write position in circular buffer
};
