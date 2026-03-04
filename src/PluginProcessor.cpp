#include "PluginProcessor.h"
#include "BinaryData.h"
#include "PluginEditor.h"

const std::array<IRInfo, 4> &SpectralConvolverAudioProcessor::getIRList() {
  static const std::array<IRInfo, 4> irList = {{
      {"Ampitheater", "Ampitheater_wav", BinaryData::Ampitheater_wav,
       BinaryData::Ampitheater_wavSize},
      {"Bedroom", "Bedroom_wav", BinaryData::Bedroom_wav,
       BinaryData::Bedroom_wavSize},
      {"Synthetic", "Synthetic_wav", BinaryData::Synthetic_wav,
       BinaryData::Synthetic_wavSize},
      {"Kronecker", "Kronecker_wav", BinaryData::Kronecker_wav,
       BinaryData::Kronecker_wavSize},
  }};
  return irList;
}

SpectralConvolverAudioProcessor::SpectralConvolverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
  loadImpulseResponseByIndex(0);
}

SpectralConvolverAudioProcessor::~SpectralConvolverAudioProcessor() {}

const juce::String SpectralConvolverAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool SpectralConvolverAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool SpectralConvolverAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool SpectralConvolverAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double SpectralConvolverAudioProcessor::getTailLengthSeconds() const {
  if (irLoaded.load() && currentSampleRate > 0.0)
    return static_cast<double>(irLength) / currentSampleRate;
  return 0.0;
}

int SpectralConvolverAudioProcessor::getNumPrograms() { return 1; }
int SpectralConvolverAudioProcessor::getCurrentProgram() { return 0; }
void SpectralConvolverAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}
const juce::String SpectralConvolverAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void SpectralConvolverAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

//==============================================================================
int SpectralConvolverAudioProcessor::calculateFFTOrder(int irLen,
                                                       int blockSize) {
  const int minFFTSize = blockSize + irLen - 1;
  int order = 1;
  while ((1 << order) < minFFTSize)
    ++order;
  order = std::max(6, std::min(14, order));

  return order;
}

void SpectralConvolverAudioProcessor::rebuildConvolvers() {
  const juce::SpinLock::ScopedLockType lock(irLock);

  freqConvolvers.clear();
  timeConvolvers.clear();

  if (currentIR.empty())
    return;

  fftOrder = calculateFFTOrder(irLength, currentBlockSize);

  const int numChannels =
      std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());

  for (int ch = 0; ch < numChannels; ++ch) {
    // Create frequency domain convolver
    freqConvolvers.push_back(std::make_unique<FreqDomainConvolver>(
        currentIR, fftOrder, currentBlockSize));

    // Create time domain convolver
    timeConvolvers.push_back(std::make_unique<TimeDomainConvolver>(currentIR));
  }

  irLoaded.store(true);
  irPendingRebuild.store(false);

  DBG("Convolvers rebuilt: " << numChannels << " channels, FFT order "
                             << fftOrder << " (size " << (1 << fftOrder)
                             << "), IR length " << irLength);
}

void SpectralConvolverAudioProcessor::prepareToPlay(double sampleRate,
                                                    int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;
  rebuildConvolvers();
}

