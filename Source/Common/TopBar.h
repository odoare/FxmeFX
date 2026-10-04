/*
  ==============================================================================

    TopBar.h

    Shared plugin header bar for the standalone VST3/AU FxmeFX plugins: dark
    background, the FX-Mechanics logo, the plugin name, then
    "v<version> - FX-Mechanics" (the company name a link to fx-mechanics.com,
    underlined under the pointer), and the preset bar at the right end, with
    an accent hairline in the plugin's own base colour. The same in every
    plugin of the collection.

    Deliberately lives outside the embeddable *Component classes — those are
    shared with the FX-Mechanics host bundle, which draws its own chrome.
    Only each plugin's PluginEditor (the VST3/AU/Standalone wrapper) uses it.

    setPresetBank() adds the effect's preset bar (FxmeTools module presets,
    shared with every plugin that embeds the effect) at the right end, with
    its "..." browser. In a narrow window the bar narrows (down to 150 px)
    to keep the version line; only below that does the version give way.

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
    TopBar (juce::String pluginName, juce::Colour accentColour)
        : name (std::move (pluginName)), accent (accentColour)
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

        // "v<version> - FX-Mechanics", the company name a link (brighter and
        // underlined under the pointer).
        companyHit = {};
        if (! parts.version.isEmpty())
        {
            const auto font = versionFont();
            g.setFont (font);
            auto r = parts.version;

            g.setColour (juce::Colours::lightgrey);
            g.drawText (versionPrefix(), r.removeFromLeft (textWidth (font, versionPrefix())),
                        juce::Justification::centredLeft, false);

            const int companyW = textWidth (font, companyName);
            auto company = r.removeFromLeft (companyW);
            companyHit = company.withSizeKeepingCentre (companyW, (int) font.getHeight() + 4);

            g.setColour (companyHot ? juce::Colours::white : juce::Colours::lightgrey);
            g.drawText (companyName, company, juce::Justification::centredLeft, false);
            if (companyHot)
                g.fillRect (company.getX(), company.getCentreY() + (int) (font.getHeight() * 0.5f),
                            companyW - 1, 1);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const bool over = companyHit.contains (e.getPosition());
        if (over != companyHot)
        {
            companyHot = over;
            repaint (companyHit.expanded (2));
        }
        setMouseCursor (over ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (companyHot)
        {
            companyHot = false;
            repaint (companyHit.expanded (2));
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (companyHit.contains (e.getPosition()))
            juce::URL (companyUrl).launchInDefaultBrowser();
    }

private:
    static constexpr const char* companyName = "FX-Mechanics";
    static constexpr const char* companyUrl  = "https://fx-mechanics.com";
    static constexpr int presetBarWidth    = 210;
    static constexpr int minPresetBarWidth = 150;   // it shrinks this far before the version goes

    static juce::String versionPrefix()  { return "v" FXMEFX_VERSION_STRING " - "; }

    static int textWidth (const juce::Font& f, const juce::String& t)
    {
        return juce::GlyphArrangement::getStringWidthInt (f, t) + 1;
    }

    juce::Font nameFont() const
    {
        return juce::Font (juce::FontOptions ((float) getHeight() * 0.48f, juce::Font::bold));
    }

    static juce::Font versionFont() { return juce::Font (juce::FontOptions (12.0f)); }

    struct Layout { juce::Rectangle<int> logo, name, version, preset; };

    /** Left to right: logo, name, "v<version> - FX-Mechanics"; the preset bar
        at the right end. In a narrow window the bar gives up width (down to
        minPresetBarWidth) before the version line goes. */
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
        area.removeFromLeft (10);

        const auto font = versionFont();
        const int versionW = textWidth (font, versionPrefix()) + textWidth (font, companyName);
        constexpr int gap = 12;

        int barW = 0;
        if (presetBar != nullptr)
        {
            const int roomForBar = area.getWidth() - versionW - gap;
            barW = roomForBar >= minPresetBarWidth ? juce::jmin (presetBarWidth, roomForBar)
                                                   : juce::jmin (presetBarWidth, area.getWidth());
            parts.preset = area.removeFromRight (barW)
                               .withSizeKeepingCentre (barW, juce::jmin (24, area.getHeight()));
            area.removeFromRight (gap);
        }

        if (area.getWidth() >= versionW)
            parts.version = area.removeFromLeft (versionW);

        return parts;
    }

    juce::String name;
    juce::Colour accent;
    juce::Image logo;
    std::unique_ptr<fxme::PresetBarComponent> presetBar;
    juce::Rectangle<int> companyHit;   // as last painted
    bool companyHot = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBar)
};

} // namespace fxmefx
