/*------------------------------------------------------------------------
Copyright (C) 2001-2023  Sophia Poirier

This file is part of Polarizer.

Polarizer is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Polarizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Polarizer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "polarizereditor.h"

#include <cmath>

#include "dfxmisc.h"
#include "polarizer.h"


//-----------------------------------------------------------------------------
enum
{
	// positions
	kSliderFrameThickness = 4,
	kSliderX = 67 - kSliderFrameThickness,
	kSliderY = 70 - kSliderFrameThickness,
	kSliderInc = 118,

	kDisplayX = kSliderX - 32 + kSliderFrameThickness,
	kDisplayY = kSliderY + 221,
	kDisplayWidth = 110,
	kDisplayHeight = 17,

	kImplodeButtonX = kSliderX + (kSliderInc*2),
	kImplodeButtonY = kSliderY,

	kDestroyFXLinkX = 40,
	kDestroyFXLinkY = 322
};


constexpr auto kValueTextFont = dfx::kFontName_BoringBoron;
constexpr float kValueTextSize = 16.8f;



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

static bool leapDisplayProc(float inValue, char* outText, void*)
{
	auto const value_i = static_cast<long>(inValue);
	bool const success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld sample", value_i) > 0;
	if (success && (std::abs(value_i) > 1))
	{
		dfx::StrlCat(outText, "s", DGTextDisplay::kTextMaxLength);
	}
	return success;
}

static bool amountDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue * 10.0f) > 0;
}

static float amountValueFromTextConvertProc(float inValue, DGTextDisplay*)
{
	return inValue / 10.0f;
}


//-----------------------------------------------------------------------------
class PolarizerSlider final : public DGSlider
{
public:
	PolarizerSlider(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion,
					DGImage* inHandleImage, DGImage* inBackgroundImage)
	:	DGSlider(inOwnerEditor, inParameterID, inRegion, dfx::kAxis_Vertical, nullptr, inBackgroundImage),
		mHandleImage(inHandleImage)  // store handle image independently so that CSlider does not base control range on it
	{
		setOffsetHandle(VSTGUI::CPoint(0, kSliderFrameThickness));
		setViewSize(inRegion, false);  // HACK to trigger a recalculation of the slider range based on new handle offset
	}

	void draw(VSTGUI::CDrawContext* inContext) override
	{
		if (auto const image = getDrawBackground())
		{
			image->draw(inContext, getViewSize());
		}

		if (mHandleImage)
		{
			auto const yoff = std::round(static_cast<float>(mHandleImage->getHeight()) * (1.0f - getValueNormalized())) + kSliderFrameThickness;
			mHandleImage->draw(inContext, getViewSize(), VSTGUI::CPoint(-kSliderFrameThickness, -yoff));

			DGRect bottomBorderRect(getViewSize());
			bottomBorderRect.setSize(mHandleImage->getWidth(), kSliderFrameThickness);
			bottomBorderRect.offset(kSliderFrameThickness, getViewSize().getHeight() - kSliderFrameThickness);
			inContext->setFillColor(DGColor::kBlack);
			inContext->drawRect(bottomBorderRect, VSTGUI::kDrawFilled);
		}

#ifdef TARGET_API_RTAS
		getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

		setDirty(false);
	}

	CLASS_METHODS(PolarizerSlider, DGSlider)

private:
	VSTGUI::SharedPointer<DGImage> const mHandleImage;
};


//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(PolarizerEditor)

//-----------------------------------------------------------------------------
void PolarizerEditor::OpenEditor()
{
	// load some graphics

	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const sliderBackgroundImage = LoadImage("slider-background.png");

	auto const implodeButtonImage = LoadImage("implode-button.png");
	auto const destroyFXLinkButtonImage = LoadImage("destroy-fx-link.png");


	DGRect pos;

	//--create the sliders---------------------------------

	// leap size
	pos.set(kSliderX, kSliderY, sliderBackgroundImage->getWidth(), sliderBackgroundImage->getHeight());
	emplaceControl<PolarizerSlider>(this, kSkip, pos, sliderHandleImage, sliderBackgroundImage);

	// polarization amount
	pos.offset(kSliderInc, 0);
	emplaceControl<PolarizerSlider>(this, kAmount, pos, sliderHandleImage, sliderBackgroundImage);


	//--create the displays---------------------------------------------

	// leap size read-out
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSkip, pos, leapDisplayProc, nullptr, nullptr, dfx::TextAlignment::Center,
								  kValueTextSize, DGColor::kBlack, kValueTextFont);

	// polarization amount read-out
	pos.offset(kSliderInc, 0);
	auto const amountDisplay = emplaceControl<DGTextDisplay>(this, kAmount, pos, amountDisplayProc, nullptr, nullptr,
															 dfx::TextAlignment::Center, kValueTextSize,
															 DGColor::kBlack, kValueTextFont);
	amountDisplay->setValueFromTextConvertProc(amountValueFromTextConvertProc);


	//--create the buttons----------------------------------------------

	// IMPLODE
	emplaceControl<DGToggleImageButton>(this, kImplode, kImplodeButtonX, kImplodeButtonY, implodeButtonImage, true);

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkButtonImage->getWidth(), destroyFXLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkButtonImage, DESTROYFX_URL);
}
