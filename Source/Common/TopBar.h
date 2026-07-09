/*
  ==============================================================================

    TopBar.h

    Shared plugin header bar for the standalone VST3/AU FxmeFX plugins: dark
    background, the FX-Mechanics logo, the plugin name, a short description,
    and the version number, with an accent hairline in the plugin's own base
    colour. Same pattern as Spread and the other FX-Mechanics products.

    Deliberately lives outside the embeddable *Component classes — those are
    shared with the FX-Mechanics host bundle, which draws its own chrome.
    Only each plugin's PluginEditor (the VST3/AU/Standalone wrapper) uses it.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeCommonBinaryData.h>

namespace fxmefx
{

constexpr int kTopBarHeight = 44;

// Near-black background with just a whisper of the plugin's accent colour —
// same restrained, dark feel as Spread's backdrop.
inline void paintTintedBackground (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent)
{
    const auto topColour    = juce::Colours::black.interpolatedWith (accent, 0.05f);
    const auto bottomColour = juce::Colours::black.interpolatedWith (accent, 0.02f);
    juce::ColourGradient grad (bottomColour, bounds.getBottomLeft(),
                               topColour, bounds.getTopRight(), false);
    g.setGradientFill (grad);
    g.fillRect (bounds);
}

// Same near-black/whisper-of-accent treatment, but along the component's
// corner-to-corner diagonal — for the embeddable effect Components' own
// paint(), which is what actually fills a plugin window (the standalone
// editor's flat background above sits entirely behind it).
inline void paintComponentBackground (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent)
{
    const auto diagonale     = bounds.getTopLeft() - bounds.getBottomRight();
    const auto length        = diagonale.getDistanceFromOrigin();
    const auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    const auto height        = (bounds.getWidth() * bounds.getHeight()) / length;

    const auto darkBase     = juce::Colour (0xff181818);
    const auto lightCorner = darkBase.interpolatedWith (accent, 0.14f);
    const auto darkCorner  = darkBase.interpolatedWith (accent, 0.05f);

    juce::ColourGradient grad (darkCorner,  perpendicular *  height,
                               lightCorner, perpendicular * -height, false);
    g.setGradientFill (grad);
    g.fillRect (bounds);
}

class TopBar : public juce::Component
{
public:
    TopBar (juce::String pluginName, juce::String description, juce::Colour accentColour)
        : name (std::move (pluginName)), blurb (std::move (description)), accent (accentColour)
    {
        logo = juce::ImageCache::getFromMemory (FxmeCommonBinaryData::logo_png,
                                                FxmeCommonBinaryData::logo_pngSize);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        const auto bg = juce::Colour (0xff14101a);
        juce::ColourGradient grad (bg.brighter (0.12f), b.getTopLeft(), bg, b.getBottomLeft(), false);
        g.setGradientFill (grad);
        g.fillRect (b);
        g.setColour (accent.withAlpha (0.55f));
        g.fillRect (b.removeFromBottom (1.5f));

        auto area = getLocalBounds().reduced (10, 5);

        if (logo.isValid())
        {
            const int side = area.getHeight();
            auto logoArea = area.removeFromLeft (side);
            g.drawImage (logo, logoArea.toFloat(),
                         juce::RectanglePlacement::centred
                       | juce::RectanglePlacement::onlyReduceInSize);
            area.removeFromLeft (10);
        }

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions ((float) getHeight() * 0.48f, juce::Font::bold)));
        const int nameWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), name) + 8;
        g.drawText (name, area.removeFromLeft (nameWidth), juce::Justification::centredLeft);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.drawText ("v" JucePlugin_VersionString "  -  FX-Mechanics",
                    area.removeFromRight (140), juce::Justification::centredRight);

        area.removeFromLeft (10);
        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::Font (juce::FontOptions ((float) getHeight() * 0.24f)));
        g.drawText (blurb, area, juce::Justification::centredLeft);
    }

private:
    juce::String name, blurb;
    juce::Colour accent;
    juce::Image logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBar)
};

} // namespace fxmefx
