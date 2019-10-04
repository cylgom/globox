#ifdef GLOBOX_X11

#include "globox_x11.h"
#include "globox.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <stdbool.h>
#include <stdlib.h>

bool globox_open_x11(struct globox* globox)
{
	uint32_t values[2];

	// connect to server
	globox->x11_conn = xcb_connect(NULL, &(globox->x11_screen));

	// get screen pointer corresponding to preferred screen number
	const xcb_setup_t* setup = xcb_get_setup(globox->x11_conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	for (int i = 0; i < globox->x11_screen; ++i)
	{
		// there is no other documented way...
		xcb_screen_next(&iter);
	}

	xcb_screen_t* screen = iter.data;

	// get a free window id
	globox->x11_win = xcb_generate_id(globox->x11_conn);

	// create a window and attach it to this id
	xcb_visualid_t root_visual = screen->root_visual;
	values[0] = screen->black_pixel;
	values[1] = XCB_EVENT_MASK_EXPOSURE;

	xcb_create_window(
		globox->x11_conn,
		24, // force 24bpp instead of XCB_COPY_FROM_PARENT
		globox->x11_win,
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
	globox->x11_gfx = xcb_generate_id(globox->x11_conn);

	values[0] = screen->black_pixel;
	values[1] = 0;

	xcb_create_gc(
		globox->x11_conn,
		globox->x11_gfx,
		globox->x11_win,
		XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES,
		values);

	// map the window
	xcb_map_window(globox->x11_conn, globox->x11_win);

	// operations have no effect when the context is in failure state
	// so we can check it after going through the whole process
	if (xcb_connection_has_error(globox->x11_conn) > 0)
	{
		return false;
	}

	// commit operations
	int ret = xcb_flush(globox->x11_conn);

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
			globox->x11_conn,
			xcb_shm_query_version(globox->x11_conn),
			NULL);

	if (reply == NULL)
	{
		return false;
	}

	uint8_t shared = reply->shared_pixmaps;
	free(reply);

	// unlike wayland, X can't automatically copy buffers from cpu to gpu
	// so if the display server is running in DRM we need to do it manually
	// for this we can use xcb_put_image() to transfer the data using a socket
	if (shared == 0)
	{
		// transfer the data using a socket
		globox->x11_socket = true;
		// I have some bad news
		globox->rgba = (uint32_t*) malloc(4 * globox->width * globox->height);

		if (globox->rgba == NULL)
		{
			return false;
		}

		// create the pixmap
		globox->x11_pix = xcb_generate_id(globox->x11_conn);

		xcb_create_pixmap(
			globox->x11_conn,
			24, // force 24bpp instead of geometry->depth
			globox->x11_pix,
			globox->x11_win,
			globox->width,
			globox->height);
	}
	else
	{
		// create the shared memory buffer
		globox->x11_socket = false;
		globox->x11_shm.shmid = shmget(
			IPC_PRIVATE,
			globox->width * globox->height * 4,
			IPC_CREAT | 0600);
		globox->x11_shm.shmaddr = shmat(globox->x11_shm.shmid, 0, 0);
		globox->x11_shm.shmseg = xcb_generate_id(globox->x11_conn);
		xcb_shm_attach(globox->x11_conn, globox->x11_shm.shmseg, globox->x11_shm.shmid, 0);

		globox->x11_pix = xcb_generate_id(globox->x11_conn);

		shmctl(globox->x11_shm.shmid, IPC_RMID, 0);

		globox->rgba = (uint32_t*) globox->x11_shm.shmaddr;

		// create pixmap with window depth
		xcb_shm_create_pixmap(
			globox->x11_conn,
			globox->x11_pix,
			globox->x11_win,
			globox->width,
			globox->height,
			24, // force 24bpp instead of geometry->depth
			globox->x11_shm.shmseg,
			0);
	}

	if (xcb_connection_has_error(globox->x11_conn) > 0)
	{
		return false;
	}

	globox->backend = GLOBOX_BACKEND_X11;

	return true;
}

void globox_close_x11(struct globox* globox)
{
	if (globox->x11_socket)
	{
		free(globox->rgba);
	}
	else
	{
		xcb_shm_detach(globox->x11_conn, globox->x11_shm.shmseg);
		shmdt(globox->x11_shm.shmaddr);
		xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
		xcb_destroy_window(globox->x11_conn, globox->x11_win);
	}

	xcb_disconnect(globox->x11_conn);
}

void globox_commit_x11(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	if (globox->x11_socket)
	{
		size_t len = sizeof (xcb_get_image_request_t);
		size_t len_theoric = (len + (4 * globox->width * globox->height)) >> 2;
		uint64_t len_max = xcb_get_maximum_request_length(globox->x11_conn);

		int32_t y2 = y;
		uint32_t height2 = height;

		if (len_theoric >= len_max)
		{
			uint64_t rows_batch = ((len_max << 2) - len) / (4 * globox->width);

			while (rows_batch <= height2)
			{
				xcb_put_image(
					globox->x11_conn,
					XCB_IMAGE_FORMAT_Z_PIXMAP,
					globox->x11_pix,
					globox->x11_gfx,
					width,
					height2,
					x,
					y2,
					0,
					24,
					4 * width * height2,
					(void*) (globox->rgba + x + (y2 * globox->width)));

				y2 += rows_batch;
				height2 -= rows_batch;
			}
		}

		xcb_put_image(
			globox->x11_conn,
			XCB_IMAGE_FORMAT_Z_PIXMAP,
			globox->x11_pix,
			globox->x11_gfx,
			width,
			height2,
			x,
			y2,
			0,
			24,
			4 * width * height2,
			(void*) (globox->rgba + x + (y2 * globox->width)));
	}

	xcb_copy_area(
		globox->x11_conn,
		globox->x11_pix,
		globox->x11_win,
		globox->x11_gfx,
		x,
		y,
		x,
		y,
		width,
		height);

	xcb_flush(globox->x11_conn);
}

#endif
