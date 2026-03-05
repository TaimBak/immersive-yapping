#include "PluginProcessor.h"
#include "BinaryData.h"
#include "PluginEditor.h"

const std::array<IRInfo, 3> &SpectralConvolverAudioProcessor::getIRList()
{
  static const std::array<IRInfo, 3> irList = {{
      {"Synthetic", "Synthetic_wav", BinaryData::Synthetic_wav,
       BinaryData::Synthetic_wavSize},
      {"Kronecker", "Kronecker_wav", BinaryData::Kronecker_wav,
       BinaryData::Kronecker_wavSize},
      {"Room", "Room_wav", BinaryData::Room_wav, BinaryData::Room_wavSize},
  }};
  return irList;
}

juce::AudioProcessorValueTreeState::ParameterLayout
SpectralConvolverAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"dryWet", 1}, "Dry/Wet",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"algorithm", 1}, "Algorithm",
      juce::StringArray{"Frequency Domain", "Time Domain"}, 0));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"irIndex", 1}, "Impulse Response",
      juce::StringArray{"Synthetic", "Kronecker", "Room"}, 0));

  return layout;
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
  dryWetParam = apvts.getRawParameterValue("dryWet");
  algorithmParam = apvts.getRawParameterValue("algorithm");
  irIndexParam = apvts.getRawParameterValue("irIndex");

  loadImpulseResponseByIndex(0);
}

SpectralConvolverAudioProcessor::~SpectralConvolverAudioProcessor() {}

const juce::String SpectralConvolverAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

bool SpectralConvolverAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool SpectralConvolverAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool SpectralConvolverAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double SpectralConvolverAudioProcessor::getTailLengthSeconds() const
{
  if (irLoaded.load() && currentSampleRate > 0.0)
    return static_cast<double>(irLength) / currentSampleRate;
  return 0.0;
}

int SpectralConvolverAudioProcessor::getNumPrograms() { return 1; }
int SpectralConvolverAudioProcessor::getCurrentProgram() { return 0; }
void SpectralConvolverAudioProcessor::setCurrentProgram(int index)
{
  juce::ignoreUnused(index);
}
const juce::String SpectralConvolverAudioProcessor::getProgramName(int index)
{
  juce::ignoreUnused(index);
  return {};
}

void SpectralConvolverAudioProcessor::changeProgramName(
    int index, const juce::String &newName)
{
  juce::ignoreUnused(index, newName);
}

//==============================================================================
int SpectralConvolverAudioProcessor::calculateFFTOrder(int irLen, int blockSize)
{
  const int minFFTSize = blockSize + irLen - 1;
  int order = 1;
  while ((1 << order) < minFFTSize)
    ++order;
  order = std::max(6, std::min(14, order));

  return order;
}

void SpectralConvolverAudioProcessor::rebuildConvolvers()
{
  const juce::SpinLock::ScopedLockType lock(irLock);

  freqConvolvers.clear();
  timeConvolvers.clear();

  if (currentIR.empty())
    return;

  fftOrder = calculateFFTOrder(irLength, currentBlockSize);

  const int numChannels =
      std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());

  for (int ch = 0; ch < numChannels; ++ch)
  {
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
                                                    int samplesPerBlock)
{
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;
  wetBuffer.resize((size_t)samplesPerBlock);
  dryBuffer.resize((size_t)samplesPerBlock);
  rebuildConvolvers();
}

void SpectralConvolverAudioProcessor::releaseResources()
{
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
    const BusesLayout &layouts) const
{
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

void SpectralConvolverAudioProcessor::setConvolverType(ConvolverType type)
{
  currentConvolverType.store(type);
  DBG("Convolver type set to: " << (type == ConvolverType::FrequencyDomain
                                        ? "Frequency Domain"
                                        : "Time Domain"));
}

void SpectralConvolverAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
  static bool firstCall = true;
  if (firstCall)
  {
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

  // Read APVTS parameters
  const float dryWetMix = dryWetParam->load();
  const auto convolverType = static_cast<int>(algorithmParam->load()) == 0
                                 ? ConvolverType::FrequencyDomain
                                 : ConvolverType::TimeDomain;
  currentConvolverType.store(convolverType);

  const int newIRIndex = static_cast<int>(irIndexParam->load());
  if (newIRIndex != currentIRIndex.load())
    loadImpulseResponseByIndex(newIRIndex);

  if (irPendingRebuild.load())
    rebuildConvolvers();

  if (!irLoaded.load() || (freqConvolvers.empty() && timeConvolvers.empty()))
    return;

  const juce::SpinLock::ScopedTryLockType lock(irLock);

  if (!lock.isLocked())
    return;

  for (int channel = 0; channel < totalNumInputChannels; channel++)
  {
    // Check convolver availability based on type
    if (convolverType == ConvolverType::FrequencyDomain)
      if (channel >= static_cast<int>(freqConvolvers.size()) ||
          !freqConvolvers[channel])
        continue;
      else if (channel >= static_cast<int>(timeConvolvers.size()) ||
               !timeConvolvers[channel])
        continue;

    auto *channelData = buffer.getWritePointer(channel);

    // Save dry signal into pre-allocated buffer if mixing needed
    if (dryWetMix < 1.0f)
      std::copy(channelData, channelData + numSamples, dryBuffer.data());

    // Process through the selected convolver into pre-allocated wet buffer
    if (convolverType == ConvolverType::FrequencyDomain)
      freqConvolvers[channel]->processBlock(channelData, wetBuffer.data(),
                                            numSamples);
    else
      timeConvolvers[channel]->processBlock(channelData, wetBuffer.data(),
                                            (size_t)numSamples);

    static int debugCounter = 0;
    if (++debugCounter % 200 == 0)
    {
      float wetPeak = 0.0f;
      for (int i = 0; i < numSamples; ++i)
        wetPeak = std::max(wetPeak, std::abs(wetBuffer[(size_t)i]));

      DBG("Convolver: " << (convolverType == ConvolverType::FrequencyDomain
                                ? "Freq"
                                : "Time")
                        << ", wetSignal size: " + juce::String(numSamples)
                        << ", peak: " + juce::String(wetPeak));
    }

    if (dryWetMix >= 1.0f)
    {
      for (int i = 0; i < numSamples; ++i)
        channelData[i] = wetBuffer[(size_t)i] * wetGain;
    }
    else if (dryWetMix <= 0.0f)
      ;  // 100% dry - leave buffer unchanged
    else // Mix dry and wet
    {
      const float wet = dryWetMix;
      const float dry = 1.0f - dryWetMix;
      for (int i = 0; i < numSamples; ++i)
        channelData[i] =
            dry * dryBuffer[(size_t)i] + (wet * wetBuffer[(size_t)i] * wetGain);
    }
  }
}

void SpectralConvolverAudioProcessor::loadImpulseResponse(
    const std::vector<float> &ir)
{
  if (ir.empty())
    return;

  const juce::SpinLock::ScopedLockType lock(irLock);
  currentIR = ir;
  irLength = static_cast<int>(ir.size());

  irPendingRebuild.store(true);
  DBG("IR loaded: " << irLength << " samples");
}

void SpectralConvolverAudioProcessor::loadImpulseResponseByIndex(int index)
{
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
  if (!reader)
  {
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

juce::AudioProcessorEditor *SpectralConvolverAudioProcessor::createEditor()
{
  return new SpectralConvolverAudioProcessorEditor(*this);
}

void SpectralConvolverAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData)
{
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void SpectralConvolverAudioProcessor::setStateInformation(const void *data,
                                                          int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new SpectralConvolverAudioProcessor();
}
