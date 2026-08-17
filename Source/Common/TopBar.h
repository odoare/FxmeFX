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
#include "Version.h"

namespace fxmefx
{

constexpr int kTopBarHeight = 44;

// Shared sizing for the effect Components' own first row (on/off button,
// title, optional extra control) so every plugin's header lines up the same.
constexpr float kHeaderRowHeight = 40.0f;
constexpr float kOnButtonWidth   = 60.0f;

// The two house backdrops now live in FxmeTools as fxme::paintTintedBackground
// and fxme::paintComponentBackground (lookandfeels/PanelBackground.h), so every
// FX-Mechanics plugin shares them. Only this bar, which embeds FxmeFX's own logo
// and series version, is still project-specific.

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
        g.drawText ("v" FXMEFX_VERSION_STRING "  -  FX-Mechanics",
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
