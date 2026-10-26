#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <cmath>
#include <iostream>
#include <memory>

class MidSide670RenderApplication final : public juce::JUCEApplication,
                                          private juce::Timer
{
public:
    const juce::String getApplicationName() override { return "MidSide-670 Screenshot"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}
    void systemRequestedQuit() override {}
    void shutdown() override {}

    void initialise (const juce::String& commandLine) override
    {
        outputFile = juce::File::getCurrentWorkingDirectory().getChildFile ("dist/midside670-ui.png");

        for (const auto& argument : juce::StringArray::fromTokens (commandLine, true))
        {
            if (argument.startsWith ("--out="))
            {
                const auto path = argument.substring (6);
                outputFile = juce::File::isAbsolutePath (path)
                                 ? juce::File (path)
                                 : juce::File::getCurrentWorkingDirectory().getChildFile (path);
            }
        }

        outputFile = outputFile.getFullPathName();
        outputFile.getParentDirectory().createDirectory();

        processor = std::make_unique<MidSide670AudioProcessor>();
        processor->setPlayConfigDetails (2, 2, 48000.0, 1024);
        processor->prepareToPlay (48000.0, 1024);
        processor->setCurrentProgram (4); // M/S Drum Room, to show independent GR meters.

        // Park the pointer away from the unit before it appears, so the captured
        // snapshot never carries a stale hover highlight.
        juce::Desktop::getInstance().getMainMouseSource().setScreenPosition ({ 5.0f, 5.0f });

        editor = std::unique_ptr<juce::AudioProcessorEditor> (processor->createEditor());

        if (editor == nullptr)
        {
            std::cerr << "Could not create the plugin editor.\n";
            setApplicationReturnValue (1);
            quit();
            return;
        }

        editor->setSize (1180, 720);
        editor->addToDesktop (0);
        editor->setTopLeftPosition (30, 30);
        editor->setVisible (true);
        editor->toFront (true);

        testBuffer.setSize (2, 1024);
        startTimerHz (30);

        juce::Timer::callAfterDelay (1800, [this] { capture(); });
    }

private:
    void timerCallback() override
    {
        if (processor == nullptr)
            return;

        for (int i = 0; i < testBuffer.getNumSamples(); ++i)
        {
            const double t = static_cast<double> (sampleCounter + i) / 48000.0;
            const float movement = 0.54f + 0.26f * static_cast<float> (
                std::sin (juce::MathConstants<double>::twoPi * 0.9 * t));
            const double fundamental = juce::MathConstants<double>::twoPi * 220.0 * t;
            const double sideTone = juce::MathConstants<double>::twoPi * 330.0 * t;

            const float centre = 0.18f * static_cast<float> (
                std::sin (fundamental) + 0.30 * std::sin (2.0 * fundamental));
            const float edge = 0.15f * static_cast<float> (
                std::sin (sideTone) + 0.35 * std::sin (2.2 * sideTone));

            testBuffer.setSample (0, i, movement * (centre + edge));
            testBuffer.setSample (1, i, movement * (centre - edge * 0.82f));
        }

        sampleCounter += testBuffer.getNumSamples();
        juce::MidiBuffer midi;
        processor->processBlock (testBuffer, midi);
    }

    void capture()
    {
        if (editor == nullptr)
            return;

        stopTimer();
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

        if (image.isValid())
        {
            outputFile.deleteFile();

            if (auto stream = std::unique_ptr<juce::FileOutputStream> (outputFile.createOutputStream()))
            {
                juce::PNGImageFormat png;
                const bool written = png.writeImageToStream (image, *stream);
                stream->flush();

                if (written)
                    std::cout << "Rendered " << outputFile.getFullPathName() << '\n';
                else
                    std::cerr << "Failed to encode PNG.\n";
            }
            else
            {
                std::cerr << "Could not open " << outputFile.getFullPathName() << '\n';
            }
        }
        else
        {
            std::cerr << "Editor snapshot was invalid.\n";
        }

        processor->releaseResources();
        editor->removeFromDesktop();
        editor.reset();
        processor.reset();
        quit();
    }

    std::unique_ptr<MidSide670AudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::File outputFile;
    juce::AudioBuffer<float> testBuffer;
    juce::int64 sampleCounter = 0;
};

START_JUCE_APPLICATION (MidSide670RenderApplication)
