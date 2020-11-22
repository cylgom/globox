/// this file implements the main globox functions for the macos platform
/// it is completely isolated from the graphic context functions

#define _XOPEN_SOURCE 700

// include globox structures and functions
#include "globox.h"
#include "globox_error.h"
// system includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// macOS includes
#include "macos/globox_macos_helpers.h"

// include platform structures
#include "macos/globox_macos.h"

#define RESIZE_REACH_CORNER 12
#define RESIZE_REACH_SIDE 7
#define RESIZE_REACH_TITLEBAR 22

enum system_cursor_ids
{
	CURSOR_ARROW = 0,
	CURSOR_WE = 28,
	CURSOR_NESW = 30,
	CURSOR_NS = 32,
	CURSOR_NWSE = 34,
};

enum system_cursor_hover
{
	HOVER_NONE = 0,
	HOVER_N,
	HOVER_NW,
	HOVER_W,
	HOVER_SW,
	HOVER_S,
	HOVER_SE,
	HOVER_E,
	HOVER_NE,
	HOVER_TITLEBAR,
};

// window state event sender
void event_window_state(short event_subtype, long event_value)
{
	// create custom
	struct macos_point point =
	{
		.x = 0.0,
		.y = 0.0,
	};

	id* event =
		macos_msg_event(
			(id) objc_getClass("NSEvent"),
			sel_getUid(
				"otherEventWithType:"
				"location:"
				"modifierFlags:"
				"timestamp:"
				"windowNumber:"
				"context:"
				"subtype:"
				"data1:"
				"data2:"),
			NSEventTypeApplicationDefined,
			point,
			0,
			0,
			0,
			NULL,
			event_subtype,
			event_value,
			0);

	// get the nsapplication singleton
	id nsapplication =
		macos_msg_id_none(
			(id) objc_getClass("NSApplication"),
			sel_getUid("sharedApplication"));

	// send the custom event to the main thread
	macos_msg_post(
		nsapplication,
		sel_getUid("postEvent:atStart:"),
		event,
		false);
}

unsigned long presentation(
	id windowdelegate,
	SEL cmd,
	id* window,
	unsigned long options)
{
	return NSApplicationPresentationHideDock
		| NSApplicationPresentationHideMenuBar
		| NSApplicationPresentationDisableAppleMenu
		| NSApplicationPresentationFullScreen;
}

// window state event callbacks
void event_window_minimize_on(
	id windowdelegate,
	SEL cmd,
	id* notif)
{
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_STATE,
		GLOBOX_MACOS_EVENT_WINDOW_MINIMIZE_ON);
}

void event_window_minimize_off(
	id windowdelegate,
	SEL cmd,
	id* notif)
{
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_STATE,
		GLOBOX_MACOS_EVENT_WINDOW_MINIMIZE_OFF);
}

void event_window_fullscreen_on(
	id windowdelegate,
	SEL cmd,
	id* notif)
{
#if 1
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_STATE,
		GLOBOX_MACOS_EVENT_WINDOW_FULLSCREEN_ON);

	id nsapplication =
		macos_msg_id_none(
			(id) objc_getClass("NSApplication"),
			sel_getUid("sharedApplication"));

	id appdelegate =
		macos_msg_id_none(
			nsapplication,
			sel_getUid("delegate"));

	void* out;
	
	object_getInstanceVariable(
		appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	id screen =
		macos_msg_id_none(
			platform->globox_macos_obj_window,
			sel_getUid("deepestScreen"));

	platform->globox_macos_old_window_frame =
		macos_msg_rect_none(
			platform->globox_macos_obj_window,
			sel_getUid("frame"));

	platform->globox_macos_fullscreen = true;

	struct macos_rect frame =
		macos_msg_rect_none(
			screen,
			sel_getUid("frame"));

	macos_msg_id_size(
		platform->globox_macos_obj_view,
		sel_getUid("setFrameSize:"),
		frame.size);

	macos_msg_id_size(
		platform->globox_macos_obj_blur,
		sel_getUid("setFrameSize:"),
		frame.size);

	globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;
#endif
}

void event_window_fullscreen_off(
	id windowdelegate,
	SEL cmd,
	id* notif)
{
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_STATE,
		GLOBOX_MACOS_EVENT_WINDOW_FULLSCREEN_OFF);

	id nsapplication =
		macos_msg_id_none(
			(id) objc_getClass("NSApplication"),
			sel_getUid("sharedApplication"));

	id appdelegate =
		macos_msg_id_none(
			nsapplication,
			sel_getUid("delegate"));

	void* out;
	
	object_getInstanceVariable(
		appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	macos_msg_resize(
		platform->globox_macos_obj_window,
		sel_getUid("setFrame:display:"),
		platform->globox_macos_old_window_frame,
		true);

	platform->globox_macos_old_window_frame.size.height -=
		RESIZE_REACH_TITLEBAR;

	macos_msg_id_size(
		platform->globox_macos_obj_view,
		sel_getUid("setFrameSize:"),
		platform->globox_macos_old_window_frame.size);

	macos_msg_id_size(
		platform->globox_macos_obj_blur,
		sel_getUid("setFrameSize:"),
		platform->globox_macos_old_window_frame.size);

	globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;
	platform->globox_macos_fullscreen = false;
}

