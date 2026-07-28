#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HecateLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Antique gold, matched to the jewelry in the background artwork
    inline static const juce::Colour accent{0xffd0a95c};
    inline static const juce::Colour textBright{0xffd8d2c4};
    inline static const juce::Colour textDim{0xff9a917e};

    HecateLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, textBright);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, textBright);
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff26212b));
        setColour(juce::TextButton::buttonOnColourId, accent);
        setColour(juce::TextButton::textColourOffId, accent);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto arcRadius = radius - 3.0f;
        juce::PathStrokeType stroke(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff211d27));
        g.strokePath(track, stroke);

        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
        g.setColour(accent);
        g.strokePath(value, stroke);

        auto knobRadius = arcRadius - 8.0f;
        auto knobRect = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f).withCentre(centre);

        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillEllipse(knobRect.translated(0.0f, 2.0f));

        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3542), centre.x, knobRect.getY(),
                                               juce::Colour(0xff191621), centre.x, knobRect.getBottom(),
                                               false));
        g.fillEllipse(knobRect);

        g.setColour(juce::Colour(0xff544d5e));
        g.drawEllipse(knobRect, 1.2f);

        juce::Path pointer;
        pointer.addRoundedRectangle(-2.0f, -knobRadius + 4.0f, 4.0f, knobRadius * 0.42f, 2.0f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
        g.setColour(accent);
        g.fillPath(pointer);
    }
};
