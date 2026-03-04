/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

SpectralConvolverAudioProcessorEditor::SpectralConvolverAudioProcessorEditor(
    SpectralConvolverAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  // Algorithm toggle button
  algorithmToggle.setButtonText("Use Time Domain");
  addAndMakeVisible(algorithmToggle);

  algorithmLabel.setText("Convolution Algorithm:", juce::dontSendNotification);
  algorithmLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(algorithmLabel);

  // IR selector ComboBox
  irSelectorLabel.setText("Impulse Response:", juce::dontSendNotification);
  irSelectorLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(irSelectorLabel);

  const auto &irList = SpectralConvolverAudioProcessor::getIRList();
  for (int i = 0; i < static_cast<int>(irList.size()); ++i)
    irSelector.addItem(irList[static_cast<size_t>(i)].displayName, i + 1);
  addAndMakeVisible(irSelector);

  // Dry/Wet slider
  dryWetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  addAndMakeVisible(dryWetSlider);

  dryWetLabel.setText("Dry/Wet:", juce::dontSendNotification);
  dryWetLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(dryWetLabel);

  // Status label
  updateStatusLabel();
  statusLabel.setJustificationType(juce::Justification::centred);
  statusLabel.setFont(juce::FontOptions(18.0f));
  addAndMakeVisible(statusLabel);

  // APVTS attachments
  dryWetAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "dryWet", dryWetSlider);
  irAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          audioProcessor.apvts, "irIndex", irSelector);
  algorithmAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          audioProcessor.apvts, "algorithm", algorithmToggle);

  // Update status label when algorithm toggle changes
  algorithmToggle.onClick = [this]() { updateStatusLabel(); };

  setSize(400, 400);
}

SpectralConvolverAudioProcessorEditor::
    ~SpectralConvolverAudioProcessorEditor() {}

void SpectralConvolverAudioProcessorEditor::updateStatusLabel() {
  if (!algorithmToggle.getToggleState())
    statusLabel.setText("Current: Frequency Domain (FFT)",
                        juce::dontSendNotification);
  else
    statusLabel.setText("Current: Time Domain (Direct)",
                        juce::dontSendNotification);
}

void SpectralConvolverAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(24.0f));
  g.drawFittedText("Spectral Convolver", getLocalBounds().removeFromTop(60),
                   juce::Justification::centred, 1);

  // Draw IR info
  g.setFont(juce::FontOptions(14.0f));
  juce::String irInfo;
  if (audioProcessor.isIRLoaded()) {
    const auto &irList = SpectralConvolverAudioProcessor::getIRList();
    int idx = audioProcessor.getCurrentIRIndex();
    irInfo = juce::String(irList[static_cast<size_t>(idx)].displayName) +
             " - " + juce::String(audioProcessor.getIRLength()) + " samples";
  } else {
    irInfo = "No IR Loaded";
  }
  g.drawFittedText(irInfo, getLocalBounds().removeFromBottom(40),
                   juce::Justification::centred, 1);
}

void SpectralConvolverAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds();
  bounds.removeFromTop(70);    // Space for title
  bounds.removeFromBottom(50); // Space for IR info

  auto centerArea = bounds.reduced(20);

  // Status label at top of center area
  statusLabel.setBounds(centerArea.removeFromTop(30));

  centerArea.removeFromTop(20); // Spacing

  // Toggle row
  auto toggleRow = centerArea.removeFromTop(30);
  algorithmLabel.setBounds(toggleRow.removeFromLeft(150));
  toggleRow.removeFromLeft(10); // Spacing
  algorithmToggle.setBounds(toggleRow);

  centerArea.removeFromTop(15); // Spacing

  // IR selector row
  auto irRow = centerArea.removeFromTop(30);
  irSelectorLabel.setBounds(irRow.removeFromLeft(150));
  irRow.removeFromLeft(10); // Spacing
  irSelector.setBounds(irRow);

  centerArea.removeFromTop(15); // Spacing

  // Dry/Wet slider row
  auto dryWetRow = centerArea.removeFromTop(30);
  dryWetLabel.setBounds(dryWetRow.removeFromLeft(150));
  dryWetRow.removeFromLeft(10); // Spacing
  dryWetSlider.setBounds(dryWetRow);
}