struct macos_rect event_window_maximize_on(
	id windowdelegate,
	SEL cmd,
	id* window,
	struct macos_rect rect)
{
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_STATE,
		GLOBOX_MACOS_EVENT_WINDOW_MAXIMIZE_ON);

	return rect;
}

void event_window_move(
	id windowdelegate,
	SEL cmd,
	id* notif)
{
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_STATE,
		GLOBOX_MACOS_EVENT_WINDOW_MAXIMIZE_OFF);
}

void event_window_close(
	id windowdelegate,
	SEL cmd,
	id* notif)
{
	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_CLOSE,
		0);
}

struct macos_size event_window_resize(
	id windowdelegate,
	SEL cmd,
	id* window,
	struct macos_size size)
{
#if 1
	id nsapplication =
		macos_msg_id_none(
			(id) objc_getClass("NSApplication"),
			sel_getUid("sharedApplication"));

	id appdelegate =
		macos_msg_id_none(
			nsapplication,
			sel_getUid("delegate"));

	void* out;
	
	object_getInstanceVariable(
		appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	macos_msg_id_size(
		platform->globox_macos_obj_view,
		sel_getUid("setFrameSize:"),
		size);

	macos_msg_id_size(
		platform->globox_macos_obj_blur,
		sel_getUid("setFrameSize:"),
		size);
#endif

	// TODO re-alloc CGContext

	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_RESIZE,
		0);

	return size;
}

bool appdelegate_init_common(
	struct macos_appdelegate* appdelegate,
	SEL cmd,
	id msg)
{
	// get the globox context
	void* out;

	object_getInstanceVariable(
		(id) appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	// configure the window
	int states =
		NSWindowStyleMaskClosable
		| NSWindowStyleMaskMiniaturizable;

	if (globox->globox_frameless == false)
	{
		states |= NSWindowStyleMaskTitled;
	}

	struct macos_rect rect_win =
	{
		.origin.x = globox->globox_x,
		.origin.y = globox->globox_y,
		.size.width = globox->globox_width,
		.size.height = globox->globox_height,
	};

	appdelegate->window =
		macos_msg_id_none(
			(id) objc_getClass("NSWindow"),
			sel_getUid("alloc"));

	appdelegate->window =
		macos_msg_win(
			appdelegate->window,
			sel_getUid("initWithContentRect:styleMask:backing:defer:"),
			rect_win,
			states,
			NSBackingStoreBuffered,
			false);

	// declare a window delegate class
	Class windowdelegateclass =
		objc_allocateClassPair(
			(Class) objc_getClass("NSObject"),
			"WindowDelegate",
			0);

	// inject some methods in the window delegate class
	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowWillUseStandardFrame:defaultFrame:"),
		(IMP) event_window_maximize_on,
		"v@:^@:@");

	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowWillMove:"),
		(IMP) event_window_move,
		"v@:^@");

	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowWillMiniaturize:"),
		(IMP) event_window_minimize_on,
		"v@:^@");

	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowDidDeminiaturize:"),
		(IMP) event_window_minimize_off,
		"v@:^@");

	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowWillEnterFullScreen:"),
		(IMP) event_window_fullscreen_on,
		"v@:^@");

	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowWillExitFullScreen:"),
		(IMP) event_window_fullscreen_off,
		"v@:^@");

	class_addMethod(
		windowdelegateclass,
		sel_getUid("windowWillClose:"),
		(IMP) event_window_close,
		"v@:^@");

#if 1
	class_addMethod(
		windowdelegateclass,
		sel_getUid("window:willUseFullScreenContentSize:"),
		(IMP) event_window_resize,
		"@:^@:@");
