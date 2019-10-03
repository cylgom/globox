#include "globox.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include <xcb/xcb.h>
#endif

#ifdef GLOBOX_WAYLAND
#include <wayland.h>
#endif

#ifdef GLOBOX_X11
static bool globox_open_x11(struct globox* globox)
{
	// connect to server
	globox->server_conn = xcb_connect(NULL, &(globox->preferred_screen));

	// get screen pointer corresponding to preferred screen number
	const xcb_setup_t* setup = xcb_get_setup(globox->server_conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	for (int i = 0; i < globox->preferred_screen; ++i)
	{
		// there is no other documented way...
		xcb_screen_next(&iter);
	}

	xcb_screen_t* screen = iter.data;

	// get a free window id
	globox->window_id = xcb_generate_id(globox->server_conn);

	// create a window and attach it to this id
	xcb_create_window(
		globox->server_conn,
		XCB_COPY_FROM_PARENT,
		globox->window_id,
		screen->root,
		globox->x,
		globox->y,
		globox->width,
		globox->height,
		0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		0,
		NULL);

	// map the window
	xcb_map_window(globox->server_conn, globox->window_id);

	// operations have no effect when the context is in failure state
	// so we can check it after going through the whole process
	if (xcb_connection_has_error(globox->server_conn) > 0)
	{
		xcb_disconnect(globox->server_conn);

		return false;
	}

	// commit operations
	int ret = xcb_flush(globox->server_conn);

	// this probably doesn't count as a failure so we check it after
	// all the rest and abort if it didn't succeed
	if (ret <= 0)
	{
		xcb_disconnect(globox->server_conn);

		return false;
	}

	globox->backend = GLOBOX_BACKEND_X11;

	return true;
}
#endif

#ifdef GLOBOX_WAYLAND
static bool globox_open_wayland(struct globox* globox)
{
	globox->backend = GLOBOX_BACKEND_WAYLAND;

	return true;
}
#endif

bool globox_open(
	struct globox* globox,
	enum globox_state state,
	char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	// in case GLOBOX_WAYLAND is not defined
	bool ok = true;

	globox->x = x;
	globox->y = y;
	globox->height = height;
	globox->width = width;

#ifdef GLOBOX_WAYLAND
	ok = globox_open_wayland(globox);
#endif

#ifdef GLOBOX_X11
	#ifdef GLOBOX_WAYLAND
	if (ok == false)
	{
	#endif
		ok = globox_open_x11(globox);
	#ifdef GLOBOX_WAYLAND
	}
	#endif
#endif

	return ok;
}

void globox_close(struct globox* globox)
{
#ifdef GLOBOX_WAYLAND
	if (globox->backend == GLOBOX_BACKEND_WAYLAND)
	{
		return;
	}
#endif

#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		xcb_disconnect(globox->server_conn);
		return;
	}
#endif
}

bool globox_change_title(
	struct globox* globox,
	char* title)
{
	bool ok = true;

	return ok;
}

bool globox_change_state(
	struct globox* globox,
	enum globox_state state)
{
	bool ok = true;

	return ok;
}
