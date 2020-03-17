#ifdef GLOBOX_QUARTZ

#include "globox.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quartz.h"
#include "quartz_helpers.h"

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
	globox->redraw = false; // TODO set to true
	globox->frame_event = frame_event;
	globox->closed = false;

	globox->title = NULL;
	globox_set_title(globox, title);
	globox_set_state(globox, state);

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

	return true;
}

inline void globox_close(struct globox* globox)
{
	// TODO
	// handle the case where the main loop and context were nor properly ended
	// (I don't know how to do this yet, must do some research)
	free(globox->title);
}

inline bool globox_handle_events(struct globox* globox)
{
	id* event = quartz_msg_poll(
		globox->fd.app,
		sel_getUid("nextEventMatchingMask:untilDate:inMode:dequeue:"),
		NSEventMaskAny,
		NULL, // defaults to distantPast
		NSDefaultRunLoopMode,
		true);

	if (event != NULL)
	{
		unsigned long type =
			quartz_msg_type(event, sel_getUid("type"));

		printf("event type: %lu\n", type);

		if (type == NSEventTypeApplicationDefined)
		{
			short subtype =
				quartz_msg_type(event, sel_getUid("subtype"));
			long data =
				quartz_msg_type(event, sel_getUid("data1"));

			// window state change accounting 
			if (subtype == GLOBOX_QUARTZ_EVENT_WINDOW_STATE)
			{
				enum globox_state tmp_event = globox->state;

				switch (data)
				{
					case GLOBOX_QUARTZ_WINDOW_EVENT_MAXIMIZE_ON:
					{
						globox->state = GLOBOX_STATE_MAXIMIZED;
						break;
					}
					case GLOBOX_QUARTZ_WINDOW_EVENT_MAXIMIZE_OFF:
					{
						globox->state = GLOBOX_STATE_REGULAR;
						break;
					}
					case GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_ON:
					{
						globox->state = GLOBOX_STATE_MINIMIZED;
						break;
					}
					case GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_OFF:
					{
						globox->state = globox->quartz_state_old;
						break;
					}
					case GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_ON:
					{
						globox->state = GLOBOX_STATE_FULLSCREEN;
						break;
					}
					case GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_OFF:
					{
						globox->state = globox->quartz_state_old;
						break;
					}
				}

				globox->quartz_state_old = tmp_event;

				printf("state: %u\n", globox->state);
			}

			printf("custom event received: %hd\n", subtype);
		}
		else
		{
			// TODO proper event handling
			// set close var on close
			quartz_msg_send(
				globox->fd.app,
				sel_getUid("sendEvent:"),
				event);

			printf("event sent\n");
		}
	}

	return true;
}

inline bool globox_shrink(struct globox* globox)
{

	return true;
}

inline void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	globox->redraw = false;
}

inline void globox_commit(struct globox* globox)
{

}

inline void globox_prepoll(struct globox* globox)
{
	// not used ATM
}

inline void globox_set_icon(struct globox* globox, uint32_t* pixmap, uint32_t len)
{
	// not used ATM
}

// direct title change
inline void globox_set_title(struct globox* globox, const char* title)
{
	if (globox->title != NULL)
	{
		free(globox->title);
	}

	globox->title = strdup(title);

	// TODO
}

inline void globox_set_state(struct globox* globox, enum globox_state state)
{
	switch (state)
	{
		// TODO
		case GLOBOX_STATE_REGULAR:
		{

			break;
		}
		case GLOBOX_STATE_MAXIMIZED:
		{

			break;
		}
		case GLOBOX_STATE_MINIMIZED:
		{

			break;
		}
		case GLOBOX_STATE_FULLSCREEN:
		{

			break;
		}
	}

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
