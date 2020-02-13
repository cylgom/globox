#include "quartz.h"
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include "globox.h"

// fucking Objective-C
id(*quartz_msg_id)(id, SEL) =
	(id(*)(id, SEL)) objc_msgSend;
void(*quartz_msg_void)(id, SEL) =
	(void(*)(id, SEL)) objc_msgSend;
void(*quartz_msg_ptr)(id, SEL, void*) =
	(void(*)(id, SEL, void*)) objc_msgSend;
id(*quartz_msg_rect)(id, SEL, struct quartz_rect) =
	(id(*)(id, SEL, struct quartz_rect)) objc_msgSend;
id(*quartz_msg_win)(id, SEL, struct quartz_rect, int, int, bool) =
	(id(*)(id, SEL, struct quartz_rect, int, int, bool)) objc_msgSend;

void quartz_rect(id app, SEL cmd, struct quartz_rect rect)
{
	id buf = quartz_msg_id(
		(id) objc_getClass("NSColor"),
		sel_getUid("redColor"));

	struct quartz_rect rect_new =
	{
		.origin.x = 21,
		.origin.y = 21,
		.size.width = 210,
		.size.height = 210,
	};

	quartz_msg_void(
		buf,
		sel_getUid("set"));

	NSRectFill(rect_new);
}

bool post_launch(struct quartz_app* app, SEL cmd, id msg)
{
	// parameters
	enum NSWindowStyleMask states =
		NSWindowStyleMaskTitled
		| NSWindowStyleMaskClosable
		| NSWindowStyleMaskMiniaturizable
		| NSWindowStyleMaskResizable
		| NSWindowStyleMaskFullScreen;
	
	struct quartz_rect rect_win =
	{
		.origin.x = 0,
		.origin.y = 0,
		.size.width = 1024,
		.size.height = 460,
	};
	
	struct quartz_rect rect_view =
	{
		.origin.x = 0,
		.origin.y = 0,
		.size.width = 320,
		.size.height = 480,
	};

	// window
	app->window = quartz_msg_id(
		(id) objc_getClass("NSWindow"),
		sel_getUid("alloc"));

	app->window = quartz_msg_win(
		app->window,
		sel_getUid("initWithContentRect:styleMask:backing:defer:"),
		rect_win,
		states,
		NSBackingStoreBuffered,
		false);
	
	// view
	id view = quartz_msg_id(
		(id) objc_getClass("View"),
		sel_getUid("alloc"));

	view = quartz_msg_rect(
		view,
		sel_getUid("initWithFrame:"),
		rect_view);

	// window + view
	quartz_msg_ptr(app->window, sel_getUid("setContentView:"), view);
	quartz_msg_id(app->window, sel_getUid("becomeFirstResponder"));
	quartz_msg_ptr(app->window, sel_getUid("makeKeyAndOrderFront:"), app);

	return true;
}
