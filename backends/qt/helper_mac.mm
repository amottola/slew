#include <Carbon/Carbon.h>
struct CPSProcessSerNum
{
	UInt32 lo;
	UInt32 hi;
};
extern "C" {
	OSErr CPSGetCurrentProcess(struct CPSProcessSerNum *psn);
	OSErr CPSEnableForegroundOperation(struct CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
	OSErr CPSSetFrontProcess(struct CPSProcessSerNum *psn);
}


#include "slew.h"


extern void qt_mac_set_dock_menu(QMenu *);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#define HAS_COCOA
#include <QtGuiVersion>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#else
#ifdef QT_MAC_USE_COCOA
#define HAS_COCOA
#endif
#endif

#ifdef HAS_COCOA
#include <QTimer>

class DelayedResizer : public QObject
{
	Q_OBJECT
	
public:
	DelayedResizer(QWidget *widget, bool enabled) : QObject(widget), fWidget(widget), fEnabled(enabled) {}

public slots:
	void resize() { helper_set_resizeable(fWidget, fEnabled); deleteLater(); }

private:
	QWidget		*fWidget;
	bool		fEnabled;
};


#undef slots
#include <Cocoa/Cocoa.h>



#if !defined(MAC_OS_X_VERSION_10_8) || (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_8)
@protocol NSUserNotificationCenterDelegate
@end
#endif

@class NSUserNotificationCenter;

// Since NSUserNotification and NSUserNotificationCenter are new classes in
// 10.8, they cannot simply be declared with an @interface. An @implementation
// is needed to link, but providing one would cause a runtime conflict when
// running on 10.8. Instead, provide the interface defined as a protocol and
// use that instead, because sizeof(id<Protocol>) == sizeof(Class*). In order to
// instantiate, use NSClassFromString and simply assign the alloc/init'd result
// to an instance of the proper protocol. This way the compiler, linker, and
// loader are all happy. And the code isn't full of objc_msgSend.
@protocol SlewUserNotification <NSObject>
@property(copy) NSString* title;
@property(copy) NSString* subtitle;
@property(copy) NSString* informativeText;
@property(copy) NSString* actionButtonTitle;
@property(copy) NSDictionary* userInfo;
@property(copy) NSDate* deliveryDate;
@property(copy) NSTimeZone* deliveryTimeZone;
@property(copy) NSDateComponents* deliveryRepeatInterval;
@property(readonly) NSDate* actualDeliveryDate;
@property(readonly, getter=isPresented) BOOL presented;
@property(readonly, getter=isRemote) BOOL remote;
@property(copy) NSString* soundName;
@property BOOL hasActionButton;
@end

@protocol SlewUserNotificationCenter
+ (NSUserNotificationCenter*)defaultUserNotificationCenter;
@property(assign) id<NSUserNotificationCenterDelegate> delegate;
@property(copy) NSArray* scheduledNotifications;
- (void)scheduleNotification:(id<SlewUserNotification>)notification;
- (void)removeScheduledNotification:(id<SlewUserNotification>)notification;
@property(readonly) NSArray* deliveredNotifications;
- (void)deliverNotification:(id<SlewUserNotification>)notification;
- (void)removeDeliveredNotification:(id<SlewUserNotification>)notification;
- (void)removeAllDeliveredNotifications;
@end

@interface NotificationCenterDelegate : NSObject
    <NSUserNotificationCenterDelegate> {
}
@end

@implementation NotificationCenterDelegate
- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(id<SlewUserNotification>)nsNotification {
  // Always display notifications, regardless of whether the app is foreground.
  return YES;
}

@end


#else
#include <Carbon/Carbon.h>
#endif

#include <QMenu>


#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <qpa/qplatformnativeinterface.h>
static NSWindow *
qt_mac_window_for(const QWidget *w)
{
	QWindow *window = w->window()->windowHandle();
	if (!window)
		return NULL;
	return static_cast<NSWindow *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("nswindow", window));
}
#else
extern WindowRef qt_mac_window_for(const QWidget *w);
#endif


void
helper_bring_application_to_front()
{
	struct CPSProcessSerNum psn;
	if ((!CPSGetCurrentProcess(&psn)) && (!CPSEnableForegroundOperation(&psn, 0x03, 0x3C, 0x2C, 0x1103))) {
		if (!CPSSetFrontProcess(&psn))
			[NSApplication sharedApplication];
	}
}


bool
helper_notify_center(const QString &title, const QString &text)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	Class _NSUserNotificationCenter = NSClassFromString(@"NSUserNotificationCenter");
	if (_NSUserNotificationCenter != nil) {
		id<SlewUserNotificationCenter> center = [_NSUserNotificationCenter performSelector:@selector(defaultUserNotificationCenter)];
		if (center) {
			static NotificationCenterDelegate *delegate = NULL;
			
			if (!delegate) {
				delegate = [[NotificationCenterDelegate alloc] init];
				[center setDelegate: delegate];
			}
			
			id<SlewUserNotification> ns_notification = [[NSClassFromString(@"NSUserNotification") alloc] init];
			NSString *ns_title = [[NSString alloc] initWithUTF8String:(char *)title.toUtf8().constData()];
			NSString *ns_text = [[NSString alloc] initWithUTF8String:(char *)text.toUtf8().constData()];
		
			ns_notification.title = ns_title;
			ns_notification.informativeText = ns_text;
			ns_notification.hasActionButton = NO;
		
			[center deliverNotification:ns_notification];
			
			[ns_title release];
			[ns_text release];
			[ns_notification release];
			
			return true;
		}
	}
	[pool release];
	return false;
}


void
helper_set_resizeable(QWidget *widget, bool enabled)
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
	widget->winId();
#endif
#ifdef HAS_COCOA
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *window = (NSWindow *)qt_mac_window_for(widget);
	
	if (!window) {
		QTimer::singleShot(100, new DelayedResizer(widget, enabled), SLOT(resize()));
		return;
	}
	
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
	WindowRef window = (WindowRef)qt_mac_window_for(widget);
	
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
#ifdef HAS_COCOA
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *window = (NSWindow *)qt_mac_window_for(widget);
	
	if (!window)
		return;
	
	[window setCanHide: FALSE];
	[window setHasShadow: FALSE];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
	[window setCollectionBehavior: NSWindowCollectionBehaviorIgnoresCycle];
#endif
	
	[pool release];
#else
	WindowRef window = (WindowRef)qt_mac_window_for(widget);
	
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
#ifdef HAS_COCOA
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	NSMenu *nsmenu = menu->toNSMenu();
#else
	NSMenu *nsmenu = static_cast<NSMenu *>(menu->macMenu());
#endif
	
	[[nsmenu delegate] menu: nsmenu willHighlightItem: nil];
	[pool release];
#endif
}


void
helper_set_dock_menu(QMenu *menu)
{
	qt_mac_set_dock_menu(menu);
}


#include "helper_mac.moc"
