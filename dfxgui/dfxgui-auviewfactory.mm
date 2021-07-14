/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2011-2021  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#import "dfxgui-auviewfactory.h"

#import <AudioUnit/AUCocoaUIView.h>
#import <Cocoa/Cocoa.h>
#import <memory>

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
	std::unique_ptr<DfxGuiEditor> mDfxGuiEditor;
	NSTimer* mIdleTimer;
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
	return [NSString stringWithCString:PLUGIN_NAME_STRING " editor" encoding:NSUTF8StringEncoding];
}

//-----------------------------------------------------------------------------
// this class is simply a view-factory, returning a new autoreleased view each time it's called
- (NSView*) uiViewForAudioUnit:(AudioUnit)inAU withSize:(NSSize)inPreferredSize
{
	NSAssert(inAU != nil, @"null Audio Unit instance");
#if __has_feature(objc_arc)
	// TODO: is this implementation actually okay either way?
	NSView* __autoreleasing view = [[DGNSViewForAU alloc] initWithAU:inAU preferredSize:inPreferredSize];
	return view;
#else
	return [[[DGNSViewForAU alloc] initWithAU:inAU preferredSize:inPreferredSize] autorelease];
#endif
}

@end  // DGCocoaAUViewFactory



//-----------------------------------------------------------------------------
CFStringRef dfx::DGCocoaAUViewFactory_CopyClassName()
{
	auto const name = NSStringFromClass([DGCocoaAUViewFactory class]);
	if (name != nil)
	{
		return reinterpret_cast<CFStringRef>(CFBridgingRetain(name));
	}
	return nil;
}






//-----------------------------------------------------------------------------
@implementation DGNSViewForAU
//-----------------------------------------------------------------------------

[[nodiscard]] extern std::unique_ptr<DfxGuiEditor> DFXGUI_NewEditorInstance(DGEditorListenerInstance inEffectInstance);

//-----------------------------------------------------------------------------
- (id) initWithAU:(AudioUnit)inAU preferredSize:(NSSize)inSize
{
	mIdleTimer = nil;

	if (inAU == nil)
	{
		return nil;
	}

	mDfxGuiEditor = DFXGUI_NewEditorInstance(inAU);
	if (!mDfxGuiEditor)
	{
		return nil;
	}

	self = [super initWithFrame:NSMakeRect(0.0, 0.0, inSize.width, inSize.height)];
	if (self == nil)
	{
		return nil;
	}

	auto const success = mDfxGuiEditor->open((__bridge void*)self);
	if (!success)
	{
#if !__has_feature(objc_arc)
		[self release];
#endif
		return nil;
	}

	// set the size of the view to match the editor's size
	if (mDfxGuiEditor->getFrame() != nil)
	{
		auto const& frameSize = mDfxGuiEditor->getFrame()->getViewSize();
		auto const newSize = NSMakeRect(0.0, 0.0, frameSize.getWidth(), frameSize.getHeight());
		[self setFrame:newSize];
	}

	mIdleTimer = [NSTimer scheduledTimerWithTimeInterval:DfxGuiEditor::kIdleTimerInterval 
												  target:self selector:@selector(idle:) userInfo:nil repeats:YES];

	return self;
}

//-----------------------------------------------------------------------------
// XXX implement this even if we don't actually support view resizing?
- (void) setFrame:(NSRect)inNewSize
{
	[super setFrame:inNewSize];

	if (mDfxGuiEditor)
	{
		if (mDfxGuiEditor->getFrame() != nil)
		{
			mDfxGuiEditor->getFrame()->setSize(inNewSize.size.width, inNewSize.size.height);
		}
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
	}
	mIdleTimer = nil;

	[super removeFromSuperview];
}

/*
TODO: eh do I need to implement this?
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
	if (mDfxGuiEditor)
	{
		mDfxGuiEditor->close();
		mDfxGuiEditor.reset();
	}

#if !__has_feature(objc_arc)
	[super dealloc];
#endif
}

//-----------------------------------------------------------------------------
- (void) idle:(NSTimer*) __unused theTimer
{
	if (mDfxGuiEditor)
	{
		mDfxGuiEditor->idle();
	}
}

@end  // DGNSViewForAU