#endif

	class_addMethod(
		windowdelegateclass,
		sel_getUid("window:willUseFullScreenPresentationOptions:"),
		(IMP) presentation,
		"@:^@:@");

	// instantiate the window delegate object
	id windowdelegate =
		macos_msg_id_none(
			(id) windowdelegateclass,
			sel_getUid("alloc"));

	// link the window delegate to the application delegate
	macos_msg_void_id(
		appdelegate->window,
		sel_getUid("setDelegate:"),
		windowdelegate);

	// instantiate the view objects
	platform->globox_macos_obj_masterview =
		macos_msg_id_none(
			(id) objc_getClass("NSView"),
			sel_getUid("alloc"));

	platform->globox_macos_obj_blur =
		macos_msg_id_none(
			(id) objc_getClass("NSVisualEffectView"),
			sel_getUid("alloc"));

	platform->globox_macos_obj_view =
		macos_msg_id_none(
			(id) platform->globox_macos_class_view,
			sel_getUid("alloc"));

	// set the view object's globox context pointer
	object_setInstanceVariable(
		platform->globox_macos_obj_view,
		"globox",
		globox);

	// time for the backend callback
	bool err_callback =
		platform->globox_macos_appdelegate_callback(
			appdelegate,
			cmd,
			msg);

	if (err_callback == false)
	{
		return false;
	}

	// initialize window
	macos_msg_void_voidptr(
		appdelegate->window,
		sel_getUid("setContentView:"),
		platform->globox_macos_obj_masterview);

	macos_msg_void_none(
		appdelegate->window,
		sel_getUid("becomeFirstResponder"));

	macos_msg_void_voidptr(
		appdelegate->window,
		sel_getUid("makeKeyAndOrderFront:"),
		appdelegate);

	// escape the default event loop
	macos_msg_void_voidptr(
		platform->globox_platform_event_handle,
		sel_getUid("stop:"),
		NULL);

	// save a reference to the window instance
	platform->globox_macos_obj_window = appdelegate->window;

	macos_msg_void_bool(
		platform->globox_macos_obj_window,
		sel_getUid("setMovable:"),
		false);

	macos_msg_void_ulong(
		platform->globox_macos_obj_window,
		sel_getUid("setCollectionBehavior:"),
		NSWindowCollectionBehaviorParticipatesInCycle
		| NSWindowCollectionBehaviorFullScreenPrimary
		| NSWindowCollectionBehaviorFullScreenAllowsTiling);

	struct macos_size min_size =
	{
		.width = 25,
		.height = 35,
	};

	macos_msg_void_size(
		platform->globox_macos_obj_window,
		sel_getUid("setMinSize:"),
		min_size);

	id* button =
		macos_msg_idptr_int(
			platform->globox_macos_obj_window,
			sel_getUid("standardWindowButton:"),
			2);

	id* button_min =
		macos_msg_idptr_int(
			platform->globox_macos_obj_window,
			sel_getUid("standardWindowButton:"),
			1);

	id* button_close =
		macos_msg_idptr_int(
			platform->globox_macos_obj_window,
			sel_getUid("standardWindowButton:"),
			0);

	platform->globox_macos_buttons[2] =
		macos_msg_rect_none(
			(id) button,
			sel_getUid("frame"));

	platform->globox_macos_buttons[1] =
		macos_msg_rect_none(
			(id) button_min,
			sel_getUid("frame"));

	platform->globox_macos_buttons[0] =
		macos_msg_rect_none(
			(id) button_close,
			sel_getUid("frame"));

	macos_msgptr_void_bool(
		button,
		sel_getUid("setEnabled:"),
		true);

	return true;
}

static void make_class_view(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// create the View class
	platform->globox_macos_class_view =
		objc_allocateClassPair(
			(Class) objc_getClass("NSView"),
			"View",
			0);

	// inject the globox pointer
	class_addIvar(
		platform->globox_macos_class_view,
		"globox",
		sizeof (void*),
		sizeof (void*),
		"ˆv");
}

static void make_class_appdelegate(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// create the AppDelegate class
	platform->globox_macos_class_appdelegate =
		objc_allocateClassPair(
			(Class) objc_getClass("NSObject"),
			"AppDelegate",
			0);

	// inject the globox pointer
	class_addIvar(
		platform->globox_macos_class_appdelegate,
		"globox",
		sizeof (void*),
		sizeof (void*),
		"ˆv");

	// inject the initialization method
	class_addMethod(
		platform->globox_macos_class_appdelegate,
		sel_getUid("applicationDidFinishLaunching:"),
		(IMP) appdelegate_init_common,
		"i@:@");
}

static void init_nsapplication(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	platform->globox_platform_event_handle =
		macos_msg_id_none(
			(id) objc_getClass("NSApplication"),
			sel_getUid("sharedApplication"));
}

static void init_appdelegate(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// create the AppDelegate instance
	platform->globox_macos_obj_appdelegate =
		macos_msg_id_none(
			(id) platform->globox_macos_class_appdelegate,
			sel_getUid("alloc"));

	// set the instance's globox pointer
	object_setInstanceVariable(
		platform->globox_macos_obj_appdelegate,
		"globox",
		globox);

	// run AppDelegate init
	platform->globox_macos_obj_appdelegate =
		macos_msg_id_none(
			platform->globox_macos_obj_appdelegate,
			sel_getUid("init"));
}

