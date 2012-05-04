/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2011  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "dfxguicontrol.h"
#include "dfxguieditor.h"

#include "dfxpluginproperties.h"
#include "dfxmath.h"
#include "dfxplugin.h"	// XXX for launch_url(), but should move that elsewhere?



#ifdef TARGET_PLUGIN_USES_VSTGUI
#else



const SInt32 kContinuousControlMaxValue = 0x0FFFFFFF - 1;
#if TARGET_OS_MAC
	const MenuID kDfxGui_ControlContextualMenuID = 3;	// XXX eh?  how am I supposed to come up with a meaningful 16-bit value for this?
#endif


#ifndef DFXGUI_USE_CONTEXTUAL_MENU
	#define DFXGUI_USE_CONTEXTUAL_MENU	1
#endif



#pragma mark -
#pragma mark DGControl

//-----------------------------------------------------------------------------
#ifdef TARGET_API_RTAS
#if 0	// XXX do this?
long CControl::kZoomModifier = kControl;
long CControl::kDefaultValueModifier = kShift;
#endif
#endif

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion)
:	CControl(*inRegion, inOwnerEditor, inParamID)
{
#ifdef TARGET_API_AUDIOUNIT
	auvp = CAAUParameter(ownerEditor->GetEditAudioUnit(), (AudioUnitParameterID)inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
	parameterAttached = true;
	valueRange = auvp.ParamInfo().maxValue - auvp.ParamInfo().minValue;
#else
	valueRange = 1.0f;	// XXX implement
#endif
	
	init(inRegion);
}

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange)
:	CControl(*inRegion, inOwnerEditor, DFX_PARAM_INVALID_ID), 
	valueRange(inRange)
{
#ifdef TARGET_API_AUDIOUNIT
	auvp = CAAUParameter();	// an empty CAAUParameter
	parameterAttached = false;
#else
#endif

	init(inRegion);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGControl::init(DGRect * inRegion)
{
	controlPos = *inRegion;
	vizArea = *inRegion;

	isContinuous = false;
	fineTuneFactor = kDfxGui_DefaultFineTuneFactor;
	mouseDragRange = kDfxGui_DefaultMouseDragRange;

	shouldRespondToMouse = true;
	shouldRespondToMouseWheel = true;
	currentlyIgnoringMouseTracking = false;
	shouldWraparoundValues = false;

	drawAlpha = 1.0f;

	// add this control to the owner editor's list of controls
	if (getDfxGuiEditor() != NULL)
		getDfxGuiEditor()->addControl(this);
}

//-----------------------------------------------------------------------------
DGControl::~DGControl()
{
}


//-----------------------------------------------------------------------------
// force a redraw
void DGControl::redraw()
{
	invalid();
}

//-----------------------------------------------------------------------------
void DGControl::setControlContinuous(bool inContinuity)
{
	bool oldContinuity = isContinuous;
	isContinuous = inContinuity;
	if (inContinuity != oldContinuity)
	{
		// XXX do anything?
	}
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	if (! getRespondToMouse() )
		return;

	currentlyIgnoringMouseTracking = false;

	// do this to make touch automation work
	// AUCarbonViewControl::HandleEvent will catch ControlClick but not ControlContextualMenuClick
	if ( isParameterAttached() )//&& (inEventKind == kEventControlContextualMenuClick) )
		getDfxGuiEditor()->automationgesture_begin( getParameterID() );

	#if TARGET_PLUGIN_USES_MIDI
		if ( isParameterAttached() )
			getDfxGuiEditor()->setmidilearner( getParameterID() );
	#endif

	// set the default value of the parameter
	if ( (inKeyModifiers & kDGKeyModifier_accel) && isParameterAttached() )
	{
		getDfxGuiEditor()->setparameter_default( getParameterID() );
		currentlyIgnoringMouseTracking = true;
		return;
	}

	mouseDown(inXpos, inYpos, inMouseButtons, inKeyModifiers, inIsDoubleClick);
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	if (! getRespondToMouse() )
		return;

	if (!currentlyIgnoringMouseTracking)
		mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	if (! getRespondToMouse() )
		return;

	if (!currentlyIgnoringMouseTracking)
		mouseUp(inXpos, inYpos, inKeyModifiers);

	currentlyIgnoringMouseTracking = false;

	// do this to make touch automation work
	if ( isParameterAttached() )
		getDfxGuiEditor()->automationgesture_end( getParameterID() );
}

//-----------------------------------------------------------------------------
bool DGControl::do_mouseWheel(long inDelta, DGAxis inAxis, DGKeyModifiers inKeyModifiers)
{
	if (! getRespondToMouseWheel() )
		return false;
	if (! getRespondToMouse() )
		return false;

	if ( isParameterAttached() )
		getDfxGuiEditor()->automationgesture_begin( getParameterID() );

	bool wheelResult = mouseWheel(inDelta, inAxis, inKeyModifiers);

	if ( isParameterAttached() )
		getDfxGuiEditor()->automationgesture_end( getParameterID() );

	return wheelResult;
}

//-----------------------------------------------------------------------------
// a default implementation of mouse wheel handling that should work for most controls
bool DGControl::mouseWheel(long inDelta, DGAxis inAxis, DGKeyModifiers inKeyModifiers)
{
ControlRef carbonControl = NULL;	// XXX just quieting errors for now
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 oldValue = GetControl32BitValue(carbonControl);
	SInt32 newValue = oldValue;

	if ( isContinuousControl() )
	{
		float diff = (float)inDelta;
		if (inKeyModifiers & kDGKeyModifier_shift)	// slo-mo
			diff /= getFineTuneFactor();
		newValue = oldValue + (SInt32)(diff * (float)(max-min) / getMouseDragRange());
	}
	else
	{
		if (inDelta > 0)
			newValue = oldValue + 1;
		else if (inDelta < 0)
			newValue = oldValue - 1;

		// wrap around
		if ( getWraparoundValues() )
		{
			if (newValue > max)
				newValue = min;
			else if (newValue < min)
				newValue = max;
		}
	}

	if (newValue > max)
		newValue = max;
	if (newValue < min)
		newValue = min;
	if (newValue != oldValue)
		SetControl32BitValue(carbonControl, newValue);

	return true;
}

//-----------------------------------------------------------------------------
enum {
	kDfxContextualMenuItem_Global_Undo = 0,
	kDfxContextualMenuItem_Global_SetDefaultParameterValues,
	kDfxContextualMenuItem_Global_RandomizeParameterValues,
	kDfxContextualMenuItem_Global_GenerateAutomationSnapshot,
	kDfxContextualMenuItem_Global_CopyState,
	kDfxContextualMenuItem_Global_PasteState,
	kDfxContextualMenuItem_Global_SavePresetFile,
	kDfxContextualMenuItem_Global_LoadPresetFile,
//	kDfxContextualMenuItem_Global_UserPresets,	// preset files sub-menu(s)?
//	kDfxContextualMenuItem_Global_FactoryPresets,	// factory presets sub-menu?
#if !TARGET_PLUGIN_IS_INSTRUMENT
//	kDfxContextualMenuItem_Global_Bypass,	// effect bypass?
#endif
#if TARGET_PLUGIN_USES_MIDI
	kDfxContextualMenuItem_Global_MidiLearn,
	kDfxContextualMenuItem_Global_MidiReset,
#endif
//	kDfxContextualMenuItem_Global_ResizeGUI,
	kDfxContextualMenuItem_Global_WindowTransparency,
	kDfxContextualMenuItem_Global_OpenWebSite,
	kDfxContextualMenuItem_Global_NumItems
};
enum {
	kDfxContextualMenuItem_Parameter_SetDefaultValue = 0,
	kDfxContextualMenuItem_Parameter_TextEntryForValue,	// type in value
//	kDfxContextualMenuItem_Parameter_ValueStrings,	// a sub-menu of value selections, for parameters that have them
	kDfxContextualMenuItem_Parameter_Undo,
	kDfxContextualMenuItem_Parameter_RandomizeParameterValue,
	kDfxContextualMenuItem_Parameter_GenerateAutomationSnapshot,
//	kDfxContextualMenuItem_Parameter_SnapMode,	// toggle snap mode
#if TARGET_PLUGIN_USES_MIDI
	kDfxContextualMenuItem_Parameter_MidiLearner,
	kDfxContextualMenuItem_Parameter_MidiUnassign,
	kDfxContextualMenuItem_Parameter_TextEntryForMidiCC,	// assign MIDI CC by typing in directly
#endif
	kDfxContextualMenuItem_Parameter_NumItems
};
const UInt32 kDfxMenu_MenuItemIndentAmount = 1;

//-----------------------------------------------------------------------------
OSStatus DFX_AppendSeparatorToMenu(MenuRef inMenu)
{
	if (inMenu == NULL)
		return paramErr;
	return AppendMenuItemTextWithCFString(inMenu, NULL, kMenuItemAttrSeparator, 0, NULL);
}

//-----------------------------------------------------------------------------
OSStatus DFX_AppendSectionTitleToMenu(MenuRef inMenu, CFStringRef inMenuItemText)
{
	if ( (inMenu == NULL) || (inMenuItemText == NULL) )
		return paramErr;

	MenuItemIndex menuItemIndex = 0;
	OSStatus status = AppendMenuItemTextWithCFString(inMenu, inMenuItemText, kMenuItemAttrSectionHeader, 0, &menuItemIndex);
	if (status == noErr)
	{
//		status = SetMenuItemIndent(inMenu, menuItemIndex, kDfxMenu_MenuItemIndentAmount);
		SetItemStyle(inMenu, menuItemIndex, italic | bold);// | underline);
	}
	return status;
}

//-----------------------------------------------------------------------------
int DFX_CFStringScanWithFormat(CFStringRef inString, const char * inFormat, ...)
{
	if ( (inString == NULL) || (inFormat == NULL) )
		return 0;

	int scanCount = 0;

	char * cString = DFX_CreateCStringFromCFString(inString, kCFStringEncodingUTF8);
	if (cString != NULL)
	{
		va_list variableArgumentList;
		va_start(variableArgumentList, inFormat);
		scanCount = vsscanf(cString, inFormat, variableArgumentList);
		va_end(variableArgumentList);
		free(cString);
	}

	return scanCount;
}

//-----------------------------------------------------------------------------
bool DGControl::do_contextualMenuClick()
{
#if DFXGUI_USE_CONTEXTUAL_MENU
	return contextualMenuClick();
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
bool DGControl::contextualMenuClick()
{
#if TARGET_OS_MAC && DFXGUI_USE_CONTEXTUAL_MENU
	MenuRef contextualMenu = NULL;
	MenuID contextualMenuID = kDfxGui_ControlContextualMenuID;
	MenuAttributes contextualMenuAttributes = kMenuAttrCondenseSeparators;
	OSStatus status = CreateNewMenu(contextualMenuID, contextualMenuAttributes, &contextualMenu);
	if ( (status != noErr) || (contextualMenu == NULL) )
		return false;


// --------- parameter menu creation ---------
	bool parameterMenuItemsWereAdded = false;
	DGControl * dgControl = NULL;
	// populate the parameter-specific section of the menu
	if (getType() == kDGControlType_SubControl)
	{
		dgControl = (DGControl*)this;
		if ( dgControl->isParameterAttached() )
		{
			parameterMenuItemsWereAdded = true;
			MenuItemIndex menuItemIndex = 0;
			long paramID = dgControl->getParameterID();

			// preface the parameter-specific commands section with a header title
			CFStringRef menuItemText = CFSTR("Parameter Options");
			status = DFX_AppendSectionTitleToMenu(contextualMenu, menuItemText);
			status = DFX_AppendSeparatorToMenu(contextualMenu);

			for (UInt32 i=0; i < kDfxContextualMenuItem_Parameter_NumItems; i++)
			{
				bool showCheckmark = false;
				bool disableItem = false;
				bool isFirstItemOfSubgroup = false;
				menuItemText = NULL;
				CFStringRef menuItemText_temp = NULL;
				switch (i)
				{
					case kDfxContextualMenuItem_Parameter_SetDefaultValue:
						menuItemText = CFSTR("Set default value");
						isFirstItemOfSubgroup = true;
						break;
					case kDfxContextualMenuItem_Parameter_TextEntryForValue:
						menuItemText = CFSTR("Type in a value for this parameter...");
						break;
					case kDfxContextualMenuItem_Parameter_Undo:
						if (true)	//XXX needs appropriate check for which text/function to use
							menuItemText = CFSTR("Undo this parameter");
						else
							menuItemText = CFSTR("Redo this parameter");
						break;
					case kDfxContextualMenuItem_Parameter_RandomizeParameterValue:
						menuItemText = CFSTR("Randomize value");
						break;
					case kDfxContextualMenuItem_Parameter_GenerateAutomationSnapshot:
						menuItemText = CFSTR("Generate parameter automation snapshot");
						break;
#if TARGET_PLUGIN_USES_MIDI
					case kDfxContextualMenuItem_Parameter_MidiLearner:
						menuItemText = CFSTR("MIDI learner");
						if ( (getDfxGuiEditor()->getmidilearning()) )
							showCheckmark = getDfxGuiEditor()->ismidilearner(paramID);
						else
							disableItem = true;
						isFirstItemOfSubgroup = true;
						break;
					case kDfxContextualMenuItem_Parameter_MidiUnassign:
						{
							menuItemText = CFSTR("Unassign MIDI");
							DfxParameterAssignment currentParamAssignment = getDfxGuiEditor()->getparametermidiassignment(paramID);
							// disable if not assigned
							if (currentParamAssignment.eventType == kParamEventNone)
								disableItem = true;
							// append the current MIDI assignment, if there is one, to the menu item text
							else
							{
								switch (currentParamAssignment.eventType)
								{
									case kParamEventCC:
										menuItemText_temp = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ (CC %ld)"), menuItemText, currentParamAssignment.eventNum);
										break;
									case kParamEventPitchbend:
										menuItemText_temp = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ (pitchbend)"), menuItemText);
										break;
									case kParamEventNote:
										menuItemText_temp = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ (notes %s - %s)"), menuItemText, DFX_GetNameForMIDINote(currentParamAssignment.eventNum), DFX_GetNameForMIDINote(currentParamAssignment.eventNum2));
										break;
									default:
										break;
								}
								if (menuItemText_temp != NULL)
									menuItemText = menuItemText_temp;
							}
						}
						break;
					case kDfxContextualMenuItem_Parameter_TextEntryForMidiCC:
						menuItemText = CFSTR("Type in a MIDI CC assignment...");
						break;
#endif
					default:
						break;
				}
				if (isFirstItemOfSubgroup)
					status = DFX_AppendSeparatorToMenu(contextualMenu);
				if (menuItemText != NULL)
				{
					MenuItemAttributes menuItemAttributes = 0;
					if (disableItem)
						menuItemAttributes |= kMenuItemAttrDisabled;
					MenuCommand menuItemCommandID = i + kDfxContextualMenuItem_Global_NumItems;
					status = AppendMenuItemTextWithCFString(contextualMenu, menuItemText, menuItemAttributes, menuItemCommandID, &menuItemIndex);
					if (status == noErr)
					{
						if (showCheckmark)
							CheckMenuItem(contextualMenu, menuItemIndex, true);
						status = SetMenuItemIndent(contextualMenu, menuItemIndex, kDfxMenu_MenuItemIndentAmount);
					}
				}
				if (menuItemText_temp != NULL)
					CFRelease(menuItemText_temp);
			}

			// preface the global commands section with a divider...
			status = DFX_AppendSeparatorToMenu(contextualMenu);
			// ... and a header title
			menuItemText = CFSTR("General Options");
			status = DFX_AppendSectionTitleToMenu(contextualMenu, menuItemText);
			status = DFX_AppendSeparatorToMenu(contextualMenu);
		}
	}

// --------- global menu creation ---------
	// populate the global section of the menu
	for (UInt32 i=0; i < kDfxContextualMenuItem_Global_NumItems; i++)
	{
		MenuItemIndex menuItemIndex = 0;
		bool showCheckmark = false;
		bool disableItem = false;
		bool isFirstItemOfSubgroup = false;
		CFStringRef menuItemText = NULL;
		switch (i)
		{
			case kDfxContextualMenuItem_Global_Undo:
				if (true)	//XXX needs appropriate check for which text/function to use
					menuItemText = CFSTR("Undo");
				else
					menuItemText = CFSTR("Redo");
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Global_SetDefaultParameterValues:
				menuItemText = CFSTR("Reset all parameter values to default");
				break;
			case kDfxContextualMenuItem_Global_RandomizeParameterValues:
				menuItemText = CFSTR("Randomize all parameter values");
				break;
			case kDfxContextualMenuItem_Global_GenerateAutomationSnapshot:
				menuItemText = CFSTR("Generate parameter automation snapshot");
				break;
			case kDfxContextualMenuItem_Global_CopyState:
				menuItemText = CFSTR("Copy settings");
				isFirstItemOfSubgroup = true;
				// the Pasteboard Manager API is only available in Mac OS X 10.3 or higher
				if (PasteboardCreate == NULL)
					disableItem = true;
				break;
			case kDfxContextualMenuItem_Global_PasteState:
				menuItemText = CFSTR("Paste settings");
				{
					bool currentClipboardIsPastable = false;
					getDfxGuiEditor()->pasteSettings(&currentClipboardIsPastable);
					if (!currentClipboardIsPastable)
						disableItem = true;
				}
				break;
			case kDfxContextualMenuItem_Global_SavePresetFile:
				menuItemText = CFSTR("Save preset file...");
				break;
			case kDfxContextualMenuItem_Global_LoadPresetFile:
				menuItemText = CFSTR("Load preset file...");
				break;
#if TARGET_PLUGIN_USES_MIDI
			case kDfxContextualMenuItem_Global_MidiLearn:
				menuItemText = CFSTR("MIDI learn");
				showCheckmark = getDfxGuiEditor()->getmidilearning();
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Global_MidiReset:
				menuItemText = CFSTR("MIDI assignments reset");
				break;
#endif
			case kDfxContextualMenuItem_Global_WindowTransparency:
				menuItemText = CFSTR("Set window transparency...");
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Global_OpenWebSite:
				menuItemText = CFSTR("Open "PLUGIN_CREATOR_NAME_STRING" web site");
				break;
			default:
				break;
		}
		if (isFirstItemOfSubgroup)
			status = DFX_AppendSeparatorToMenu(contextualMenu);
		if (menuItemText != NULL)
		{
			MenuItemAttributes menuItemAttributes = 0;//kMenuItemAttrDisabled
			if (disableItem)
				menuItemAttributes |= kMenuItemAttrDisabled;
			MenuCommand menuItemCommandID = i;
			status = AppendMenuItemTextWithCFString(contextualMenu, menuItemText, menuItemAttributes, menuItemCommandID, &menuItemIndex);
//			status = InsertMenuItemTextWithCFString(contextualMenu, menuItemText, MenuItemIndex inAfterItem, menuItemAttributes, menuItemCommandID);
			if (status == noErr)
			{
				if (showCheckmark)
					CheckMenuItem(contextualMenu, menuItemIndex, true);
				if (parameterMenuItemsWereAdded)
					status = SetMenuItemIndent(contextualMenu, menuItemIndex, kDfxMenu_MenuItemIndentAmount);
			}
		}
	}


// --------- show the contextual menu ---------
	Point mouseLocation_global_i;
	GetGlobalMouse(&mouseLocation_global_i);
	UInt32 userSelectionType = kCMNothingSelected;
	SInt16 menuID = 0;
	MenuItemIndex menuItemIndex = 0;
	status = ContextualMenuSelect(contextualMenu, mouseLocation_global_i, false, kCMHelpItemOtherHelp, "\p"PLUGIN_NAME_STRING" manual", NULL, &userSelectionType, &menuID, &menuItemIndex);

	bool result = true;
	// XXX this happens "if the user selects an item that requires no additional actions on your part", so report it as being handled?
	if ( (status == userCanceledErr) && (userSelectionType == kCMNothingSelected) )
	{
		result = true;
		goto cleanupMenu;
	}
	if (status != noErr)
	{
		result = false;
		goto cleanupMenu;
	}
	result = true;

// --------- handle the contextual menu command (global) ---------
	if (userSelectionType == kCMShowHelpSelected)
		launch_documentation();
	else if (userSelectionType == kCMMenuItemSelected)
	{
//fprintf(stderr, "menuID = %d, menuItemIndex = %u\n", menuID, menuItemIndex);
		MenuCommand menuCommandID = 0;
		status = GetMenuItemCommandID(contextualMenu, menuItemIndex, &menuCommandID);
		if (status == noErr)
		{
			bool tryHandlingParameterCommand = false;
			switch (menuCommandID)
			{
				case kDfxContextualMenuItem_Global_Undo:
					//XXX implement
					break;
				case kDfxContextualMenuItem_Global_SetDefaultParameterValues:
					getDfxGuiEditor()->setparameters_default(true);
					break;
				case kDfxContextualMenuItem_Global_RandomizeParameterValues:
					getDfxGuiEditor()->randomizeparameters(true);	// XXX "yes" to writing automation data?
					break;
				case kDfxContextualMenuItem_Global_GenerateAutomationSnapshot:
					{
						UInt32 parameterListSize = 0;
						AudioUnitParameterID * parameterList = getDfxGuiEditor()->CreateParameterList(kAudioUnitScope_Global, &parameterListSize);
						if (parameterList != NULL)
						{
							for (UInt32 i=0; i < parameterListSize; i++)
								getDfxGuiEditor()->setparameter_f(parameterList[i], getDfxGuiEditor()->getparameter_f(parameterList[i]), true);
							free(parameterList);
						}
					}
					break;
				case kDfxContextualMenuItem_Global_CopyState:
					getDfxGuiEditor()->copySettings();
					break;
				case kDfxContextualMenuItem_Global_PasteState:
					getDfxGuiEditor()->pasteSettings();
					break;
				case kDfxContextualMenuItem_Global_SavePresetFile:
					{
						CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier( CFSTR(PLUGIN_BUNDLE_IDENTIFIER) );
						if (pluginBundle != NULL)
						{
							status = SaveAUStateToPresetFile_Bundle(getDfxGuiEditor()->GetEditAudioUnit(), NULL, NULL, pluginBundle);
						}
					}
					break;
				case kDfxContextualMenuItem_Global_LoadPresetFile:
					status = CustomRestoreAUPresetFile( getDfxGuiEditor()->GetEditAudioUnit() );
					break;
#if TARGET_PLUGIN_USES_MIDI
				case kDfxContextualMenuItem_Global_MidiLearn:
					getDfxGuiEditor()->setmidilearning(! (getDfxGuiEditor()->getmidilearning()) );
					break;
				case kDfxContextualMenuItem_Global_MidiReset:
					getDfxGuiEditor()->resetmidilearn();
					break;
#endif
				case kDfxContextualMenuItem_Global_WindowTransparency:
					status = getDfxGuiEditor()->openWindowTransparencyWindow();
					break;
				case kDfxContextualMenuItem_Global_OpenWebSite:
					launch_url(PLUGIN_HOMEPAGE_URL);
					break;
				default:
					tryHandlingParameterCommand = true;
					break;
			}

// --------- handle the contextual menu command (parameter-specific) ---------
			if (tryHandlingParameterCommand && parameterMenuItemsWereAdded)
			{
				long paramID = dgControl->getParameterID();
				switch (menuCommandID - kDfxContextualMenuItem_Global_NumItems)
				{
					case kDfxContextualMenuItem_Parameter_SetDefaultValue:
						getDfxGuiEditor()->setparameter_default(paramID, true);
						break;
					case kDfxContextualMenuItem_Parameter_TextEntryForValue:
						{
							// initialize the text with the current parameter value
							CFStringRef currentValueText = dgControl->createStringFromValue();
							// XXX consider adding the min and max values to the window display for user's sake
							CFStringRef text = getDfxGuiEditor()->openTextEntryWindow(currentValueText);
							if (currentValueText != NULL)
								CFRelease(currentValueText);
							if (text != NULL)
							{
//CFShow(text);
								dgControl->setValueWithString(text);
								CFRelease(text);
							}
						}
						break;
					case kDfxContextualMenuItem_Parameter_Undo:
						//XXX implement
						break;
					case kDfxContextualMenuItem_Parameter_RandomizeParameterValue:
						// XXX "yes" to writing automation data?
						getDfxGuiEditor()->randomizeparameter(paramID, true);
						break;
					case kDfxContextualMenuItem_Parameter_GenerateAutomationSnapshot:
						getDfxGuiEditor()->setparameter_f(paramID, getDfxGuiEditor()->getparameter_f(paramID), true);
						break;
#if TARGET_PLUGIN_USES_MIDI
					case kDfxContextualMenuItem_Parameter_MidiLearner:
						if ( getDfxGuiEditor()->getmidilearner() == paramID )
							getDfxGuiEditor()->setmidilearner(kNoLearner);
						else
							getDfxGuiEditor()->setmidilearner(paramID);
						break;
					case kDfxContextualMenuItem_Parameter_MidiUnassign:
						getDfxGuiEditor()->parametermidiunassign(paramID);
						break;
					case kDfxContextualMenuItem_Parameter_TextEntryForMidiCC:
						{
							// initialize the text with the current CC assignment, if there is one
							CFStringRef initialText = NULL;
							DfxParameterAssignment currentParamAssignment = getDfxGuiEditor()->getparametermidiassignment(paramID);
							if (currentParamAssignment.eventType == kParamEventCC)
								initialText = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), currentParamAssignment.eventNum);
							CFStringRef text = getDfxGuiEditor()->openTextEntryWindow(initialText);
							if (initialText != NULL)
								CFRelease(initialText);
							if (text != NULL)
							{
								long newValue = 0;
								int scanResult = DFX_CFStringScanWithFormat(text, "%ld", &newValue);
								if (scanResult > 0)
								{
									DfxParameterAssignment newParamAssignment = {0};
									newParamAssignment.eventType = kParamEventCC;
									newParamAssignment.eventChannel = 0;	// XXX not currently implemented
									newParamAssignment.eventNum = newValue;
									getDfxGuiEditor()->setparametermidiassignment(paramID, newParamAssignment);
								}
								CFRelease(text);
							}
						}
						break;
#endif
					default:
						break;
				}
			}
		}
	}

cleanupMenu:
	DisposeMenu(contextualMenu);

	return result;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
void DGControl::setParameterID(long inParameterID)
{
	if (inParameterID == DFX_PARAM_INVALID_ID)
	{
		parameterAttached = false;
#ifdef TARGET_API_AUDIOUNIT
		if (auv_control != NULL)
			delete auv_control;
		auv_control = NULL;
#endif
	}
	else if ( !parameterAttached || (inParameterID != getParameterID()) )	// only do this if it's a change
	{
		parameterAttached = true;
#ifdef TARGET_API_AUDIOUNIT
		auvp = CAAUParameter(getDfxGuiEditor()->GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, 
							kAudioUnitScope_Global, (AudioUnitElement)0);
		createAUVcontrol();
#endif
		redraw();	// it might not happen if the new parameter value is the same as the old value, so make sure it happens
	}
}

//-----------------------------------------------------------------------------
long DGControl::getParameterID()
{
	if ( isParameterAttached() )	// XXX necessary check?
		return getTag();
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
void DGControl::setValue_i(long inValue)
{
	// XXX implement
}

//-----------------------------------------------------------------------------
long DGControl::getValue_i()
{
	// XXX implement
	return 0;
}

//-----------------------------------------------------------------------------
CFStringRef DGControl::createStringFromValue()
{
	if ( isContinuousControl() )	// XXX need a better check
	{
		double currentValue;
		if ( isParameterAttached() )
			currentValue = getDfxGuiEditor()->getparameter_f( getParameterID() );
		else
			currentValue = 0.0f;	// XXX implement
		return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%.6f"), currentValue);
	}
	else
	{
		long currentValue;
		if ( isParameterAttached() )
			currentValue = getDfxGuiEditor()->getparameter_i( getParameterID() );
		else
			currentValue = getValue_i();
		return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), currentValue);
	}
}

