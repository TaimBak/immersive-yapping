Austin Clark
MUS-471-SP26

Project Timeline and Deliverables

## Part 1: Implementation Changes, Additions, and Deletions

### Summary of Work Completed in MUS 470

The fall semester produced a fully functional real-time convolution reverb plugin built in C++ with the JUCE framework. Both the time-domain convolution algorithm (circular delay buffer, O(N^2)) and the frequency-domain convolution algorithm (Overlap-Add with FFT, O(N log N)) were implemented, tested, and verified against known outputs. The plugin builds as a VST3, Audio Unit, and standalone application via CMake. Two impulse responses — an amphitheater and a bedroom — are embedded as binary data in the plugin. The GUI supports real-time algorithm switching and IR selection, and the plugin has been tested processing live audio in Reaper. Thread safety on the audio thread is handled with a spin lock using a try-lock pattern, and denormalized floating-point values are suppressed to avoid performance penalties on x86.

The core algorithmic and audio engine work outlined in the original MUS 470 spec is complete. The spring semester will focus on integration with Unreal Engine, expanding the plugin's user interface, and additional polish.

### Changes in Software API

**Addition: Unreal Engine Audio API.** The plugin's convolution DSP will be ported into Unreal Engine as a native Submix Effect plugin. This requires interfacing with Unreal's audio plugin API (FSoundEffectSubmix / USoundEffectSubmixPreset) and Unreal's build system (Unreal Build Tool with .Build.cs files). The core DSP logic — the Overlap-Add convolution, FFT, overlap buffer management, and complex multiplication — will be extracted from the JUCE project and compiled within the Unreal plugin module. The JUCE FFT dependency will either be linked as a standalone module or replaced with a lightweight FFT library compatible with Unreal's build system (candidates include KissFFT, which Unreal already bundles, or pffft).

**Addition: AudioProcessorValueTreeState (APVTS).** The JUCE plugin currently uses manual parameter handling. The dry/wet mix parameter is hardcoded to 1.0 and not exposed in the GUI. This will be replaced with JUCE's APVTS system to properly expose automatable parameters (dry/wet mix, algorithm selection, IR selection) with thread-safe, host-compatible parameter management.

### Added Implementation Details

**Unreal Engine Integration.** A multi-room Unreal Engine level will be constructed using a basic template with a playable first-person character. The convolution reverb will be applied via Unreal's Submix Effect system, and different impulse responses will be triggered based on the player's location using Audio Volume actors. When the player enters a defined zone (e.g., a hallway, a large open room, a small enclosed space), the active IR will switch to match that environment. This demonstrates the plugin's primary use case: applying environment-specific reverb to dynamic audio sources in a 3D game.

**Additional Impulse Responses.** The current set of two embedded IRs (amphitheater, bedroom) will be expanded with additional recordings captured from distinct acoustic spaces to make the Unreal demo more compelling. These may be sourced from freely available IR libraries or recorded using sound lab equipment. The goal is to have at least 4-5 distinct IRs representing varied environments (e.g., hallway, bathroom, cathedral, outdoor space).

**GUI Improvements.** The plugin editor will be expanded to include a dry/wet mix slider, backed by APVTS for proper DAW automation support. The current toggle button and combo box will remain for algorithm selection and IR selection. The layout will be cleaned up for a more polished presentation during the final demo.

### Goals Moved to Stretch / Deprioritized

**GPU Audio API integration — moved to stretch goal.** The original spec identified GPU acceleration as an "if time permits" goal, and it remains in that position. After evaluating the remaining time and the scope of Unreal Engine integration, GPU offloading is not feasible as a primary deliverable. Integrating the GPU Audio API requires significant work in GPU scheduling, latency management, and debugging that would compete directly with the Unreal integration effort for the limited remaining weeks. If Unreal integration is completed well ahead of schedule, GPU acceleration may be explored, but it is not expected to be delivered.

**Reason:** The Unreal Engine integration is the higher-value deliverable for demonstrating the plugin's stated purpose (real-time reverb for 3D game environments). GPU acceleration is an optimization that does not change the plugin's functionality, whereas Unreal integration directly fulfills the project's core goal.

### Deleted Goals

**Unity integration — removed.** The original spec mentioned both Unity and Unreal as target game engines. Unity integration has been removed from scope entirely. Demonstrating the plugin in a single game engine (Unreal) is sufficient to validate the approach, and splitting effort across two engines with different audio APIs would dilute the quality of both integrations without adding meaningful academic value.

