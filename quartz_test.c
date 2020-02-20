#include <objc/message.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// structs
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

unsigned long(*quartz_msg_type)(id, SEL) =
	(unsigned long(*)(id, SEL)) objc_msgSend;

void(*quartz_msg_ptr)(id, SEL, void*) =
	(void(*)(id, SEL, void*)) objc_msgSend;

void(*quartz_msg_send)(id, SEL, id*) =
	(void(*)(id, SEL, id*)) objc_msgSend;

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

struct globox
{
	uint32_t init_x;
	uint32_t init_y;
	uint32_t width;
	uint32_t height;
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
		quartz_msg_send(
			ctx.fd.app,
			sel_getUid("sendEvent:"),
			event);

		printf("event sent\n");
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
	enum NSWindowStyleMask states =
#if 0
		NSWindowStyleMaskTitled
		| NSWindowStyleMaskClosable
		| NSWindowStyleMaskMiniaturizable
		| NSWindowStyleMaskResizable
		| NSWindowStyleMaskFullScreen;
#else
	0;
#endif

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

	return true;
}

int main(int argc, char** argv)
{
	struct globox* globox = &ctx;

	globox->init_x = 0;
	globox->init_y = 0;
	globox->width = 100;
	globox->height = 100;

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
// commit
// set state
// set title
// close
// get size
// get title
// get state