//-----------------------------------------------------------------------------
bool DGControl::setValueWithString(CFStringRef inString)
{
	bool success = false;

	if ( isParameterAttached() )
	{
		if ( isContinuousControl() )	// XXX need a better check
		{
//			double newValue = CFStringGetDoubleValue(inString);
			double newValue = 0.0;
			int scanResult = DFX_CFStringScanWithFormat(inString, "%lf", &newValue);
			if (scanResult > 0)
			{
				getDfxGuiEditor()->setparameter_f(getParameterID(), newValue, true);
				success = true;
			}
		}
		else
		{
			long newValue = 0;
			int scanResult = DFX_CFStringScanWithFormat(inString, "%ld", &newValue);
			if (scanResult > 0)
			{
				getDfxGuiEditor()->setparameter_i(getParameterID(), newValue, true);
				success = true;
			}
		}
	}

	return success;
}

//-----------------------------------------------------------------------------
void DGControl::setOffset(long x, long y)
{
	controlPos.offset(x, y);
	vizArea.offset(x, y);
}

//-----------------------------------------------------------------------------
void DGControl::setDrawAlpha(float inAlpha)
{
	float oldalpha = drawAlpha;
	drawAlpha = inAlpha;
	if (oldalpha != inAlpha)
		redraw();
// XXX use CBitmap::drawAlphaBlend()
}