**Multichannel and ambisonics support — removed.** These were mentioned as future extensions in the original spec and are not being pursued. They would require substantial additional DSP work (multichannel IR handling, spatial encoding/decoding) that falls outside the core scope of the project. The plugin operates in stereo, which is appropriate for the VOIP/proximity chat use case described in the original motivation.

## Part 2: Timeline of Milestones and Deliverables

### Week 1 — March 3 to March 7: Plugin Parameter System and Unreal Research

- Implement APVTS in the JUCE plugin to expose dry/wet mix as an automatable parameter with a GUI slider.
- Wire algorithm selection and IR selection through APVTS for consistent parameter management.
- Test updated plugin in Reaper to verify parameter automation and state recall.
- Begin research on Unreal Engine's Submix Effect plugin API: study FSoundEffectSubmix, USoundEffectSubmixPreset, and Unreal's existing FSubmixEffectConvolutionReverb source code for reference.
- Evaluate FFT library options for the Unreal build (KissFFT vs. linking JUCE DSP module vs. pffft).
- Milestone: Dry/wet mix slider functional and automatable in Reaper.

### Week 2 — March 10 to March 14: Unreal Engine Project Setup and Submix Effect Scaffolding

- Create an Unreal Engine 5 project using a basic first-person template.
- Set up a C++ plugin module within the Unreal project for the convolution reverb Submix Effect.
- Implement the bare Submix Effect boilerplate (FSoundEffectSubmix subclass) that passes audio through unmodified.
- Milestone: Audio passes through the custom Submix Effect in the Unreal editor without artifacts.

### Week 3 — March 17 to March 21: DSP Porting and Convolution in Unreal

- Port the FreqDomainConvolver class into the Unreal plugin module, resolving FFT library dependencies.
- Integrate the frequency-domain convolver into the Submix Effect's ProcessAudio callback with a single hardcoded IR.
- Handle sample rate and block size differences between the JUCE plugin configuration and Unreal's audio device settings.
- Test real-time convolution on in-game audio sources (footsteps, ambient sounds, or voice).
- Milestone: Convolution reverb is audible and functioning on audio within the Unreal Engine editor.

### Week 4 — March 24 to March 28: Multi-Room Level Design and IR Sourcing

- Build out the Unreal level with 3-4 distinct rooms/zones of varying size and acoustic character.
- Source or record additional impulse responses (target: 4-5 total) representing varied environments (e.g., hallway, bathroom, cathedral, outdoor space).
- Embed the new IRs in the Unreal plugin module.
- Milestone: Level geometry and IR library are ready for integration.

### Week 5 — March 31 to April 4: IR Switching and Audio Volume Integration

- Place Audio Volume actors to define reverb zones corresponding to each room.
- Implement IR switching logic: when the player enters a new Audio Volume, the active IR changes to match the environment.
- Handle edge cases in IR switching (crossfading between IRs to avoid clicks/pops at zone boundaries).
- Milestone: Walking between rooms in the Unreal level produces distinct, environment-appropriate reverb.

### Week 6 — April 7 to April 11: Testing and GUI Polish

- Conduct thorough testing of both the standalone JUCE plugin (in Reaper) and the Unreal integration.
- Polish the JUCE plugin GUI layout and ensure it presents well for demonstration.
- Fix any remaining bugs, audio artifacts, or performance issues discovered during testing.
- Milestone: Both the JUCE plugin and Unreal demo are stable and free of major issues.

### Week 7 — April 14 to April 18: Final Demo Preparation and Documentation

- Record a demo video or prepare a live demo walkthrough for finals week.
- Finalize project documentation and portfolio writeup.
- Buffer time for any tasks that slipped from previous weeks.
- Milestone: Project is demo-ready for finals week presentation. Final version submitted to Moodle by April 20.

## Overview

This project implements a real-time convolution reverb system targeting 3D game engines. The core motivation is applying realistic, environment-specific reverb to dynamic audio sources — particularly player voice in VOIP proximity chat — where traditional baked-in reverb is insufficient. The fall semester established the DSP foundation: both time-domain and frequency-domain convolution algorithms, a working JUCE plugin tested in a DAW, and embedded impulse responses. The spring semester extends this into a game engine context by integrating the convolver into Unreal Engine as a native audio effect.

