#include "slew.h"

#ifdef QT_MAC_USE_COCOA
#include <Cocoa/Cocoa.h>
#else
#include <Carbon/Carbon.h>
#endif

#include <QMenu>


void
helper_set_resizeable(QWidget *widget, bool enabled)
{
#ifdef QT_MAC_USE_COCOA
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *window = [(NSView *)widget->winId() window];
	
	if ([window respondsToSelector: @selector(setStyleMask:)]) {
		NSUInteger style = [window styleMask];
		if (enabled)
			style |= NSResizableWindowMask;
		else
			style &= ~NSResizableWindowMask;
		
		[window setStyleMask: style];
	}
	
	[pool release];
#else
	WindowRef window = HIViewGetWindow((HIViewRef)widget->winId());
	
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
	int attribs[] = { kWindowResizableAttribute, 0 };
	if (enabled)
		HIWindowChangeAttributes(window, attribs, NULL);
	else
		HIWindowChangeAttributes(window, NULL, attribs);
#else
	if (enabled)
		ChangeWindowAttributes(window, kWindowResizableAttribute, 0);
	else
		ChangeWindowAttributes(window, 0, kWindowResizableAttribute);
#endif
	
#endif
}


void
helper_init_notification(QWidget *widget)
{
#ifdef QT_MAC_USE_COCOA
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *window = [(NSView *)widget->winId() window];
	
	[window setCanHide: FALSE];
	[window setHasShadow: FALSE];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
	[window setCollectionBehavior: NSWindowCollectionBehaviorIgnoresCycle];
#endif
	
	[pool release];
#else
	WindowRef window = HIViewGetWindow((HIViewRef)widget->winId());
	
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
	int attribs[] = { kHIWindowBitDoesNotHide, kHIWindowBitDoesNotCycle, kHIWindowBitNoShadow, 0 };
	HIWindowChangeAttributes(window, attribs, NULL);
#else
	ChangeWindowAttributes(window, kHIWindowBitDoesNotHide | kHIWindowBitDoesNotCycle | kHIWindowBitNoShadow, 0);
#endif

#endif
}


void
helper_clear_menu_previous_action(QMenu *menu)
{
#ifdef QT_MAC_USE_COCOA
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSMenu *nsmenu = static_cast<NSMenu *>(menu->macMenu());
	
	[[nsmenu delegate] willHighlightItem: nil];
	[pool release];
#endif
}