//-----------------------------------------------------------------------------
void DGControl::setForeBounds(long x, long y, long w, long h)
{
	vizArea.set(x, y, w, h);
}

//-----------------------------------------------------------------------------
void DGControl::shrinkForeBounds(long inXoffset, long inYoffset, long inWidthShrink, long inHeightShrink)
{
	vizArea.offset(inXoffset, inYoffset);
	vizArea.setWidth( getWidth() - inWidthShrink );
	vizArea.setHeight( getHeight() - inHeightShrink );
}

//-----------------------------------------------------------------------------
bool DGControl::setHelpText(const char * inHelpText)
{
	if (inHelpText == NULL)
		return false;

	return setAttribute(kCViewTooltipAttribute, strlen(inHelpText)+1, inHelpText);
}






#pragma mark -
//-----------------------------------------------------------------------------
#if 0
void DGBackgroundControl::draw(CDrawContext * inContext)
{
	DGRect drawRect( (long)(getDfxGuiEditor()->GetXOffset()), (long)(getDfxGuiEditor()->GetYOffset()), getWidth(), getHeight() );

	// draw the background image, if there is one
	if (backgroundImage != NULL)
	{
		backgroundImage->draw(&drawRect, inContext);
	}

	if (dragIsActive)
	{
		const float dragHiliteThickness = 2.0f;	// XXX is there a proper way to query this?
		if (HIThemeSetStroke != NULL)
		{
//OSStatus status = HIThemeBrushCreateCGColor(kThemeBrushDragHilite, CGColorRef * outColor);
			OSStatus status = HIThemeSetStroke(kThemeBrushDragHilite, NULL, inContext->getPlatformGraphicsContext(), inContext->getHIThemeOrientation());
			if (status == noErr)
			{
				CGRect cgRect = drawRect.convertToCGRect( inContext->getPortHeight() );
				const float halfLineWidth = dragHiliteThickness / 2.0f;
				cgRect = CGRectInset(cgRect, halfLineWidth, halfLineWidth);	// CoreGraphics lines are positioned between pixels rather than on them
				CGContextStrokeRect(inContext->getPlatformGraphicsContext(), cgRect);
			}
		}
		else
		{
			RGBColor dragHiliteColor;
			OSErr error = GetDragHiliteColor(getDfxGuiEditor()->GetCarbonWindow(), &dragHiliteColor);
			if (error == noErr)
			{
				const float rgbScalar = 1.0f / (float)0xFFFF;
				DGColor strokeColor((float)(dragHiliteColor.red) * rgbScalar, (float)(dragHiliteColor.green) * rgbScalar, (float)(dragHiliteColor.blue) * rgbScalar);
				inContext->setStrokeColor(strokeColor);
				inContext->strokeRect(&drawRect, dragHiliteThickness);
			}
		}
	}
}