In terms of experience level: the algorithm implementation and JUCE plugin development built on foundational DSP knowledge from coursework (MAT 320/321) but required significant new learning, particularly around the Overlap-Add method, real-time audio thread constraints, and the JUCE framework. Unreal Engine integration is largely new territory — while I have general familiarity with Unreal, I have not previously worked with its audio plugin API or Submix Effect system. This will be the area requiring the most new learning this semester. The UI work is the area I have the most prior experience with.

The majority of spring semester effort will go toward the audio engine component (Unreal integration), as the algorithm implementation is substantially complete from the fall. UI work will be secondary, focused on exposing proper parameter controls in the JUCE plugin and ensuring the Unreal demo is presentable.

## Algorithm Implementation

The convolution algorithms are complete and verified from MUS 470. No changes to the core DSP are planned for the spring semester.

The frequency-domain convolver implements Frank Wefers's Overlap-Add scheme for partitioned convolution. Input audio blocks of B = 512 samples are zero-padded to an FFT size of K = 1024 and transformed via forward FFT. The result is multiplied element-wise with the pre-computed IR spectrum (the IR of N = 128 samples is similarly zero-padded and transformed once at load time). The product is inverse-transformed back to the time domain, producing B + N - 1 = 639 output samples. The overlap tail (samples beyond the block size B) is stored and summed with the beginning of the next block's output, ensuring seamless reconstruction. The constraint K >= B + N - 1 is enforced to prevent time-domain aliasing.

The time-domain convolver serves as a verification baseline, computing the direct convolution sum using a circular delay buffer. Its O(N^2) complexity makes it impractical for production use but valuable for A/B testing against the frequency-domain implementation.

The only algorithmic work this semester involves adapting the convolver for Unreal Engine's audio pipeline, which may require adjustments to block size handling and sample rate assumptions but does not change the underlying algorithm.

## User Interface with Parameters

The current JUCE plugin GUI provides a toggle button for algorithm selection (time-domain vs. frequency-domain) and a combo box for IR selection, with a status label displaying the active configuration. The dry/wet mix is hardcoded to 1.0 with no user control.

Spring semester UI work includes:

- Adding APVTS-backed parameter controls: a dry/wet mix slider will be the primary addition, allowing users to blend between the unprocessed and convolved signal. This parameter will be automatable within a DAW host.
- Cleaning up the editor layout for a more polished appearance during the final demonstration.
- The Unreal Engine demo does not require a custom in-engine UI. IR switching will be driven by Audio Volume placement in the level editor, and the convolution effect will be configured through Unreal's standard Submix Effect preset system.

The UI scope is intentionally modest this semester. The emphasis is on functional, well-integrated controls rather than visual complexity, as the primary demonstration vehicle is the Unreal Engine walkthrough rather than the standalone plugin editor.

## Audio Engine Components

This is the primary focus of the spring semester. The project currently satisfies the audio engine requirement through its JUCE AudioProcessor implementation: a real-time plugin with its own audio callback (processBlock), managed audio buffers, thread-safe state handling, and multi-format output (VST3, AU, Standalone). This will be preserved and improved with proper APVTS parameter management.

The major new audio engine work is porting the convolver into Unreal Engine as a native Submix Effect. This involves:

- Implementing FSoundEffectSubmix and USoundEffectSubmixPreset subclasses that hook into Unreal's audio rendering pipeline.
- Managing audio buffers within Unreal's audio thread, which operates under similar real-time constraints as JUCE's audio callback (no allocation, no blocking).
- Resolving the FFT dependency for the Unreal build environment, either by integrating a standalone FFT library or linking the JUCE DSP module.
- Implementing IR switching driven by gameplay events (player entering Audio Volume zones), which requires coordination between the game thread (collision/overlap detection) and the audio thread (swapping the active IR spectrum). The existing try-lock pattern from the JUCE plugin will be adapted for this purpose.

The final deliverable will demonstrate two audio engine integrations: the original JUCE plugin running in a DAW (Reaper), and the Unreal Engine Submix Effect processing in-game audio with per-room impulse response switching.

References:

Verlag, L., & Gmbh, B. (n.d.). Partitioned convolution algorithms for real-time auralization. https://publications.rwth-aachen.de/record/466561/files/466561.pdf

Steiglitz, K. (2020). A digital signal processing primer: with applications to digital audio and computer music. Dover Publications, Inc.

Matthijs Hollemans. (2024). The Complete Beginner's Guide to Audio Plug-in Development.

Epic Games. Unreal Engine 5 Documentation: Audio System, Submix Effects, Audio Volumes. https://dev.epicgames.com/documentation/en-us/unreal-engine/