static void link_appdelegate(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// attach the AppDelegate to the NSApp singleton
	macos_msg_void_voidptr(
		platform->globox_platform_event_handle,
		sel_getUid("setDelegate:"),
		platform->globox_macos_obj_appdelegate);

	// finish launching
	macos_msg_void_none(
		platform->globox_platform_event_handle,
		sel_getUid("finishLaunching"));

	macos_msg_void_none(
		platform->globox_platform_event_handle,
		sel_getUid("run"));
}

static void wait_nsapplication(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// wait for the application to be running and enable the window
	bool wait;

	do
	{
		wait = macos_msg_bool_none(
			platform->globox_platform_event_handle,
			sel_getUid("isRunning"));
	}
	while (wait == true);

	// the `1` below is not clearly defined in apple's online reference document
	// we use is exceptionally because the benefits are huge and apple sucks
	macos_msg_void_int(
		platform->globox_platform_event_handle,
		sel_getUid("setActivationPolicy:"),
		1);
}

// initalize the display system
void globox_platform_init(
	struct globox* globox,
	bool transparent,
	bool frameless,
	bool blurred)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	char** log = globox->globox_log;

	globox->globox_redraw = true;
	globox->globox_transparent = transparent;
	globox->globox_frameless = frameless;
	globox->globox_blurred = blurred;
	platform->globox_macos_fullscreen = false;
	platform->globox_inhibit_resize = false;

	platform->globox_macos_semaphore_draw = dispatch_semaphore_create(1);
	platform->globox_macos_cursor_hover = HOVER_NONE;
	platform->globox_macos_obj_cursor_a = false;

	log[GLOBOX_ERROR_MACOS_CONN] = "";
	log[GLOBOX_ERROR_MACOS_EGL_FAIL] = "egl error";

	make_class_view(globox);
	make_class_appdelegate(globox);
}

