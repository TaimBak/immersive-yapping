/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class SpectralConvolverAudioProcessorEditor
    : public juce::AudioProcessorEditor {
public:
  SpectralConvolverAudioProcessorEditor(SpectralConvolverAudioProcessor &);
  ~SpectralConvolverAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  void updateStatusLabel();
  SpectralConvolverAudioProcessor &audioProcessor;

  juce::ToggleButton algorithmToggle;
  juce::Label algorithmLabel;
  juce::Label statusLabel;

  juce::ComboBox irSelector;
  juce::Label irSelectorLabel;

  juce::Slider dryWetSlider;
  juce::Label dryWetLabel;

  // APVTS attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      dryWetAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      irAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      algorithmAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      SpectralConvolverAudioProcessorEditor)
};
