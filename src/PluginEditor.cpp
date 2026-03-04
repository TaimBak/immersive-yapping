/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectralConvolverAudioProcessorEditor::SpectralConvolverAudioProcessorEditor(SpectralConvolverAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Algorithm toggle button
    algorithmToggle.setButtonText("Use Time Domain");
    algorithmToggle.setToggleState(audioProcessor.getConvolverType() == ConvolverType::TimeDomain,juce::dontSendNotification);
    algorithmToggle.addListener(this);
    addAndMakeVisible(algorithmToggle);

    // Label for the toggle
    algorithmLabel.setText("Convolution Algorithm:", juce::dontSendNotification);
    algorithmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(algorithmLabel);

    // IR selector ComboBox
    irSelectorLabel.setText("Impulse Response:", juce::dontSendNotification);
    irSelectorLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(irSelectorLabel);

    const auto& irList = SpectralConvolverAudioProcessor::getIRList();
    for (int i = 0; i < static_cast<int>(irList.size()); ++i)
        irSelector.addItem(irList[static_cast<size_t>(i)].displayName, i + 1);

    irSelector.setSelectedId(audioProcessor.getCurrentIRIndex() + 1, juce::dontSendNotification);
    irSelector.onChange = [this]()
    {
        int selectedIndex = irSelector.getSelectedId() - 1;
        audioProcessor.loadImpulseResponseByIndex(selectedIndex);
        repaint();
    };
    addAndMakeVisible(irSelector);

    // Status label showing current algorithm
    updateStatusLabel();
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::FontOptions(18.0f));
    addAndMakeVisible(statusLabel);

    setSize(400, 350);
}

SpectralConvolverAudioProcessorEditor::~SpectralConvolverAudioProcessorEditor()
{
    algorithmToggle.removeListener(this);
}

void SpectralConvolverAudioProcessorEditor::updateStatusLabel()
{
    if (audioProcessor.getConvolverType() == ConvolverType::FrequencyDomain)
        statusLabel.setText("Current: Frequency Domain (FFT)", juce::dontSendNotification);
    else
        statusLabel.setText("Current: Time Domain (Direct)", juce::dontSendNotification);
}

void SpectralConvolverAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &algorithmToggle)
    {
        if (algorithmToggle.getToggleState())
            audioProcessor.setConvolverType(ConvolverType::TimeDomain);
        else
            audioProcessor.setConvolverType(ConvolverType::FrequencyDomain);

        updateStatusLabel();
    }
}

void SpectralConvolverAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f));
    g.drawFittedText("Spectral Convolver", getLocalBounds().removeFromTop(60),
        juce::Justification::centred, 1);

    // Draw IR info
    g.setFont(juce::FontOptions(14.0f));
    juce::String irInfo;
    if (audioProcessor.isIRLoaded())
    {
        const auto& irList = SpectralConvolverAudioProcessor::getIRList();
        int idx = audioProcessor.getCurrentIRIndex();
        irInfo = juce::String(irList[static_cast<size_t>(idx)].displayName)
            + " - " + juce::String(audioProcessor.getIRLength()) + " samples";
    }
    else
    {
        irInfo = "No IR Loaded";
    }
    g.drawFittedText(irInfo, getLocalBounds().removeFromBottom(40),
        juce::Justification::centred, 1);
}

void SpectralConvolverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(70);  // Space for title
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
}