static long system_corecursortype(
	id self,
	SEL sel)
{
	id nsapplication =
		macos_msg_id_none(
			(id) objc_getClass("NSApplication"),
			sel_getUid("sharedApplication"));

	id appdelegate =
		macos_msg_id_none(
			nsapplication,
			sel_getUid("delegate"));

	void* out;
	
	object_getInstanceVariable(
		appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	return platform->globox_macos_cursor;
}

// create the window
void globox_platform_create_window(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// create the window
	init_nsapplication(globox);
	init_appdelegate(globox);

	// configure the view for transparency
	macos_msg_void_bool(
		platform->globox_macos_obj_view,
		sel_getUid("setOpaque:"),
		false);

	id color = macos_msg_id_none(
		(id) objc_getClass("NSColor"),
		sel_getUid("clearColor"));

	macos_msg_void_id(
		platform->globox_macos_obj_view,
		sel_getUid("setBackgroundColor:"),
		color);

	// platform update
	globox_platform_set_title(globox, globox->globox_title);
	globox_platform_set_state(globox, globox->globox_state);

	// create system cursor subclass
	platform->globox_macos_class_cursor =
		objc_allocateClassPair(
			(Class) objc_getClass("NSCursor"),
			"Cursor",
			0);

	class_addMethod(
		platform->globox_macos_class_cursor,
		sel_getUid("_coreCursorType"),
		(IMP) system_corecursortype,
		"@:");

	platform->globox_macos_obj_cursor_a =
		macos_msg_id_none(
			(id) platform->globox_macos_class_cursor,
			sel_getUid("alloc"));

	platform->globox_macos_obj_cursor_b =
		macos_msg_id_none(
			(id) platform->globox_macos_class_cursor,
			sel_getUid("alloc"));

	// the rest
	link_appdelegate(globox);
	wait_nsapplication(globox);

	macos_msg_void_id(
		platform->globox_macos_obj_masterview,
		sel_getUid("addSubview:"),
		platform->globox_macos_obj_view);

	macos_msg_subview(
		platform->globox_macos_obj_masterview,
		sel_getUid("addSubview:positioned:relativeTo:"),
		platform->globox_macos_obj_blur,
		-1,
		platform->globox_macos_obj_view);
}

void globox_platform_hooks(struct globox* globox)
{
}

void globox_platform_commit(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	event_window_state(
		GLOBOX_MACOS_EVENT_WINDOW_RESIZE,
		0);

	macos_msg_void_none(
		platform->globox_macos_obj_view,
		sel_getUid("setNeedsDisplay:"));
}

void globox_platform_prepoll(struct globox* globox)
{
	// not needed
}

void globox_platform_events_poll(struct globox* globox)
{
	// not needed
}

// TODO remove this from the API and externalize it into a library
void globox_platform_events_wait(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	id* future = macos_msg_idptr_none(
		(id) objc_getClass("NSDate"),
		sel_getUid("distantFuture"));

	macos_msg_poll(
		platform->globox_platform_event_handle,
		sel_getUid("nextEventMatchingMask:untilDate:inMode:dequeue:"),
		NSEventMaskAny,
		future,
		NSDefaultRunLoopMode,
		false);
}

static void update_state(struct globox* globox, long event_data)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	enum globox_state state_tmp = globox->globox_state;

	switch (event_data)
	{
		case GLOBOX_MACOS_EVENT_WINDOW_MINIMIZE_ON:
		{
			globox->globox_state = GLOBOX_STATE_MINIMIZED;
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_MINIMIZE_OFF:
		{
			globox->globox_state = platform->globox_macos_state_old;
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_FULLSCREEN_ON:
		{
			globox->globox_state = GLOBOX_STATE_FULLSCREEN;
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_FULLSCREEN_OFF:
		{
			globox->globox_state = platform->globox_macos_state_old;
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_MAXIMIZE_ON:
		{
			globox->globox_state = GLOBOX_STATE_MAXIMIZED;
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_MAXIMIZE_OFF:
		default:
		{
			globox->globox_state = GLOBOX_STATE_REGULAR;
			break;
		}
	}
	
	platform->globox_macos_state_old = state_tmp;
}

static void query_pointer(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	struct macos_point pos =
		macos_msg_point_none(
			platform->globox_macos_obj_window,
			sel_getUid("mouseLocationOutsideOfEventStream"));

	platform->globox_macos_interactive_x = pos.x;
	platform->globox_macos_interactive_y = pos.y;
}

static void handle_interactive_mode(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	if (globox->globox_interactive_mode == GLOBOX_INTERACTIVE_STOP)
	{
		return;
	}

	// get window position
	struct macos_rect frame =
		macos_msg_rect_none(
			platform->globox_macos_obj_window,
			sel_getUid("frame"));

	struct macos_rect frame_old = frame;

	int16_t old_x = platform->globox_macos_interactive_x;
	int16_t old_y = platform->globox_macos_interactive_y;

	// get the latest local coordinates directly from the window
	query_pointer(globox);

	// compute window changes
	switch (globox->globox_interactive_mode)
	{
		case GLOBOX_INTERACTIVE_MOVE:
		{
			frame.origin.x += platform->globox_macos_interactive_x - old_x;
			frame.origin.y += platform->globox_macos_interactive_y - old_y;
			break;
		}
		case GLOBOX_INTERACTIVE_N:
		{
			frame.size.height += platform->globox_macos_interactive_y - old_y;
			break;
		}
		case GLOBOX_INTERACTIVE_NW:
		{
			frame.origin.x += platform->globox_macos_interactive_x - old_x;
			frame.size.width += old_x - platform->globox_macos_interactive_x;
			frame.size.height += platform->globox_macos_interactive_y - old_y;
			break;
		}
		case GLOBOX_INTERACTIVE_W:
		{
			frame.origin.x += platform->globox_macos_interactive_x - old_x;
			frame.size.width += old_x - platform->globox_macos_interactive_x;
			break;
		}
		case GLOBOX_INTERACTIVE_SW:
		{
			frame.origin.x += platform->globox_macos_interactive_x - old_x;
			frame.origin.y += platform->globox_macos_interactive_y - old_y;
			frame.size.width += old_x - platform->globox_macos_interactive_x;
			frame.size.height += old_y - platform->globox_macos_interactive_y;
			break;
		}
		case GLOBOX_INTERACTIVE_S:
		{
			frame.origin.y += platform->globox_macos_interactive_y - old_y;
			frame.size.height += old_y - platform->globox_macos_interactive_y;
			break;
		}
		case GLOBOX_INTERACTIVE_SE:
		{
			frame.origin.y += platform->globox_macos_interactive_y - old_y;
			frame.size.width += platform->globox_macos_interactive_x - old_x;
			frame.size.height += old_y - platform->globox_macos_interactive_y;
			break;
		}
		case GLOBOX_INTERACTIVE_E:
		{
			frame.size.width += platform->globox_macos_interactive_x - old_x;
			break;
		}
		case GLOBOX_INTERACTIVE_NE:
		{
			frame.size.width += platform->globox_macos_interactive_x - old_x;
			frame.size.height += platform->globox_macos_interactive_y - old_y;
			break;
		}
		default:
		{
			break;
		}
	}

	frame.origin.x = (int16_t) frame.origin.x;
	frame.origin.y = (int16_t) frame.origin.y;
	frame.size.width = (int16_t) frame.size.width;
	frame.size.height = (int16_t) frame.size.height;

	// too small
	if (frame.size.width < 25)
	{
		frame.size.width = 25;
	}

	if (frame.size.height < 35)
	{
		frame.size.height = 35;
	}

	// set window position
	struct macos_size size_view =
	{
		.width = frame.size.width,
		.height = frame.size.height - 22,
	};

	platform->globox_macos_interactive_x += frame_old.origin.x - frame.origin.x;
	platform->globox_macos_interactive_y += frame_old.origin.y - frame.origin.y;

	dispatch_semaphore_wait(platform->globox_macos_semaphore_draw, DISPATCH_TIME_FOREVER);

	// TODO use callback
#if 1
	if ((globox->globox_width * globox->globox_height) < (size_view.width * size_view.height))
	{
		// realloc the buffer if needed
		free(platform->globox_platform_argb);
		platform->globox_platform_argb = (uint32_t*) malloc(4 * size_view.width * size_view.height);
	}
#endif

	macos_msg_resize(
		platform->globox_macos_obj_window,
		sel_getUid("setFrame:display:"),
		frame,
		true);

	macos_msg_id_size(
		platform->globox_macos_obj_blur,
		sel_getUid("setFrameSize:"),
		size_view);

	macos_msg_id_size(
		platform->globox_macos_obj_view,
		sel_getUid("setFrameSize:"),
		size_view);

	globox->globox_redraw = true;

	dispatch_semaphore_signal(platform->globox_macos_semaphore_draw);
}

static void hover_cursor(struct globox* globox, struct macos_point pos)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	struct macos_rect frame =
		macos_msg_rect_none(
			platform->globox_macos_obj_window,
			sel_getUid("frame"));

	// abort if in fullscreen or frameless
	if ((platform->globox_macos_fullscreen == true)
	|| (globox->globox_frameless == true))
	{
		return;
	}

#if 1
	// abort if on button
	int i = 0;
	struct macos_rect button_frame;

	while (i < 3)
	{
		button_frame = platform->globox_macos_buttons[i];

		if ((frame.size.height - RESIZE_REACH_TITLEBAR + button_frame.origin.y > pos.y)
		|| (frame.size.height - RESIZE_REACH_TITLEBAR + button_frame.origin.y + button_frame.size.height < pos.y)
		|| (button_frame.origin.x > pos.x)
		|| (button_frame.origin.x + button_frame.size.width < pos.x))
		{
			++i;
		}
		else
		{
			platform->globox_inhibit_resize = true;
			return;
		}
	}

	platform->globox_inhibit_resize = false;
#endif

	enum system_cursor_hover old = 
		platform->globox_macos_cursor_hover;

	if ((pos.x > RESIZE_REACH_CORNER)
	&& (pos.x < (frame.size.width - RESIZE_REACH_CORNER))
	&& (pos.y > (frame.size.height - RESIZE_REACH_SIDE))
	&& (pos.y < (frame.size.height)))
	{
		platform->globox_macos_cursor_hover = HOVER_N;
		platform->globox_macos_cursor = CURSOR_NS;
	}
	else if ((pos.x > 0)
	&& (pos.x < (RESIZE_REACH_CORNER))
	&& (pos.y > (frame.size.height - RESIZE_REACH_CORNER))
	&& (pos.y < (frame.size.height)))
	{
		platform->globox_macos_cursor_hover = HOVER_NW;
		platform->globox_macos_cursor = CURSOR_NWSE;
	}
	else if ((pos.x > 0)
	&& (pos.x < (RESIZE_REACH_SIDE))
	&& (pos.y > (RESIZE_REACH_CORNER))
	&& (pos.y < (frame.size.height - RESIZE_REACH_CORNER)))
	{
		platform->globox_macos_cursor_hover = HOVER_W;
		platform->globox_macos_cursor = CURSOR_WE;
	}
	else if ((pos.x > 0)
	&& (pos.x < (RESIZE_REACH_CORNER))
	&& (pos.y > 0)
	&& (pos.y < (RESIZE_REACH_CORNER)))
	{
		platform->globox_macos_cursor_hover = HOVER_SW;
		platform->globox_macos_cursor = CURSOR_NESW;
	}
	else if ((pos.x > RESIZE_REACH_CORNER)
	&& (pos.x < (frame.size.width - RESIZE_REACH_CORNER))
	&& (pos.y > 0)
	&& (pos.y < (RESIZE_REACH_SIDE)))
	{
		platform->globox_macos_cursor_hover = HOVER_S;
		platform->globox_macos_cursor = CURSOR_NS;
	}
	else if ((pos.x > frame.size.width - RESIZE_REACH_CORNER)
	&& (pos.x < (frame.size.width))
	&& (pos.y > 0)
	&& (pos.y < (RESIZE_REACH_CORNER)))
	{
		platform->globox_macos_cursor_hover = HOVER_SE;
		platform->globox_macos_cursor = CURSOR_NWSE;
	}
	else if ((pos.x > frame.size.width - RESIZE_REACH_SIDE)
	&& (pos.x < (frame.size.width))
	&& (pos.y > (RESIZE_REACH_CORNER))
	&& (pos.y < (frame.size.height - RESIZE_REACH_CORNER)))
	{
		platform->globox_macos_cursor_hover = HOVER_E;
		platform->globox_macos_cursor = CURSOR_WE;
	}
	else if ((pos.x > frame.size.width - RESIZE_REACH_CORNER)
	&& (pos.x < (frame.size.width))
	&& (pos.y > (frame.size.height - RESIZE_REACH_CORNER))
	&& (pos.y < (frame.size.height)))
	{
		platform->globox_macos_cursor_hover = HOVER_NE;
		platform->globox_macos_cursor = CURSOR_NESW;
	}
	else if ((pos.x > 0)
	&& (pos.x < (frame.size.width))
	&& (pos.y > (frame.size.height - RESIZE_REACH_TITLEBAR))
	&& (pos.y < (frame.size.height)))
	{
		platform->globox_macos_cursor_hover = HOVER_TITLEBAR;
		platform->globox_macos_cursor = CURSOR_ARROW;
	}
	else
	{
		platform->globox_macos_cursor_hover = HOVER_NONE;
		platform->globox_macos_cursor = CURSOR_ARROW;
	}

	if ((old == HOVER_NONE)
	&& (platform->globox_macos_cursor_hover != HOVER_NONE))
	{
		platform->globox_macos_obj_cursor_old =
			macos_msg_idptr_none(
				(id) objc_getClass("NSCursor"),
				sel_getUid("currentCursor"));
	}
	else if ((old != HOVER_NONE)
	&& (platform->globox_macos_cursor_hover == HOVER_NONE))
	{
		macos_msg_void_none(
			(id) platform->globox_macos_obj_cursor_old,
			sel_getUid("set"));
	}
}

static void update_cursor(struct globox_platform* platform)
{
	if (platform->globox_macos_cursor_hover != HOVER_NONE)
	{
		if (platform->globox_macos_cursor_use_a == true)
		{
			macos_msg_id_none(
				platform->globox_macos_obj_cursor_b,
				sel_getUid("set"));

			platform->globox_macos_cursor_use_a = false;
		}
		else
		{
			macos_msg_id_none(
				platform->globox_macos_obj_cursor_a,
				sel_getUid("set"));

			platform->globox_macos_cursor_use_a = true;
		}
	}
}

void globox_platform_events_handle(
	struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	id* event =
		macos_msg_poll(
			platform->globox_platform_event_handle,
			sel_getUid("nextEventMatchingMask:untilDate:inMode:dequeue:"),
			NSEventMaskAny,
			NULL, // default to distantPast
			NSDefaultRunLoopMode,
			true);

	if (event == NULL)
	{
		return;
	}

	unsigned long event_type =
		macos_msg_ulong_none(
			event,
			sel_getUid("type"));

	if (event_type != NSEventTypeApplicationDefined)
	{
		if (event_type == NSEventTypeAppKitDefined)
		{
			short event_subtype =
				macos_msg_short_none(
					event,
					sel_getUid("subtype"));

			if (event_subtype == NSEventSubtypeApplicationActivated)
			{
				// test the latest local coordinates directly from the window
				struct macos_point pos =
					macos_msg_point_none(
						platform->globox_macos_obj_window,
						sel_getUid("mouseLocationOutsideOfEventStream"));

				hover_cursor(globox, pos);
			}
		}
		else if ((event_type == NSEventTypeMouseMoved)
		&& (globox->globox_interactive_mode == GLOBOX_INTERACTIVE_STOP))
		{
			// test the latest local coordinates directly from the window
			struct macos_point pos =
				macos_msg_point_none(
					platform->globox_macos_obj_window,
					sel_getUid("mouseLocationOutsideOfEventStream"));

			hover_cursor(globox, pos);
		}
		else if ((event_type == NSEventTypeLeftMouseDown)
		&& (globox->globox_interactive_mode == GLOBOX_INTERACTIVE_STOP)
		&& (platform->globox_inhibit_resize == false))
		{
			// test the latest local coordinates directly from the window
			struct macos_point pos =
				macos_msg_point_none(
					platform->globox_macos_obj_window,
					sel_getUid("mouseLocationOutsideOfEventStream"));

			switch (platform->globox_macos_cursor_hover)
			{
				case HOVER_N:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_N;
					break;
				}
				case HOVER_NW:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_NW;
					break;
				}
				case HOVER_W:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_W;
					break;
				}
				case HOVER_SW:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_SW;
					break;
				}
				case HOVER_S:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_S;
					break;
				}
				case HOVER_SE:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_SE;
					break;
				}
				case HOVER_E:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_E;
					break;
				}
				case HOVER_NE:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_NE;
					break;
				}
				case HOVER_TITLEBAR:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_MOVE;
					break;
				}
				default:
				{
					globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;
					break;
				}
			}

			// save coordinates
			if (globox->globox_interactive_mode != GLOBOX_INTERACTIVE_STOP)
			{
				platform->globox_macos_interactive_x = pos.x;
				platform->globox_macos_interactive_y = pos.y;
			}
		}
		else if (event_type == NSEventTypeLeftMouseUp)
		{
			globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;
		}

		update_cursor(platform);

		if (globox->globox_interactive_mode != GLOBOX_INTERACTIVE_STOP)
		{
			handle_interactive_mode(globox);
		}

		macos_msg_void_idptr(
			platform->globox_platform_event_handle,
			sel_getUid("sendEvent:"),
			event);

		return;
	}

	short event_subtype =
		macos_msg_short_none(
			event,
			sel_getUid("subtype"));

	long event_data =
		macos_msg_long_none(
			event,
			sel_getUid("data1"));

	switch (event_subtype)
	{
		case GLOBOX_MACOS_EVENT_WINDOW_STATE:
		{
			update_state(globox, event_data);
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_CLOSE:
		{
			globox->globox_closed = true;
			break;
		}
		case GLOBOX_MACOS_EVENT_WINDOW_RESIZE:
		{
			globox->globox_redraw = true;
			break;
		}
		default:
		{
			break;
		}
	}

	update_cursor(platform);
}

