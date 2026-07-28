#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HecateLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Antique gold, matched to the jewelry in the background artwork
    inline static const juce::Colour accent{0xffd0a95c};
    inline static const juce::Colour accentDim{0xff8a7448};
    inline static const juce::Colour textBright{0xffd8d2c4};
    inline static const juce::Colour textDim{0xff9a917e};
    inline static const juce::Colour surface{0xff15121a};

    HecateLookAndFeel()
    {
        setColour(juce::Label::textColourId, textBright);
        setColour(juce::ComboBox::textColourId, textBright);
        setColour(juce::ComboBox::arrowColourId, accent);
        setColour(juce::ComboBox::backgroundColourId, surface);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xf217131e));
        setColour(juce::PopupMenu::textColourId, textBright);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha(0.25f));
        setColour(juce::PopupMenu::highlightedTextColourId, textBright);
        setColour(juce::TextButton::buttonColourId, surface);
        setColour(juce::TextButton::buttonOnColourId, accent);
        setColour(juce::TextButton::textColourOffId, accent);
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff17131e));
        setColour(juce::BubbleComponent::backgroundColourId, juce::Colour(0xf217131e));
        setColour(juce::BubbleComponent::outlineColourId, accentDim);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(3.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto arcRadius = radius - 2.0f;
        juce::PathStrokeType stroke(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        // Track and value arcs
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xcc211c26));
        g.strokePath(track, stroke);

        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
        g.setColour(accent);
        g.strokePath(value, stroke);

        // Body: dark with a soft top rim light, floating shadow below
        auto knobRadius = arcRadius - 7.0f;
        auto knobRect = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f).withCentre(centre);

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillEllipse(knobRect.translated(0.0f, 2.5f));

        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff322b3a), centre.x, knobRect.getY(),
                                               juce::Colour(0xff14111a), centre.x, knobRect.getBottom(),
                                               false));
        g.fillEllipse(knobRect);

        g.setColour(juce::Colours::white.withAlpha(0.10f));
        juce::Path rim;
        rim.addCentredArc(centre.x, centre.y, knobRadius - 0.8f, knobRadius - 0.8f, 0.0f,
                          -2.4f, -0.7f, true);
        g.strokePath(rim, juce::PathStrokeType(1.2f));

        g.setColour(juce::Colour(0xff4a4152));
        g.drawEllipse(knobRect, 1.0f);

        // Pointer
        juce::Path pointer;
        pointer.addRoundedRectangle(-1.5f, -knobRadius + 3.5f, 3.0f, knobRadius * 0.40f, 1.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
        g.setColour(accent);
        g.fillPath(pointer);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool isHighlighted, bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        if (button.getComponentID() == "tab")
        {
            if (isHighlighted || isDown)
            {
                g.setColour(juce::Colours::white.withAlpha(0.06f));
                g.fillRoundedRectangle(bounds.reduced(2.0f), 6.0f);
            }
            if (button.getToggleState())
            {
                g.setColour(accent);
                g.fillRect(bounds.getX() + 10.0f, bounds.getBottom() - 3.0f,
                           bounds.getWidth() - 20.0f, 2.0f);
            }
            return;
        }

        const float corner = bounds.getHeight() * 0.5f > 14.0f ? 8.0f : bounds.getHeight() * 0.5f;
        auto fill = button.getToggleState() ? accent : surface.withAlpha(0.88f);
        if (isDown)
            fill = fill.darker(0.2f);
        else if (isHighlighted)
            fill = fill.brighter(0.12f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds.reduced(0.5f), corner);
        g.setColour(accentDim.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool isHighlighted, bool) override
    {
        const bool isTab = button.getComponentID() == "tab";
        auto font = juce::Font(juce::FontOptions(isTab ? 13.0f : juce::jmin(12.0f, button.getHeight() * 0.62f),
                                                 juce::Font::bold))
                        .withExtraKerningFactor(isTab ? 0.2f : 0.08f);
        g.setFont(font);

        if (isTab)
            g.setColour(button.getToggleState() ? accent : (isHighlighted ? textBright : textDim));
        else
            g.setColour(button.findColour(button.getToggleState()
                                              ? juce::TextButton::textColourOnId
                                              : juce::TextButton::textColourOffId));

        g.drawText(button.getButtonText(), button.getLocalBounds(),
                   juce::Justification::centred, false);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
        g.setColour(surface.withAlpha(0.88f));
        g.fillRoundedRectangle(bounds.reduced(0.5f), 6.0f);
        g.setColour(accentDim.withAlpha(box.isEnabled() ? 0.5f : 0.2f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float)width - 13.0f, cy = (float)height * 0.5f;
        arrow.addTriangle(cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
        g.setColour(accent.withAlpha(box.isEnabled() ? 1.0f : 0.35f));
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(juce::FontOptions(12.5f));
    }
};
