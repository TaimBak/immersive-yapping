# Immersive Yapping - A Spectral Convolution Plugin for Proximity Chat

A real-time convolution reverb system built in C++ with JUCE, targeting both DAW and game engine environments. The plugin applies environment-specific impulse response (IR) reverb to audio — designed for use cases like VOIP proximity chat in 3D games where traditional baked-in reverb falls short.

## Features

- **Frequency-domain convolution** using partitioned Overlap-Add with FFT (O(N log N))
- **Time-domain convolution** via circular delay buffer (O(N²), used as a verification baseline)
- Real-time algorithm switching and IR selection
- Embedded impulse responses (amphitheater, bedroom, and more)
- Dry/wet mix control with DAW automation support (APVTS)
- Builds as **VST3**, **Audio Unit**, and **Standalone** via CMake
- Thread-safe audio processing (spin lock with try-lock pattern)
- Denormalized float suppression for x86 performance

## Building

Requires CMake and a C++17 compiler. JUCE is included as a dependency.

```bash
cmake -B build
cmake --build build
```

## Unreal Engine Integration

The convolution DSP is also being ported into Unreal Engine 5 as a native **Submix Effect** plugin. In the Unreal demo, different impulse responses are triggered based on the player's location using Audio Volume actors — walking between rooms produces distinct, environment-appropriate reverb.

## Algorithm

The frequency-domain convolver implements Frank Wefers's Overlap-Add scheme for partitioned convolution. Input blocks (B=512) are zero-padded to FFT size K=1024, transformed, multiplied with the pre-computed IR spectrum, and inverse-transformed. Overlap tails are accumulated across blocks for seamless reconstruction. The constraint K >= B + N - 1 prevents time-domain aliasing.

## References

- Wefers, F. — [Partitioned Convolution Algorithms for Real-Time Auralization](https://publications.rwth-aachen.de/record/466561/files/466561.pdf)
- Steiglitz, K. (2020) — *A Digital Signal Processing Primer*
- Hollemans, M. (2024) — *The Complete Beginner's Guide to Audio Plug-in Development*
- [Unreal Engine 5 Audio Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/)
