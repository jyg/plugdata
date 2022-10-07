/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Pd/PdInstance.h"
#include "Pd/PdLibrary.h"
#include "Standalone/PlugDataWindow.h"
#include "Statusbar.h"



class PlugDataLook;

class PlugDataPluginEditor;
class PlugDataAudioProcessor : public AudioProcessor, public pd::Instance, public Timer, public AudioProcessorParameter::Listener
{
   public:
    PlugDataAudioProcessor();
    ~PlugDataAudioProcessor() override;

    static AudioProcessor::BusesProperties buildBusesProperties();

    void setOversampling(int amount);
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
   
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    std::atomic<int> callbackType = 0;
    void timerCallback() override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void receiveNoteOn(const int channel, const int pitch, const int velocity) override;
    void receiveControlChange(const int channel, const int controller, const int value) override;
    void receiveProgramChange(const int channel, const int value) override;
    void receivePitchBend(const int channel, const int value) override;
    void receiveAftertouch(const int channel, const int value) override;
    void receivePolyAftertouch(const int channel, const int pitch, const int value) override;
    void receiveMidiByte(const int port, const int byte) override;
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    
    void receiveGuiUpdate(int type) override;

    void updateConsole() override;

    void synchroniseCanvas(String cnv, MemoryBlock block);

    void process(dsp::AudioBlock<float>, MidiBuffer&);

    void setCallbackLock(const CriticalSection* lock)
    {
        audioLock = lock;
    };

    const CriticalSection* getCallbackLock() override
    {
        return audioLock;
    };

    bool canAddBus(bool isInput) const override
    {
        return true;
    }

    bool canRemoveBus(bool isInput) const override
    {
        int nbus = getBusCount(isInput);
        return nbus > 0;
    }
    
    static Component* getComponentByIDImpl(Component* component, String ID) {
        if(auto* found = component->findChildWithID(ID)) return found;
        
        for(auto* child : component->getChildren()) {
            if(auto* found = getComponentByIDImpl(child, ID)) {
                return found;
            }
        }
        
        return nullptr;
    }
    
    template<typename T>
    T* getComponentByID(String ID) {
        if(auto* editor = getActiveEditor()) {
            return dynamic_cast<T*>(getComponentByIDImpl(editor, ID));
        }
        
        return nullptr;
    }

    void initialiseFilesystem();
    void saveSettings();
    void updateSearchPaths();

    void sendMidiBuffer();
    void sendPlayhead();
    void sendParameters();

    void messageEnqueued() override;
    void performParameterChange(int type, int idx, float value) override;

    void loadPatch(String patch);
    void loadPatch(const File& patch);

    void titleChanged() override;

    void setTheme(bool themeToUse);

    Colour getForegroundColour() override;
    Colour getBackgroundColour() override;
    Colour getTextColour() override;
    Colour getOutlineColour() override;

    // All opened patches
    OwnedArray<pd::Patch> patches;

    int lastUIWidth = 1000, lastUIHeight = 650;

    std::vector<float*> channelPointers;
    std::atomic<float>* volume;

    ValueTree settingsTree = ValueTree("PlugDataSettings");

    pd::Library objectLibrary;

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
    File appDir = homeDir.getChildFile(ProjectInfo::versionString);

    File settingsFile = homeDir.getChildFile("Settings.xml");
    File abstractions = appDir.getChildFile("Abstractions");

    Value locked = Value(var(false));
    Value commandLocked = Value(var(false));
    Value zoomScale = Value(1.0f);

    AudioProcessorValueTreeState parameters;

    StatusbarSource statusbarSource;

    Value tailLength = Value(0.0f);

    SharedResourcePointer<PlugDataLook> lnf;

    static inline constexpr int numParameters = 512;
    static inline constexpr int numInputBuses = 16;
    static inline constexpr int numOutputBuses = 16;

#if PLUGDATA_STANDALONE
    std::atomic<float> standaloneParams[numParameters] = {0};
#endif
    
    // Zero means no oversampling
    int oversampling = 0;

   private:
    void processInternal();

    int audioAdvancement = 0;
    std::vector<float> audioBufferIn;
    std::vector<float> audioBufferOut;

    MidiBuffer midiBufferIn;
    MidiBuffer midiBufferOut;
    MidiBuffer midiBufferTemp;
    MidiBuffer midiBufferCopy;

    bool midiByteIsSysex = false;
    uint8 midiByteBuffer[512] = {0};
    size_t midiByteIndex = 0;

    std::array<float, numParameters> lastParameters = {0};
    std::array<float, numParameters> changeGestureState = {0};

    std::vector<pd::Atom> atoms_playhead;

    int minIn = 2;
    int minOut = 2;
    
    std::unique_ptr<dsp::Oversampling<float>> oversampler;

    const CriticalSection* audioLock;
    
    static inline const String else_version = "ELSE v1.0-rc3";
    static inline const String cyclone_version = "cyclone v0.6-1";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataAudioProcessor)
};
