#define _XOPEN_SOURCE 700
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// structs
enum NSStringEncoding
{
	NSUTF8StringEncoding = 4,
};

enum NSBackingStoreType
{
	NSBackingStoreBuffered = 2,
};

enum NSEventType
{
	NSEventTypeApplicationDefined = 15,
};

enum NSWindowStyleMask
{
	NSWindowStyleMaskTitled = 1,
	NSWindowStyleMaskClosable = 2,
	NSWindowStyleMaskMiniaturizable = 4,
	NSWindowStyleMaskResizable = 8,
	NSWindowStyleMaskFullScreen = 16384,
};

enum NSEventMask
{
	NSEventMaskAny = 0xFFFFFFFFFFFFFFFF,
};

struct quartz_point
{
	double x;
	double y;
};

struct quartz_size
{
	double width;
	double height;
};

struct quartz_rect
{
	struct quartz_point origin;
	struct quartz_size size;
};

struct quartz_app_delegate
{
	Class isa;
	id window;
};

// funcs
id(*quartz_msg_id)(id, SEL) =
	(id(*)(id, SEL)) objc_msgSend;

id*(*quartz_msg_date)(id, SEL) =
	(id*(*)(id, SEL)) objc_msgSend;

bool(*quartz_msg_bool)(id, SEL) =
	(bool(*)(id, SEL)) objc_msgSend;

void(*quartz_msg_void)(id, SEL) =
	(void(*)(id, SEL)) objc_msgSend;

unsigned long(*quartz_msg_type)(id*, SEL) =
	(unsigned long(*)(id*, SEL)) objc_msgSend;

void(*quartz_msg_ptr)(id, SEL, void*) =
	(void(*)(id, SEL, void*)) objc_msgSend;

void(*quartz_msg_obj)(id, SEL, id) =
	(void(*)(id, SEL, id)) objc_msgSend;

void(*quartz_msg_send)(id, SEL, id*) =
	(void(*)(id, SEL, id*)) objc_msgSend;

id(*quartz_msg_string)(id, SEL, const char*, unsigned long) =
	(id(*)(id, SEL, const char*, unsigned long)) objc_msgSend;

void(*quartz_msg_post)(id, SEL, id*, bool) =
	(void(*)(id, SEL, id*, bool)) objc_msgSend;

id*(*quartz_msg_event)(id, SEL, unsigned long, struct quartz_point, unsigned long, double, long, id, short, long, long) =
	(id*(*)(id, SEL, unsigned long, struct quartz_point, unsigned long, double, long, id, short, long, long)) objc_msgSend;

id(*quartz_msg_rect)(id, SEL, struct quartz_rect) =
	(id(*)(id, SEL, struct quartz_rect)) objc_msgSend;

id(*quartz_msg_win)(id, SEL, struct quartz_rect, int, int, bool) =
	(id(*)(id, SEL, struct quartz_rect, int, int, bool)) objc_msgSend;

id*(*quartz_msg_poll)(id, SEL, unsigned long long, id*, id*, bool) =
	(id*(*)(id, SEL, unsigned long long, id*, id*, bool)) objc_msgSend;

extern void* CGBitmapContextGetData(id);
extern void NSRectFill(struct quartz_rect rect);
extern SEL NSSelectorFromString(const char*);
extern id* NSDefaultRunLoopMode;

// tmp
union globox_event
{
	int descriptor;
	void* handle;
	id app;
};

enum globox_quartz_window_event
{
	GLOBOX_QUARTZ_WINDOW_EVENT_MAXIMIZE_TOGGLE = 0,
	GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_ON,
	GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_OFF,
	GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_ON,
	GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_OFF,
};

struct globox
{
	uint32_t init_x;
	uint32_t init_y;
	uint32_t width;
	uint32_t height;
	char* title;
	union globox_event fd;

	Class quartz_app_delegate_class;
	Class quartz_view_class;

	id quartz_app_delegate_obj;
	id quartz_view_obj;
};

struct globox ctx;

