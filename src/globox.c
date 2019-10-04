#include "globox.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include <sys/ipc.h>
#include <sys/shm.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <stdlib.h>
#endif

#ifdef GLOBOX_WAYLAND
#include <wayland.h>
#endif

#ifdef GLOBOX_X11
static bool globox_open_x11(struct globox* globox)
{
	uint32_t values[2];

	// connect to server
	globox->conn = xcb_connect(NULL, &(globox->screen));

	// get screen pointer corresponding to preferred screen number
	const xcb_setup_t* setup = xcb_get_setup(globox->conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	for (int i = 0; i < globox->screen; ++i)
	{
		// there is no other documented way...
		xcb_screen_next(&iter);
	}

	xcb_screen_t* screen = iter.data;

	// get a free window id
	globox->win = xcb_generate_id(globox->conn);

	// create a window and attach it to this id
	xcb_visualid_t root_visual = screen->root_visual;
	values[0] = screen->black_pixel;
	values[1] = XCB_EVENT_MASK_EXPOSURE;

	xcb_create_window(
		globox->conn,
		24, // force 24bpp instead of XCB_COPY_FROM_PARENT
		globox->win,
		screen->root,
		globox->x,
		globox->y,
		globox->width,
		globox->height,
		0, // TODO window class
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		root_visual,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		values);

	// create a minimal graphic context
	globox->gfx = xcb_generate_id(globox->conn);

	values[0] = screen->black_pixel;
	values[1] = 0;

	xcb_create_gc(
		globox->conn,
		globox->gfx,
		globox->win,
		XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES,
		values);

	// map the window
	xcb_map_window(globox->conn, globox->win);

	// operations have no effect when the context is in failure state
	// so we can check it after going through the whole process
	if (xcb_connection_has_error(globox->conn) > 0)
	{
		return false;
	}

	// commit operations
	int ret = xcb_flush(globox->conn);

	// this probably doesn't count as a failure so we check it after
	// all the rest and abort if it didn't succeed
	if (ret <= 0)
	{
		return false;
	}

	// check if visual type is supported
	xcb_visualtype_t* visual = NULL;
	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
	xcb_visualtype_iterator_t visual_iter;

	while (depth_iter.rem)
	{
		visual_iter = xcb_depth_visuals_iterator(depth_iter.data);

		while (visual_iter.rem)
		{
			if (root_visual == visual_iter.data->visual_id)
			{
				visual = visual_iter.data;
				break;
			}

			xcb_visualtype_next(&visual_iter);
		}

		if (visual != NULL)
		{
			if ((visual->_class != XCB_VISUAL_CLASS_TRUE_COLOR)
				&& (visual->_class != XCB_VISUAL_CLASS_DIRECT_COLOR))
			{
				return false;
			}
			else
			{
				break;
			}
		}

		xcb_depth_next(&depth_iter);
	}

	// we are not done yet as we wish to bypass the xcb drawing API to
	// write directly to a shared memory buffer (just like CPU wayland)
	xcb_shm_query_version_reply_t* reply =
		xcb_shm_query_version_reply(
			globox->conn,
			xcb_shm_query_version(globox->conn),
			NULL);

	if (reply == NULL)
	{
		return false;
	}

	uint8_t shared = reply->shared_pixmaps;
	free(reply);

	// unlike wayland, X can't automatically copy buffers from cpu to gpu
	// so if the display server is running in DRM we need to do it manually
	// for this we can use xcb_put_image() to transfer the data over a socket
	if (shared == 0)
	{
		globox->socket = true;
	}
	else
	{
		// create the shared memory buffer
		globox->socket = false;
		globox->shm.shmid = shmget(
			IPC_PRIVATE,
			globox->width * globox->height * 4,
			IPC_CREAT | 0600);
		globox->shm.shmaddr = shmat(globox->shm.shmid, 0, 0);
		globox->shm.shmseg = xcb_generate_id(globox->conn);
		xcb_shm_attach(globox->conn, globox->shm.shmseg, globox->shm.shmid, 0);

		globox->pix = xcb_generate_id(globox->conn);

		shmctl(globox->shm.shmid, IPC_RMID, 0);

		globox->rgba = (uint32_t*) globox->shm.shmaddr;
		globox->comp = (uint8_t*) globox->shm.shmaddr;

		// disabled because we force 24bpp instead of retrieving the window's depth
#if 0
		xcb_get_geometry_cookie_t cookie = xcb_get_geometry(
			globox->conn,
			globox->win);
		xcb_get_geometry_reply_t* geometry = xcb_get_geometry_reply(
			globox->conn,
			cookie,
			NULL);
#endif

		// create pixmap with window depth
		xcb_shm_create_pixmap(
			globox->conn,
			globox->pix,
			globox->win,
			globox->width,
			globox->height,
			24, // force 24bpp instead of geometry->depth
			globox->shm.shmseg,
			0);
#if 0
		free(geometry);
#endif
	}

	if (xcb_connection_has_error(globox->conn) > 0)
	{
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

	if (ok == false)
	{
		xcb_disconnect(globox->conn);
	}
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
		if (globox->socket)
		{
		}
		else
		{
			xcb_shm_detach(globox->conn, globox->shm.shmseg);
			shmdt(globox->shm.shmaddr);
			xcb_free_pixmap(globox->conn, globox->pix);
			xcb_destroy_window(globox->conn, globox->win);
		}

		xcb_disconnect(globox->conn);

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

void globox_commit(struct globox* globox)
{
#ifdef GLOBOX_X11
	xcb_copy_area(
		globox->conn,
		globox->pix,
		globox->win,
		globox->gfx,
		0,
		0,
		0,
		0,
		globox->width,
		globox->height);

	xcb_flush(globox->conn);
#endif
}
