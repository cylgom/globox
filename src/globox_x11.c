#define _POSIX_C_SOURCE 200809L
#ifdef GLOBOX_X11

#include "globox.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <xcb/shm.h>
#include "x11.h"

#define EXPOSE_QUEUE_LEN 10

inline bool globox_open(
	struct globox* globox,
	enum globox_state state,
	const char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	// common init
	globox->x = x;
	globox->y = y;
	globox->width = width;
	globox->height = height;
	globox->buf_width = width;
	globox->buf_height = height;
	globox->redraw = true;

	// connect to server
	globox->x11_conn = xcb_connect(NULL, &(globox->x11_screen));
	globox->fd = xcb_get_file_descriptor(globox->x11_conn);

	// provide ewmh-dependant functions
	if (!init_atoms(globox))
	{
		return false;
	}

	xcb_screen_t* screen = get_screen(globox);

	// create the window
	create_window(globox, screen);
	create_gfx(globox, screen);
	xcb_map_window(globox->x11_conn, globox->x11_win);
	globox->x11_pixmap_update = false;
	globox->x11_visible = true;

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

	// check display server settings compatibility
	if (!visual_compatible(globox, screen))
	{
		return false;
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
		if (!buffer_socket(globox))
		{
			return false;
		}
	}
	else
	{
		buffer_shm(globox);
	}

	// still time to abort if buffer allocation operations fucked the server
	if (xcb_connection_has_error(globox->x11_conn) > 0)
	{
		return false;
	}

	globox->title = NULL;
	globox_set_title(globox, title);

	return true;
}

inline void globox_close(struct globox* globox)
{
	if (globox->x11_socket)
	{
		free(globox->argb);
		xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
	}
	else
	{
		xcb_shm_detach(globox->x11_conn, globox->x11_shm.shmseg);
		shmdt(globox->x11_shm.shmaddr);
		xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
		xcb_destroy_window(globox->x11_conn, globox->x11_win);
	}

	free(globox->title);
	xcb_disconnect(globox->x11_conn);
}

// event queue processor with smart skipping for resizing and moving operations
inline bool globox_handle_events(struct globox* globox)
{
	xcb_generic_event_t* event = xcb_poll_for_event(globox->x11_conn);
	xcb_expose_event_t* expose = NULL;
	xcb_configure_notify_event_t* resize = NULL;
	xcb_property_notify_event_t* state = NULL;
	bool redraw = false;
	bool ret = true;

	uint32_t queue[4 * EXPOSE_QUEUE_LEN];
	uint8_t cur = 0;

	while ((event != NULL) && ret)
	{
		switch (event->response_type & ~0x80)
		{
			case XCB_EXPOSE:
			{
				expose = (xcb_expose_event_t*) event;

				if (!redraw)
				{
					if (cur >= EXPOSE_QUEUE_LEN)
					{
						handle_expose(globox, queue, EXPOSE_QUEUE_LEN);
						cur = 0;
					}

					queue[(4 * cur) + 0] = expose->x;
					queue[(4 * cur) + 1] = expose->y;
					queue[(4 * cur) + 2] = expose->width;
					queue[(4 * cur) + 3] = expose->height;
					++cur;
				}

				free(expose);

				break;
			}
			case XCB_CONFIGURE_NOTIFY:
			{
				if (resize != NULL)
				{
					free(resize);
				}

				resize = (xcb_configure_notify_event_t*) event;

				if (!redraw && ((resize->width != globox->width) || (resize->height != globox->height)))
				{
					redraw = true;
					cur = 0;
				}

				break;
			}
			case XCB_PROPERTY_NOTIFY:
			{
				if (state == NULL)
				{
					state = (xcb_property_notify_event_t*) event;

					if ((state->atom != globox->x11_atoms[ATOM_STATE])
						&& (state->atom != XCB_ATOM_WM_NAME))
					{
						free(state);
						state = NULL;
					}
				}

				break;
			}
			default:
			{
				free(event);

				break;
			}
		}

		event = xcb_poll_for_event(globox->x11_conn);
	}

	if (resize != NULL)
	{
		if (redraw)
		{
			ret = globox_reserve(globox, resize->width, resize->height);

			globox->redraw = true;
			globox->width = resize->width;
			globox->height = resize->height;
		}

		globox->x = resize->x;
		globox->y = resize->y;

		free(resize);
	}

	if (cur > 0)
	{
		handle_expose(globox, queue, cur);
	}

	if (state != NULL)
	{
		handle_title(globox);
		handle_state(globox);

		free(state);
	}

	return ret;
}

inline bool globox_shrink(struct globox* globox)
{
	globox->buf_width = globox->width;
	globox->buf_height = globox->height;

	if (globox->x11_socket)
	{
		globox->argb = realloc(globox->argb, 4 * globox->width * globox->height);
	}
	else
	{
		globox->x11_shm.shmid = shmget(
			IPC_PRIVATE,
			4 * globox->width * globox->height,
			IPC_CREAT | 0600);
		uint8_t* tmpaddr = shmat(globox->x11_shm.shmid, 0, 0);

		xcb_shm_detach(globox->x11_conn, globox->x11_shm.shmseg);
		xcb_shm_attach(globox->x11_conn, globox->x11_shm.shmseg, globox->x11_shm.shmid, 0);

		shmctl(globox->x11_shm.shmid, IPC_RMID, 0);
		memcpy((uint32_t*) tmpaddr, globox->argb, 4 * globox->width * globox->height);

		shmdt(globox->x11_shm.shmaddr);
		globox->x11_shm.shmaddr = tmpaddr;

		globox->argb = (uint32_t*) globox->x11_shm.shmaddr;
	}

	return (globox->argb != NULL);
}

