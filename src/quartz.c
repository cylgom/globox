#include "quartz.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globox.h"
#include "quartz_helpers.h"

// callbacks
void quartz_view_draw_rect_callback(
	id app,
	SEL cm,
	struct quartz_rect rect)
{
	id buf = quartz_msg_id(
		(id) objc_getClass("NSColor"),
		sel_getUid("redColor"));

	struct quartz_rect rect_new =
	{
		.origin.x = 20,
		.origin.y = 20,
		.size.width = 60,
		.size.height = 60,
	};

	quartz_msg_void(
		buf,
		sel_getUid("set"));

	NSRectFill(rect_new);

	// get that sweet memory address
	id ns_ctx = quartz_msg_id(
		(id) objc_getClass("NSGraphicsContext"),
		sel_getUid("currentContext"));

	printf("%p\n", (void*) ns_ctx);

	id cg_ctx = quartz_msg_id(
		ns_ctx,
		sel_getUid("CGContext"));

	printf("%p\n", (void*) cg_ctx);

	// will fail if the context is not a bitmap
	uint8_t* argb = CGBitmapContextGetData(cg_ctx);

	printf("%p\n", (void*) argb);

	for (int i = 0; i < 2000; ++i)
	{
		argb[i] = 0xFF;
	}
}

// window state event sender
void quartz_window_event(enum globox_quartz_window_event quartz_window_event)
{
	// create custom internal event
	struct quartz_point point =
	{
		.x = 0.0,
		.y = 0.0,
	};

	id* event = quartz_msg_event(
		(id) objc_getClass("NSEvent"),
		sel_getUid("otherEventWithType:location:modifierFlags:timestamp:windowNumber:context:subtype:data1:data2:"),
		NSEventTypeApplicationDefined,
		point,
		0,
		0,
		0,
		NULL,
		quartz_window_event,
		0,
		0);

	// get app singleton
	id app = quartz_msg_id(
		(id) objc_getClass("NSApplication"),
		sel_getUid("sharedApplication"));

	// send custom event to main thread
	quartz_msg_post(
		app,
		sel_getUid("postEvent:atStart:"),
		event,
		true);
}

// window state event callbacks relaying the info to quartz_window_event
struct quartz_rect quartz_window_event_maximize_toggle(
	id window_delegate,
	SEL cmd,
	id* window,
	struct quartz_rect rect)
{
	quartz_window_event(GLOBOX_QUARTZ_WINDOW_EVENT_MAXIMIZE_TOGGLE);

	return rect;
}

void quartz_window_event_minimize_on(
	id window_delegate,
	SEL cmd,
	id* notif)
{
	quartz_window_event(GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_ON);
}

void quartz_window_event_minimize_off(
	id window_delegate,
	SEL cmd,
	id* notif)
{
	quartz_window_event(GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_OFF);
}

void quartz_window_event_fullscreen_on(
	id window_delegate,
	SEL cmd,
	id* notif)
{
	quartz_window_event(GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_ON);
}

void quartz_window_event_fullscreen_off(
	id window_delegate,
	SEL cmd,
	id* notif)
{
	quartz_window_event(GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_OFF);
}

// app delegate window creation callback
bool quartz_app_delegate_init_callback(
	struct quartz_app_delegate* app_delegate,
	SEL cmd,
	id msg)
{
	// retrieve globox context
	void* out;

	object_getInstanceVariable(
		(id) app_delegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;

	// window
	int states =
		NSWindowStyleMaskTitled
		| NSWindowStyleMaskClosable
		| NSWindowStyleMaskMiniaturizable
		| NSWindowStyleMaskResizable;

	struct quartz_rect rect_win =
	{
		.origin.x = globox->init_x,
		.origin.y = globox->init_y,
		.size.width = globox->width,
		.size.height = globox->height,
	};

	app_delegate->window = quartz_msg_id(
		(id) objc_getClass("NSWindow"),
		sel_getUid("alloc"));

	app_delegate->window = quartz_msg_win(
		app_delegate->window, 
		sel_getUid("initWithContentRect:styleMask:backing:defer:"),
		rect_win,
		states,
		NSBackingStoreBuffered,
		false);

	// window delegate
	Class window_delegate_class = objc_allocateClassPair(
		(Class) objc_getClass("NSObject"),
		"WindowDelegate",
		0);

	// inject methods
	class_addMethod(
		window_delegate_class,
		sel_getUid("windowWillUseStandardFrame:defaultFrame:"),
		(IMP) quartz_window_event_maximize_toggle,
		"v@:^@:@");

	class_addMethod(
		window_delegate_class,
		sel_getUid("windowWillMiniaturize:"),
		(IMP) quartz_window_event_minimize_on,
		"v@:^@");

	class_addMethod(
		window_delegate_class,
		sel_getUid("windowDidDeminiaturize:"),
		(IMP) quartz_window_event_minimize_off,
		"v@:^@");

	class_addMethod(
		window_delegate_class,
		sel_getUid("windowWillEnterFullScreen:"),
		(IMP) quartz_window_event_fullscreen_on,
		"v@:^@");

	class_addMethod(
		window_delegate_class,
		sel_getUid("windowWillExitFullScreen:"),
		(IMP) quartz_window_event_fullscreen_off,
		"v@:^@");

	id window_delegate = quartz_msg_id(
		(id) window_delegate_class,
		sel_getUid("alloc"));

	// register in the application delegate
	quartz_msg_ptr(
		app_delegate->window,
		sel_getUid("setDelegate:"),
		window_delegate);

	// view
	struct quartz_rect rect_view =
	{
		.origin.x = 0,
		.origin.y = 0,
		.size.width = globox->width,
		.size.height = globox->height,
	};

	globox->quartz_view_obj = quartz_msg_id(
		(id) globox->quartz_view_class,
		sel_getUid("alloc"));

	globox->quartz_view_obj = quartz_msg_rect(
		globox->quartz_view_obj,
		sel_getUid("initWithFrame:"),
		rect_view);

	// messages
	quartz_msg_ptr(
		app_delegate->window, 
		sel_getUid("setContentView:"),
		globox->quartz_view_obj);

	quartz_msg_id(
		app_delegate->window, 
		sel_getUid("becomeFirstResponder"));

	quartz_msg_ptr(
		app_delegate->window, 
		sel_getUid("makeKeyAndOrderFront:"),
		app_delegate);

	// escape the pesky default event loop
	quartz_msg_ptr(
		globox->fd.app,
		sel_getUid("stop:"),
		NULL);

	// set title
	id string = quartz_msg_string(
		(id) objc_getClass("NSString"),
		sel_getUid("stringWithCString:encoding:"),
		globox->title,
		NSUTF8StringEncoding);

	quartz_msg_obj(
		app_delegate->window,
		sel_getUid("setTitle:"),
		string);

	return true;
}
