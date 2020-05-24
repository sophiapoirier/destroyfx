/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Sophia Poirier

This file is part of EQ Sync.

EQ Sync is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

EQ Sync is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with EQ Sync.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "eqsynceditor.h"

#include "dfx-au-utilities.h"
#include "dfxmisc.h"
#include "eqsync.h"


//-----------------------------------------------------------------------------
enum
{
	// positions
	kWideFaderX = 138 - 2,
	kWideFaderY = 68 - 7,
	kWideFaderX_Panther = 141 - 2,
	kWideFaderY_Panther = 64 - 7,
	kWideFaderInc = 40,

	kTallFaderX = 138 - 7,
	kTallFaderY = 196 - 2,
	kTallFaderX_Panther = 141 - 7,
	kTallFaderY_Panther = 192 - 2,
	kTallFaderInc = 48,

	kDisplayOffsetX = 212,
	kDisplayOffsetY = 2,
	kDisplayWidth = 81,
	kDisplayHeight = 12,

	kHostSyncButtonX = 53,
	kHostSyncButtonY = 164,
	kHostSyncButtonX_Panther = 56,
	kHostSyncButtonY_Panther = 160,
	
	kDestroyFXLinkX = 158,
	kDestroyFXLinkY = 12,
	kDestroyFXLinkX_Panther = 159,
	kDestroyFXLinkY_Panther = 11,

	kHelpButtonX = 419,
	kHelpButtonY = 293,
	kHelpButtonX_Panther = 422,
	kHelpButtonY_Panther = 289
};


constexpr char const* const kValueTextFont = "Lucida Grande";
constexpr float kValueTextSize = 13.0f;
constexpr float kUnusedControlAlpha = 0.45f;



//-----------------------------------------------------------------------------
class EQSyncSlider final : public DGSlider
{
public:
	EQSyncSlider(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
				 dfx::Axis inOrientation, DGImage* inHandle, DGImage* inHandleClicked, DGImage* inBackground)
	:	DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inHandle, inBackground), 
		mRegularHandle(inHandle), mClickedHandle(inHandleClicked)
	{
	}

	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override
	{
		setHandle(mClickedHandle);
		redraw();  // ensure that the change in slider handle is reflected
		return DGSlider::onMouseDown(inPos, inButtons);
	}

	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override
	{
		setHandle(mRegularHandle);
		redraw();
		return DGSlider::onMouseUp(inPos, inButtons);
	}

	VSTGUI::CMouseEventResult onMouseCancel() override
	{
		setHandle(mRegularHandle);
		redraw();
		return DGSlider::onMouseCancel();
	}

private:
	VSTGUI::SharedPointer<DGImage> const mRegularHandle;
	VSTGUI::SharedPointer<DGImage> const mClickedHandle;
};



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(EQSyncEditor)

