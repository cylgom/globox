#define _XOPEN_SOURCE 700
#ifdef GLOBOX_QUARTZ

#include "globox.h"
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "quartz.h"

extern id NSApp;

inline bool globox_open(
	struct globox* globox,
	enum globox_state state,
	const char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height,
	bool frame_event)
{
	// common init
	globox->init_x = x;
	globox->init_y = y;
	globox->width = width;
	globox->height = height;
	globox->buf_width = width;
	globox->buf_height = height;
	globox->redraw = true;
	globox->frame_event = frame_event;
	globox->closed = false;

	if (frame_event)
	{
		//globox->fd_frame = timerfd_create(CLOCK_REALTIME, 0);
	}

	globox->quartz_view = objc_allocateClassPair(
		(Class) objc_getClass("NSView"),
		"View",
		0);

	class_addMethod(
		globox->quartz_view,
		sel_getUid("drawRect:"),
		(IMP) quartz_rect,
		"v@:");

	objc_registerClassPair(globox->quartz_view);

	globox->quartz_app = objc_allocateClassPair(
		(Class) objc_getClass("NSObject"),
		"AppDelegate",
		0);

	class_addMethod(
		globox->quartz_app,
		sel_getUid("applicationDidFinishLaunching:"),
		(IMP) post_launch,
		"i@:@");

	objc_registerClassPair(globox->quartz_app);

	quartz_msg_void(
		(id) objc_getClass("NSApplication"),
		sel_getUid("sharedApplication"));

	if (NSApp == NULL)
	{
		return false;
	}

	id app_delegate_obj = quartz_msg_id(
		(id) objc_getClass("AppDelegate"),
		sel_getUid("alloc"));

	app_delegate_obj = quartz_msg_id(
		app_delegate_obj,
		sel_getUid("init"));

	// TODO ERROR
	quartz_msg_ptr(
		NSApp,
		sel_getUid("setDelegate:"),
		app_delegate_obj);

	quartz_msg_void(
		NSApp,
		sel_getUid("run"));

	globox->fd.descriptor = 0; // TODO
	globox->title = NULL;
	globox_set_title(globox, title);
	globox_set_state(globox, state);

	return true;
}

inline void globox_close(struct globox* globox)
{
	free(globox->argb);
}

// event queue processor with smart skipping for resizing and moving operations
inline bool globox_handle_events(struct globox* globox)
{
	return true;
}

inline bool globox_shrink(struct globox* globox)
{
#ifdef GLOBOX_RENDER_SWR
	globox->buf_width = globox->width;
	globox->buf_height = globox->height;
	globox->argb = realloc(globox->argb, 4 * globox->width * globox->height);

	return (globox->argb != NULL);
#else
	return true;
#endif
}

inline void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{

}

inline void globox_commit(struct globox* globox)
{

}

inline void globox_prepoll(struct globox* globox)
{
	// not used ATM
}

// direct icon change
inline void globox_set_icon(struct globox* globox, uint32_t* pixmap, uint32_t len)
{

}

inline void globox_set_title(struct globox* globox, const char* title)
{
	if (globox->title != NULL)
	{
		free(globox->title);
	}

	globox->title = strdup(title);
}

inline void globox_set_state(struct globox* globox, enum globox_state state)
{
	globox->state = state;
}

inline char* globox_get_title(struct globox* globox)
{
	return globox->title;
}

inline enum globox_state globox_get_state(struct globox* globox)
{
	return globox->state;
}

inline void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height)
{
	*width = globox->width;
	*height = globox->height;
}

#endif