void globox_platform_free(struct globox* globox)
{
	// TODO
}

void globox_platform_set_icon(
	struct globox* globox,
	uint32_t* pixmap,
	uint32_t len)
{
	// TODO
}

void globox_platform_set_title(
	struct globox* globox,
	const char* title)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	char* tmp = strdup(title);

	if (globox->globox_title != NULL)
	{
		free(globox->globox_title);
	}

	globox->globox_title = tmp;

	id string =
		macos_msg_string_charptr_int(
			(id) objc_getClass("NSString"),
			sel_getUid("stringWithCString:encoding:"),
			tmp,
			NSUTF8StringEncoding);

	macos_msg_void_id(
		platform->globox_macos_obj_window,
		sel_getUid("setTitle:"),
		string);
}

void globox_platform_set_state(
	struct globox* globox,
	enum globox_state state)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	switch (state)
	{
		case GLOBOX_STATE_REGULAR:
		{
			if (globox->globox_state == GLOBOX_STATE_MINIMIZED)
			{
				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("deminiaturize:"),
					platform->globox_macos_obj_appdelegate);
			}
			else if (globox->globox_state == GLOBOX_STATE_MAXIMIZED)
			{
				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("zoom:"),
					platform->globox_macos_obj_appdelegate);
			}
			else if (globox->globox_state == GLOBOX_STATE_FULLSCREEN)
			{
				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("toggleFullScreen:"),
					platform->globox_macos_obj_appdelegate);
			}

			globox->globox_state = GLOBOX_STATE_REGULAR;

			break;
		}
		case GLOBOX_STATE_MAXIMIZED:
		{
			if (globox->globox_state == GLOBOX_STATE_MINIMIZED)
			{
				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("deminiaturize:"),
					platform->globox_macos_obj_appdelegate);
			}
			else if (globox->globox_state == GLOBOX_STATE_FULLSCREEN)
			{
				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("toggleFullScreen:"),
					platform->globox_macos_obj_appdelegate);
			}

			if (globox->globox_state != GLOBOX_STATE_MAXIMIZED)
			{
				globox->globox_state = GLOBOX_STATE_MAXIMIZED;

				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("zoom:"),
					platform->globox_macos_obj_appdelegate);
			}

			break;
		}
		case GLOBOX_STATE_MINIMIZED:
		{
			if (globox->globox_state != GLOBOX_STATE_MINIMIZED)
			{
				globox->globox_state = GLOBOX_STATE_MINIMIZED;

				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("miniaturize:"),
					platform->globox_macos_obj_appdelegate);
			}

			break;
		}
		case GLOBOX_STATE_FULLSCREEN:
		{
			if (globox->globox_state != GLOBOX_STATE_FULLSCREEN)
			{
				globox->globox_state = GLOBOX_STATE_FULLSCREEN;

				macos_msg_void_id(
					platform->globox_macos_obj_window,
					sel_getUid("toggleFullScreen:"),
					platform->globox_macos_obj_appdelegate);
			}

			break;
		}
	}

	globox->globox_state = state;
}

void globox_platform_interactive_mode(struct globox* globox, enum globox_interactive_mode mode)
{
	if ((mode != GLOBOX_INTERACTIVE_STOP)
		&& (globox->globox_interactive_mode != mode))
	{
		query_pointer(globox);

		if (globox_error_catch(globox))
		{
			return;
		}

		globox->globox_interactive_mode = mode;
	}
	else
	{
		globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;
	}
}

// getters
uint32_t* globox_platform_get_argb(struct globox* globox)
{
	return globox->globox_platform.globox_platform_argb;
}

id globox_platform_get_event_handle(struct globox* globox)
{
	return globox->globox_platform.globox_platform_event_handle;
}