//-----------------------------------------------------------------------------
void DGBackgroundControl::setDragActive(bool inActiveStatus)
{
	bool oldStatus = dragIsActive;
	dragIsActive = inActiveStatus;
	if (oldStatus != inActiveStatus)
		redraw();
}
#endif






#pragma mark -
#pragma mark DGCarbonViewControl

#ifdef TARGET_API_AUDIOUNIT

//-----------------------------------------------------------------------------
void DGCarbonViewControl::ControlToParameter()
{
	if (mType == kTypeContinuous)
	{
		double controlValue = GetValueFract();
		Float32 paramValue;
		DfxParameterValueConversionRequest request;
		UInt32 dataSize = sizeof(request);
		request.inConversionType = kDfxParameterValueConversion_expand;
		request.inValue = controlValue;
		if (AudioUnitGetProperty(GetOwnerView()->GetEditAudioUnit(), kDfxPluginProperty_ParameterValueConversion, 
								kAudioUnitScope_Global, mParam.mParameterID, &request, &dataSize) 
								== noErr)
			paramValue = request.outValue;
		else
			paramValue = AUParameterValueFromLinear(controlValue, &mParam);
		mParam.SetValue(mListener, this, paramValue);
	}
	else
		AUCarbonViewControl::ControlToParameter();
}

//-----------------------------------------------------------------------------
void DGCarbonViewControl::ParameterToControl(Float32 inParamValue)
{
	if (mType == kTypeContinuous)
	{
		DfxParameterValueConversionRequest request;
		UInt32 dataSize = sizeof(request);
		request.inConversionType = kDfxParameterValueConversion_contract;
		request.inValue = inParamValue;
		if (AudioUnitGetProperty(GetOwnerView()->GetEditAudioUnit(), kDfxPluginProperty_ParameterValueConversion, 
								kAudioUnitScope_Global, mParam.mParameterID, &request, &dataSize) 
								== noErr)
			SetValueFract(request.outValue);
		else
			SetValueFract(AUParameterValueToLinear(inParamValue, &mParam));
	}
	else
		AUCarbonViewControl::ParameterToControl(inParamValue);
}

#endif	// TARGET_API_AUDIOUNIT



#endif	// !TARGET_PLUGIN_USES_VSTGUI