// draw a part of the buffer on the screen
// re-allocate the pixmap here to avoid artifacts
inline void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	if (globox->x11_socket)
	{
		int32_t y2 = y;
		uint32_t height2 = height;

		size_t len = sizeof (xcb_get_image_request_t);
		size_t len_theoric = (len + (4 * globox->width * height2)) >> 2;
		uint64_t len_max = xcb_get_maximum_request_length(globox->x11_conn);

		if (globox->x11_pixmap_update)
		{
			xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
			xcb_create_pixmap(
				globox->x11_conn,
				24, // force 24bpp instead of geometry->depth
				globox->x11_pix,
				globox->x11_win,
				globox->width,
				globox->height);
			globox->x11_pixmap_update = false;
		}

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
					globox->width,
					rows_batch,
					0,
					y2,
					0,
					24,
					4 * globox->width * rows_batch,
					(void*) (globox->argb + (y2 * globox->width)));

				y2 += rows_batch;
				height2 -= rows_batch;
			}
		}

		xcb_put_image(
			globox->x11_conn,
			XCB_IMAGE_FORMAT_Z_PIXMAP,
			globox->x11_pix,
			globox->x11_gfx,
			globox->width,
			height2,
			0,
			y2,
			0,
			24,
			4 * globox->width * height2,
			(void*) (globox->argb + (y2 * globox->width)));
	}
	else if (globox->x11_pixmap_update)
	{
		xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
		xcb_shm_create_pixmap(
			globox->x11_conn,
			globox->x11_pix,
			globox->x11_win,
			globox->width,
			globox->height,
			24, // force 24bpp instead of geometry->depth
			globox->x11_shm.shmseg,
			0);
		globox->x11_pixmap_update = false;
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
	globox->redraw = false;
}

inline void globox_commit(struct globox* globox)
{
	xcb_flush(globox->x11_conn);
}

// direct icon change
inline void globox_set_icon(struct globox* globox, uint32_t* pixmap, uint32_t len)
{
	xcb_change_property(
		globox->x11_conn,
		XCB_PROP_MODE_REPLACE,
		globox->x11_win,
		globox->x11_atoms[ATOM_ICON],
		XCB_ATOM_CARDINAL,
		32,
		len,
		pixmap);

	xcb_flush(globox->x11_conn);
}

// direct title change
inline void globox_set_title(struct globox* globox, const char* title)
{
	if (globox->title != NULL)
	{
		free(globox->title);
	}

	globox->title = strdup(title);

	xcb_change_property(
		globox->x11_conn,
		XCB_PROP_MODE_REPLACE,
		globox->x11_win,
		XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING,
		8,
		strlen(title) + 1,
		title);
}

// window states are really just "flags" for the server and can be combined
// because of this we need to set each relevant "flag" when changing state
inline void globox_set_state(struct globox* globox, enum globox_state state)
{
	switch (state)
	{
		case GLOBOX_STATE_REGULAR:
		{
			xcb_map_window(globox->x11_conn, globox->x11_win);
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 0);

			break;
		}
		case GLOBOX_STATE_MAXIMIZED:
		{
			xcb_map_window(globox->x11_conn, globox->x11_win);
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 1);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 1);

			break;
		}
		case GLOBOX_STATE_MINIMIZED:
		{
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 0);
			xcb_unmap_window(globox->x11_conn, globox->x11_win);

			break;
		}
		case GLOBOX_STATE_FULLSCREEN:
		{
			xcb_map_window(globox->x11_conn, globox->x11_win);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 1);

			break;
		}
	}

	globox->state = state;
}

// ask the server to move the window
inline void globox_set_pos(struct globox* globox, uint32_t x, uint32_t y)
{
	uint32_t values[2] = {x, y};

	xcb_configure_window(
		globox->x11_conn,
		globox->x11_win,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
		values);
}

// ask the server to resize the window
inline bool globox_set_size(struct globox* globox, uint32_t width, uint32_t height)
{
	uint32_t values[2] = {width, height};

	// we know the window will be resized so we can
	// execute globox_reserve now to try to allocate
	// the required resources as soon as possible and
	// reduce artifacts on weird desktop environments
	bool ret = globox_reserve(globox, width, height);

	if (ret)
	{
		xcb_configure_window(
			globox->x11_conn,
			globox->x11_win,
			XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
			values);
	}

	globox->width = width;
	globox->height = height;

	return ret;
}

inline char* globox_get_title(struct globox* globox)
{
	return globox->title;
}

inline enum globox_state globox_get_state(struct globox* globox)
{
	return globox->state;
}

inline void globox_get_pos(struct globox* globox, int32_t* x, int32_t* y)
{
	*x = globox->x;
	*y = globox->y;
}

inline void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height)
{
	*width = globox->width;
	*height = globox->height;
}

#endif