void SpectralConvolverAudioProcessor::releaseResources() {
  const juce::SpinLock::ScopedLockType lock(irLock);

  for (auto &conv : freqConvolvers)
    if (conv)
      conv->reset();

  for (auto &conv : timeConvolvers)
    if (conv)
      conv->reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralConvolverAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void SpectralConvolverAudioProcessor::setConvolverType(ConvolverType type) {
  currentConvolverType.store(type);
  DBG("Convolver type set to: " << (type == ConvolverType::FrequencyDomain
                                        ? "Frequency Domain"
                                        : "Time Domain"));
}

void SpectralConvolverAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
  static bool firstCall = true;
  if (firstCall) {
    firstCall = false;
    DBG("========== PROCESSBLOCK FIRST CALL ==========");
    DBG("freqConvolvers.size(): " + juce::String(freqConvolvers.size()));
    DBG("timeConvolvers.size(): " + juce::String(timeConvolvers.size()));
    DBG("currentIR.size(): " + juce::String(currentIR.size()));
    DBG("buffer samples: " + juce::String(buffer.getNumSamples()));
    DBG("=============================================");
  }

  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;

  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();
  const auto numSamples = buffer.getNumSamples();
  const float wetGain = 1.0f;

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; i++)
    buffer.clear(i, 0, numSamples);

  if (irPendingRebuild.load())
    rebuildConvolvers();

  if (!irLoaded.load() || (freqConvolvers.empty() && timeConvolvers.empty()))
    return;

  const juce::SpinLock::ScopedTryLockType lock(irLock);

  if (!lock.isLocked())
    return;

  const auto convolverType = currentConvolverType.load();

  for (int channel = 0; channel < totalNumInputChannels; channel++) {
    // Check convolver availability based on type
    if (convolverType == ConvolverType::FrequencyDomain)
      if (channel >= static_cast<int>(freqConvolvers.size()) ||
          !freqConvolvers[channel])
        continue;
      else if (channel >= static_cast<int>(timeConvolvers.size()) ||
               !timeConvolvers[channel])
        continue;

    auto *channelData = buffer.getWritePointer(channel);

    std::vector<float> drySignal;
    if (dryWetMix < 1.0f)
      drySignal.assign(channelData, channelData + numSamples);

    // Process through the selected convolver
    std::vector<float> wetSignal;

    if (convolverType == ConvolverType::FrequencyDomain)
      wetSignal =
          freqConvolvers[channel]->processBlock(channelData, numSamples);
    else
      wetSignal =
          timeConvolvers[channel]->processBlock(channelData, numSamples);

    static int debugCounter = 0;
    if (++debugCounter % 200 == 0) {
      float wetPeak = 0.0f;
      for (size_t i = 0; i < wetSignal.size(); ++i)
        wetPeak = std::max(wetPeak, std::abs(wetSignal[i]));

      DBG("Convolver: " << (convolverType == ConvolverType::FrequencyDomain
                                ? "Freq"
                                : "Time")
                        << ", wetSignal size: " + juce::String(wetSignal.size())
                        << ", peak: " + juce::String(wetPeak));
    }

    const int outputSize =
        std::min(static_cast<int>(wetSignal.size()), numSamples);

    if (dryWetMix >= 1.0f) {
      for (int i = 0; i < outputSize; ++i)
        channelData[i] = wetSignal[i] * wetGain;
    } else if (dryWetMix <= 0.0f)
      ;  // 100% dry - leave buffer unchanged
    else // Mix dry and wet
    {
      const float wet = dryWetMix;
      const float dry = 1.0f - dryWetMix;
      for (int i = 0; i < outputSize; ++i)
        channelData[i] = dry * drySignal[i] + (wet * wetSignal[i] * wetGain);
    }

    for (int i = outputSize; i < numSamples; ++i)
      channelData[i] = 0.0f;
  }
}

void SpectralConvolverAudioProcessor::loadImpulseResponse(
    const std::vector<float> &ir) {
  if (ir.empty())
    return;

  const juce::SpinLock::ScopedLockType lock(irLock);
  currentIR = ir;
  irLength = static_cast<int>(ir.size());

  irPendingRebuild.store(true);
  DBG("IR loaded: " << irLength << " samples");
}

void SpectralConvolverAudioProcessor::loadImpulseResponseByIndex(int index) {
  const auto &irList = getIRList();
  if (index < 0 || index >= static_cast<int>(irList.size()))
    return;

  const auto &info = irList[static_cast<size_t>(index)];

  auto memStream = std::make_unique<juce::MemoryInputStream>(
      info.data, static_cast<size_t>(info.size), false);
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(std::move(memStream)));
  if (!reader) {
    DBG("Failed to create reader for IR: " + juce::String(info.displayName));
    return;
  }

  const int numSamples = static_cast<int>(reader->lengthInSamples);
  if (numSamples <= 0 || numSamples > 10 * 48000)
    return;

  juce::AudioBuffer<float> irBuffer(1, numSamples);
  reader->read(&irBuffer, 0, numSamples, 0, true, false);

  std::vector<float> ir(irBuffer.getReadPointer(0),
                        irBuffer.getReadPointer(0) + numSamples);

  currentIRIndex.store(index);
  loadImpulseResponse(ir);
  DBG("Loaded embedded IR: " + juce::String(info.displayName) + " (" +
      juce::String(numSamples) + " samples)");
}

bool SpectralConvolverAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *SpectralConvolverAudioProcessor::createEditor() {
  return new SpectralConvolverAudioProcessorEditor(*this);
}

void SpectralConvolverAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  juce::MemoryOutputStream stream(destData, true);
  stream.writeFloat(dryWetMix);
  stream.writeInt(static_cast<int>(currentConvolverType.load()));
  stream.writeInt(currentIRIndex.load());
}

void SpectralConvolverAudioProcessor::setStateInformation(const void *data,
                                                          int sizeInBytes) {
  juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
  if (sizeInBytes >= static_cast<int>(sizeof(float)))
    dryWetMix = stream.readFloat();

  if (sizeInBytes >= static_cast<int>(sizeof(float) + sizeof(int)))
    currentConvolverType.store(static_cast<ConvolverType>(stream.readInt()));

  if (sizeInBytes >= static_cast<int>(sizeof(float) + 2 * sizeof(int))) {
    int irIndex = stream.readInt();
    loadImpulseResponseByIndex(irIndex);
  }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new SpectralConvolverAudioProcessor();
}
