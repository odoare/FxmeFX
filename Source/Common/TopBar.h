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

    setPresetBank() adds the effect's preset bar (FxmeTools module presets,
    shared with every plugin that embeds the effect) at the right end, with
    its "..." browser. It has priority over the text: in a narrow window the
    version and then the description give way to it.

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

    /** Shows a preset bar on `bank` (the effect's module presets). */
    void setPresetBank (fxme::PresetBank& bank)
    {
        presetBar = std::make_unique<fxme::PresetBarComponent> (bank);
        presetBar->setAccentColour (accent);
        presetBar->setBrowserButtonVisible (true);
        presetBar->setBrowserSize (300, 360);
        addAndMakeVisible (*presetBar);
        resized();
    }

    void resized() override
    {
        if (presetBar != nullptr)
            presetBar->setBounds (layout().preset);
    }

    void paint (juce::Graphics& g) override
    {
        const auto parts = layout();
        auto b = getLocalBounds().toFloat();

        const auto bg = juce::Colour (0xff14101a);
        juce::ColourGradient grad (bg.brighter (0.12f), b.getTopLeft(), bg, b.getBottomLeft(), false);
        g.setGradientFill (grad);
        g.fillRect (b);
        g.setColour (accent.withAlpha (0.55f));
        g.fillRect (b.removeFromBottom (1.5f));

        if (logo.isValid() && ! parts.logo.isEmpty())
            g.drawImage (logo, parts.logo.toFloat(),
                         juce::RectanglePlacement::centred
                       | juce::RectanglePlacement::onlyReduceInSize);

        g.setColour (juce::Colours::white);
        g.setFont (nameFont());
        g.drawText (name, parts.name, juce::Justification::centredLeft);

        g.setColour (juce::Colours::lightgrey);
        if (! parts.version.isEmpty())
        {
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText ("v" FXMEFX_VERSION_STRING "  -  FX-Mechanics",
                        parts.version, juce::Justification::centredRight);
        }

        if (! parts.blurb.isEmpty())
        {
            g.setFont (juce::Font (juce::FontOptions ((float) getHeight() * 0.24f)));
            g.drawText (blurb, parts.blurb, juce::Justification::centredLeft);
        }
    }

private:
    static constexpr int versionWidth   = 140;
    static constexpr int presetBarWidth = 210;
    static constexpr int minBlurbWidth  = 60;

    juce::Font nameFont() const
    {
        return juce::Font (juce::FontOptions ((float) getHeight() * 0.48f, juce::Font::bold));
    }

    struct Layout { juce::Rectangle<int> logo, name, preset, version, blurb; };

    /** Left to right: logo, name, description; at the right end the preset
        bar, then the version left of it. The bar comes first, then the
        version, then the description, each only if there is room. */
    Layout layout() const
    {
        Layout parts;
        auto area = getLocalBounds().reduced (10, 5);

        if (logo.isValid())
        {
            parts.logo = area.removeFromLeft (area.getHeight());
            area.removeFromLeft (10);
        }

        const int nameWidth = juce::GlyphArrangement::getStringWidthInt (nameFont(), name) + 8;
        parts.name = area.removeFromLeft (nameWidth);

        if (presetBar != nullptr)
        {
            area.removeFromLeft (6);
            const int w = juce::jmin (presetBarWidth, area.getWidth());
            parts.preset = area.removeFromRight (w).withSizeKeepingCentre (w, juce::jmin (24, area.getHeight()));
            area.removeFromRight (10);
        }

        if (area.getWidth() >= versionWidth)
            parts.version = area.removeFromRight (versionWidth);

        area.removeFromLeft (10);
        if (area.getWidth() >= minBlurbWidth)
            parts.blurb = area;

        return parts;
    }

    juce::String name, blurb;
    juce::Colour accent;
    juce::Image logo;
    std::unique_ptr<fxme::PresetBarComponent> presetBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBar)
};

} // namespace fxmefx