void handler(int sig)
{
	id* event = quartz_msg_poll(
		ctx.fd.app,
		sel_getUid("nextEventMatchingMask:untilDate:inMode:dequeue:"),
		NSEventMaskAny,
		NULL, // defaults to distantPast
		NSDefaultRunLoopMode,
		true);

	if (event != NULL)
	{
		unsigned long type =
			quartz_msg_type(event, sel_getUid("type"));

		printf("%lu\n", type);

		if (type == NSEventTypeApplicationDefined)
		{
			short subtype =
				quartz_msg_type(event, sel_getUid("subtype"));

			printf("custom event received: %hd\n", subtype);
		}
		else
		{
			quartz_msg_send(
				ctx.fd.app,
				sel_getUid("sendEvent:"),
				event);

			printf("event sent\n");
		}
	}
}

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

	printf("%p\n", ns_ctx);

	id cg_ctx = quartz_msg_id(
		ns_ctx,
		sel_getUid("CGContext"));

	printf("%p\n", cg_ctx);

	// will fail if the context is not a bitmap
	uint8_t* argb = CGBitmapContextGetData(cg_ctx);

	printf("%p\n", argb);

	for (int i = 0; i < 2000; ++i)
	{
		argb[i] = 0xFF;
	}
}

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

int main(int argc, char** argv)
{
	struct globox* globox = &ctx;

	// parameters
	char* title = "globox";

	// common init
	globox->init_x = 0;
	globox->init_y = 0;
	globox->width = 100;
	globox->height = 100;
	globox->title = NULL;

	// pre-init title
	globox->title = strdup(title);

// FUNC0
	// create View class
	globox->quartz_view_class = objc_allocateClassPair(
		(Class) objc_getClass("NSView"),
		"View",
		0);
	
	class_addMethod(
		globox->quartz_view_class,
		sel_getUid("drawRect:"),
		(IMP) quartz_view_draw_rect_callback,
		"v@:");
// END

// FUNC1
	// create AppDelegate class
	globox->quartz_app_delegate_class = objc_allocateClassPair(
		(Class) objc_getClass("NSObject"),
		"AppDelegate",
		0);

	// inject initialization method
	class_addMethod(
		globox->quartz_app_delegate_class, 
		sel_getUid("applicationDidFinishLaunching:"), 
		(IMP) quartz_app_delegate_init_callback,
		"i@:@");

	// inject globox context pointer
	class_addIvar(
		globox->quartz_app_delegate_class, 
		"globox",
		sizeof (void*),
		sizeof (void*),
		"^v");
// END

	id app = quartz_msg_id(
		(id) objc_getClass("NSApplication"),
		sel_getUid("sharedApplication"));

	globox->fd.app = app;

// FUNC3
	// create an AppDelegate instance
	globox->quartz_app_delegate_obj = quartz_msg_id(
		(id) globox->quartz_app_delegate_class,
		sel_getUid("alloc"));

	// set instance's globox context pointer
	object_setInstanceVariable(
		globox->quartz_app_delegate_obj,
		"globox",
		globox);

	// run AppDelegate init
	globox->quartz_app_delegate_obj = quartz_msg_id(
		globox->quartz_app_delegate_obj,
		sel_getUid("init"));
// END

// FUNC4
	// attach AppDelegate to NSApp singleton
	quartz_msg_ptr(
		app,
		sel_getUid("setDelegate:"),
		globox->quartz_app_delegate_obj);

	// we made it
	quartz_msg_void(
		app,
		sel_getUid("finishLaunching"));

	// create the happy little window
	quartz_msg_void(
		app,
		sel_getUid("run"));
// END

	while (quartz_msg_bool(app, sel_getUid("isRunning")));

// EXAMPLE LOOP
	id* future = quartz_msg_date(
		(id) objc_getClass("NSDate"),
		sel_getUid("distantFuture"));

	while (true)
	{
		id* event = quartz_msg_poll(
			ctx.fd.app,
			sel_getUid("nextEventMatchingMask:untilDate:inMode:dequeue:"),
			NSEventMaskAny,
			future,
			NSDefaultRunLoopMode,
			false);

		if (event != NULL)
		{
			handler(0);
		}
	}
// END EXAMPLE LOOP

	return 0;
}

// TODO (other globox functions have no effect under macOS)
// set state
// set title
// close
// commit
// get size
// get title
// get state