//-----------------------------------------------------------------------------
EQSyncEditor::EQSyncEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long EQSyncEditor::OpenEditor()
{
	auto wideFaderX = kWideFaderX_Panther;
	auto wideFaderY = kWideFaderY_Panther;
	auto tallFaderX = kTallFaderX_Panther;
	auto tallFaderY = kTallFaderY_Panther;
	auto hostSyncButtonX = kHostSyncButtonX_Panther;
	auto hostSyncButtonY = kHostSyncButtonY_Panther;
	auto destroyFXLinkX = kDestroyFXLinkX_Panther;
	auto destroyFXLinkY = kDestroyFXLinkY_Panther;
	auto helpButtonX = kHelpButtonX_Panther;
	auto helpButtonY = kHelpButtonY_Panther;

	VSTGUI::SharedPointer<DGImage> horizontalSliderBackgroundImage;
	VSTGUI::SharedPointer<DGImage> verticalSliderBackgroundImage;
	VSTGUI::SharedPointer<DGImage> sliderHandleImage;
	VSTGUI::SharedPointer<DGImage> sliderHandleClickedImage;
	VSTGUI::SharedPointer<DGImage> hostSyncButtonImage;
	VSTGUI::SharedPointer<DGImage> destroyFXLinkTabImage;

	auto const macOS = GetMacOSVersion() & 0xFFF0;
	switch (macOS)
	{
		// Jaguar (Mac OS X 10.2)
		case 0x1020:
			wideFaderX = kWideFaderX;
			wideFaderY = kWideFaderY;
			tallFaderX = kTallFaderX;
			tallFaderY = kTallFaderY;
			hostSyncButtonX = kHostSyncButtonX;
			hostSyncButtonY = kHostSyncButtonY;
			destroyFXLinkX = kDestroyFXLinkX;
			destroyFXLinkY = kDestroyFXLinkY;
			helpButtonX = kHelpButtonX;
			helpButtonY = kHelpButtonY;
//			backgroundImage = VSTGUI::makeOwned<DGImage>("eq-sync-background.png");
			horizontalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("horizontal-slider-background.png");
			verticalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("vertical-slider-background.png");
			sliderHandleImage = VSTGUI::makeOwned<DGImage>("slider-handle.png");
			sliderHandleClickedImage = VSTGUI::makeOwned<DGImage>("slider-handle-clicked.png");
			hostSyncButtonImage = VSTGUI::makeOwned<DGImage>("host-sync-button-panther.png");  // it's the same widget image in Panther and Jaguar
			destroyFXLinkTabImage = VSTGUI::makeOwned<DGImage>("destroy-fx-link-tab.png");
			break;
		// Panther (Mac OS X 10.3)
		case 0x1030:
		default:
//			backgroundImage = VSTGUI::makeOwned<DGImage>("eq-sync-background-panther.png");
			horizontalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("horizontal-slider-background-panther.png");
			verticalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("vertical-slider-background-panther.png");
			sliderHandleImage = VSTGUI::makeOwned<DGImage>("slider-handle-panther.png");
			sliderHandleClickedImage = VSTGUI::makeOwned<DGImage>("slider-handle-clicked-panther.png");
			hostSyncButtonImage = VSTGUI::makeOwned<DGImage>("host-sync-button-panther.png");
			destroyFXLinkTabImage = VSTGUI::makeOwned<DGImage>("destroy-fx-link-tab-panther.png");
			break;
	}

	auto const helpButtonImage = VSTGUI::makeOwned<DGImage>("help-button.png");


	DGRect pos;

	for (long i = kRate_Sync; i <= kTempo; i++)
	{
		// create the horizontal sliders
		pos.set(wideFaderX, wideFaderY + (kWideFaderInc * i), horizontalSliderBackgroundImage->getWidth(), horizontalSliderBackgroundImage->getHeight());
		emplaceControl<EQSyncSlider>(this, i, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderHandleClickedImage, horizontalSliderBackgroundImage);

		// create the displays
		VSTGUI::CParamDisplayValueToStringProc textProc = nullptr;
		if (i == kRate_Sync)
		{
			textProc = [](float inValue, char* outText, void* inEditor)
			{
				return static_cast<DfxGuiEditor*>(inEditor)->getparametervaluestring(kRate_Sync, outText);
			};
		}
		else if (i == kSmooth)
		{
			textProc = [](float inValue, char* outText, void*)
			{
				return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f%%", inValue) > 0;
			};
		}
		else if (i == kTempo)
		{
			textProc = [](float inValue, char* outText, void*)
			{
				return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f", inValue) > 0;
			};
		}
		pos.set(wideFaderX + kDisplayOffsetX, wideFaderY + kDisplayOffsetY + (kWideFaderInc * i), kDisplayWidth, kDisplayHeight);
		emplaceControl<DGTextDisplay>(this, i, pos, textProc, this, nullptr, dfx::TextAlignment::Left, kValueTextSize, DGColor::kBlack, kValueTextFont);
	}

	// create the vertical sliders
	for (long i = kA0; i <= kB2; i++)
	{
		pos.set(tallFaderX + (kTallFaderInc * (i - kA0)), tallFaderY, verticalSliderBackgroundImage->getWidth(), verticalSliderBackgroundImage->getHeight());
		emplaceControl<EQSyncSlider>(this, i, pos, dfx::kAxis_Vertical, sliderHandleImage, sliderHandleClickedImage, verticalSliderBackgroundImage);
	}


	// create the host sync button
	pos.set(hostSyncButtonX, hostSyncButtonY, hostSyncButtonImage->getWidth() / 2, hostSyncButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kTempoAuto, pos, hostSyncButtonImage, DGButton::Mode::Increment, true);

	// create the Destroy FX web page link tab
	pos.set(destroyFXLinkX, destroyFXLinkY, destroyFXLinkTabImage->getWidth(), destroyFXLinkTabImage->getHeight() / 2);
	auto const dfxWebLink = emplaceControl<DGWebLink>(this, pos, destroyFXLinkTabImage, DESTROYFX_URL);
	auto const dfxWebLinkTruncation = (pos.getWidth() / 2) - 9;
	auto dfxWebLinkMouseableRegion = pos;
	dfxWebLinkMouseableRegion.setWidth(dfxWebLinkMouseableRegion.getWidth() - dfxWebLinkTruncation);
	dfxWebLinkMouseableRegion.offset(dfxWebLinkTruncation, 0);
	dfxWebLink->setMouseableArea(dfxWebLinkMouseableRegion);

	// create the help button
	pos.set(helpButtonX, helpButtonY, helpButtonImage->getWidth(), helpButtonImage->getHeight() / 2);
	auto const helpButton = emplaceControl<DGButton>(this, pos, helpButtonImage, 2, DGButton::Mode::Momentary);
	helpButton->setUserReleaseProcedure([](long, void*)
	{
		dfx::LaunchDocumentation();
	}, this, true);


	HandleTempoAutoChange();


	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void EQSyncEditor::parameterChanged(long inParameterID)
{
	if (inParameterID == kTempoAuto)
	{
		HandleTempoAutoChange();
	}
}

//-----------------------------------------------------------------------------
void EQSyncEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.0f;
	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == kTempo)
		{
			control->setDrawAlpha(alpha);
		}
	}
}
