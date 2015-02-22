/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2011-2015  Sophia Poirier

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

#import <Cocoa/Cocoa.h>
#import <AudioUnit/AUCocoaUIView.h>

#import "dfxgui-auviewfactory.h"
#import "dfxguieditor.h"



// XXX need to uniquify the Cocoa class names for each executable
#define ConcatMacros1(a, b, c, d, e)	a ## _ ## b ## _ ## c ## _ ## d ## _ ## e
#define ConcatMacros2(a, b, c, d, e)	ConcatMacros1(a, b, c, d, e)
#define UniqueObjCClassName(baseName)	ConcatMacros2(baseName, PLUGIN_CLASS_NAME, PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_BUGFIX)

#define DGCocoaAUViewFactory	UniqueObjCClassName(DGCocoaAUViewFactory)
#define DGNSViewForAU			UniqueObjCClassName(DGNSViewForAU)



//-----------------------------------------------------------------------------
@interface DGCocoaAUViewFactory : NSObject <AUCocoaUIBase>
{
}
@end



//-----------------------------------------------------------------------------
@interface DGNSViewForAU : NSView
{
	DfxGuiEditor * mDfxGuiEditor;
	NSTimer * mIdleTimer;
}
- (id) initWithAU:(AudioUnit)inAU preferredSize:(NSSize)inSize;
@end






//-----------------------------------------------------------------------------
@implementation DGCocoaAUViewFactory
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
- (unsigned) interfaceVersion
{
	return 0;
}

//-----------------------------------------------------------------------------
- (NSString*) description
{
	return [[[NSString alloc] initWithString:@""PLUGIN_NAME_STRING" editor"] autorelease];
}

//-----------------------------------------------------------------------------
// this class is simply a view-factory, returning a new autoreleased view each time it's called
- (NSView*) uiViewForAudioUnit:(AudioUnit)inAU withSize:(NSSize)inPreferredSize
{
	return [[[DGNSViewForAU alloc] initWithAU:inAU preferredSize:inPreferredSize] autorelease];
}

@end
// DGCocoaAUViewFactory



//-----------------------------------------------------------------------------
CFStringRef DGCocoaAUViewFactory_CopyClassName()
{
	NSString* name = NSStringFromClass([DGCocoaAUViewFactory class]);
	if (name != nil)
		[name retain];
	return reinterpret_cast<CFStringRef>(name);
}






//-----------------------------------------------------------------------------
@implementation DGNSViewForAU
//-----------------------------------------------------------------------------

extern DfxGuiEditor * DFXGUI_NewEditorInstance(AudioUnit inEffectInstance);

//-----------------------------------------------------------------------------
- (id) initWithAU:(AudioUnit)inAU preferredSize:(NSSize)inSize
{
	mDfxGuiEditor = nil;
	mIdleTimer = nil;

	if (inAU == nil)
		return nil;

#if !__LP64__ && MAC_COCOA && (VSTGUI_VERSION_MAJOR == 3)
	VSTGUI::CFrame::setCocoaMode(true);
#endif

	mDfxGuiEditor = DFXGUI_NewEditorInstance(inAU);
	if (mDfxGuiEditor == nil)
		return nil;

	self = [super initWithFrame:NSMakeRect(0.0, 0.0, inSize.width, inSize.height)];
	if (self == nil)
		return nil;

	bool success = mDfxGuiEditor->open(self);
	if (!success)
	{
		[self dealloc];
		return nil;
	}

	// set the size of the view to match the editor's size
	if (mDfxGuiEditor->getFrame() != nil)
	{
		CRect frameSize = mDfxGuiEditor->getFrame()->getViewSize(frameSize);
		NSRect newSize = NSMakeRect(0.0, 0.0, frameSize.width(), frameSize.height());
		[self setFrame:newSize];
	}

	mIdleTimer = [NSTimer scheduledTimerWithTimeInterval:kDfxGui_IdleTimerInterval 
							target:self selector:@selector(idle:) userInfo:nil repeats:YES];
	if (mIdleTimer != nil)
		[mIdleTimer retain];

	return self;
}

//-----------------------------------------------------------------------------
// XXX implement this even if we don't actually support view resizing?
- (void) setFrame:(NSRect)inNewSize
{
	[super setFrame:inNewSize];

	if (mDfxGuiEditor != nil)
	{
		if (mDfxGuiEditor->getFrame() != nil)
			mDfxGuiEditor->getFrame()->setSize(inNewSize.size.width, inNewSize.size.height);
	}
}

//-----------------------------------------------------------------------------
- (BOOL) isFlipped
{
	return YES;
}

//-----------------------------------------------------------------------------
- (void) removeFromSuperview
{
	if (mIdleTimer != nil)
	{
		[mIdleTimer invalidate];
		[mIdleTimer release];
	}
	mIdleTimer = nil;

	[super removeFromSuperview];
}

/*
XXX eh do I need to implement this?  or is it only an option in VSTGUI 4?
//-----------------------------------------------------------------------------
- (void) viewDidMoveToSuperview
{
	if (plugView != nil)
	{
		if ([self superview] != nil)
		{
			if (!isAttached)
			{
				isAttached = plugView->attached(self, kPlatformTypeNSView) == kResultTrue;
			}
		}
		else
		{
			if (isAttached)
			{
				plugView->removed();
				isAttached = NO;
			}
		}
	}
}
*/

//-----------------------------------------------------------------------------
- (void) dealloc
{
	if (mDfxGuiEditor != nil)
	{
		DfxGuiEditor * editor_temp = mDfxGuiEditor;
		mDfxGuiEditor = nil;
		editor_temp->close();
		delete editor_temp;
	}

	[super dealloc];
}

//-----------------------------------------------------------------------------
- (void) idle:(NSTimer*) theTimer
{
	if (mDfxGuiEditor != nil)
		mDfxGuiEditor->idle();
}

@end
// DGNSViewForAU
