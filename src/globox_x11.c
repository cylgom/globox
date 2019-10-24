#define _POSIX_C_SOURCE 200809L
#ifdef GLOBOX_X11

#include "globox_x11.h"
#include "globox.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/randr.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define EXPOSE_QUEUE_LEN 10

enum x11_atom_types
{
	ATOM_STATE_MAXIMIZED_HORZ = 0,
	ATOM_STATE_MAXIMIZED_VERT,
	ATOM_STATE_FULLSCREEN,
	ATOM_STATE_HIDDEN,
	ATOM_STATE,
	ATOM_ICON,
	ATOM_COUNT // used to get size
};

// get access to _NET_WM atoms without using ewmh
static inline bool init_atoms(struct globox* globox)
{
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;
	xcb_generic_error_t* error;
	char* atoms_names[ATOM_COUNT] =
	{
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE_HIDDEN",
		"_NET_WM_STATE",
		"_NET_WM_ICON",
	};

	for(uint8_t i = 0; i < ATOM_COUNT; ++i)
	{
		cookie = xcb_intern_atom(
			globox->x11_conn,
			0,
			strlen(atoms_names[i]),
			atoms_names[i]);

		reply = xcb_intern_atom_reply(
			globox->x11_conn,
			cookie,
			&error);

		if (error != NULL)
		{
			free(reply);

			return false;
		}

		globox->x11_atoms[i] = reply->atom;

		free(reply);
	}

	return true;
}

static inline xcb_screen_t* get_screen(struct globox* globox)
{
	const xcb_setup_t* setup = xcb_get_setup(globox->x11_conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	for (int i = 0; i < globox->x11_screen; ++i)
	{
		// there is no other documented way...
		xcb_screen_next(&iter);
	}

	return iter.data;
}

// we use a pixmap background instead of a plain color
// to work around resizing artifacts on some desktop environments
static inline void create_window(struct globox* globox, xcb_screen_t* screen)
{
	uint32_t values[2] =
	{
		XCB_BACK_PIXMAP_NONE,
		XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE,
	};

	globox->x11_win = xcb_generate_id(globox->x11_conn);

	xcb_visualid_t root_visual = screen->root_visual;

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
		XCB_CW_BACK_PIXMAP | XCB_CW_EVENT_MASK,
		values);
}

static inline void create_gfx(struct globox* globox, xcb_screen_t* screen)
{
	uint32_t values[2] =
	{
		screen->black_pixel,
		0,
	};

	globox->x11_gfx = xcb_generate_id(globox->x11_conn);

	xcb_create_gc(
		globox->x11_conn,
		globox->x11_gfx,
		globox->x11_win,
		XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES,
		values);
}

// check if the given screen offers a compatible mode
static inline bool visual_compatible(struct globox* globox, xcb_screen_t* screen)
{
	xcb_visualtype_t* visual = NULL;
	xcb_visualid_t root_visual = screen->root_visual;
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
				return true;
			}
		}

		xcb_depth_next(&depth_iter);
	}

	return false;
}

// classic pixmap allocation based on malloc
static inline bool buffer_socket(struct globox* globox)
{
	// transfer the data using a socket
	globox->x11_socket = true;
	// I have some bad news
	globox->argb = (uint32_t*) malloc(4 * globox->width * globox->height);

	if (globox->argb == NULL)
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

	return true;
}

// better buffer allocation based on shared-memory
static inline void buffer_shm(struct globox* globox)
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

	globox->argb = (uint32_t*) globox->x11_shm.shmaddr;

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

inline bool globox_open_x11(struct globox* globox, const char* title)
{
	// connect to server
	globox->x11_conn = xcb_connect(NULL, &(globox->x11_screen));

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
	globox_set_title_x11(globox, title);

	return true;
}

inline void globox_close_x11(struct globox* globox)
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

// will loose all buffer information when resizing
static inline bool globox_reserve(
	struct globox* globox,
	uint32_t width,
	uint32_t height)
{
	if (globox->x11_socket)
	{
		if ((globox->buf_width * globox->buf_height) < (width * height))
		{
			xcb_generic_error_t* error;
			xcb_randr_get_screen_info_cookie_t screen_cookie;
			xcb_randr_get_screen_info_reply_t* screen_reply;
			screen_cookie = xcb_randr_get_screen_info(globox->x11_conn, globox->x11_win);
			screen_reply = xcb_randr_get_screen_info_reply(globox->x11_conn, screen_cookie, &error);

			if (error != NULL)
			{
				free(screen_reply);

				return false;
			}

			xcb_window_t root = screen_reply->root;
			free(screen_reply);

			xcb_get_geometry_cookie_t win_cookie;
			xcb_get_geometry_reply_t* win_reply;
			win_cookie = xcb_get_geometry(globox->x11_conn, root);
			win_reply = xcb_get_geometry_reply(globox->x11_conn, win_cookie, &error);

			if (error != NULL)
			{
				free(win_reply);

				return false;
			}

			width = (1 + (width / win_reply->width)) * win_reply->width;
			height = (1 + (height / win_reply->height)) * win_reply->height;
			free(win_reply);

			globox->buf_width = width;
			globox->buf_height = height;

			// should be faster than realloc
			free(globox->argb);
			globox->argb = malloc(4 * width * height);
		}
	}
	else
	{
		if ((globox->buf_width * globox->buf_height) != (width * height))
		{
			// free
			xcb_shm_detach(globox->x11_conn, globox->x11_shm.shmseg);
			shmdt(globox->x11_shm.shmaddr);

			// malloc
			globox->x11_shm.shmid = shmget(
				IPC_PRIVATE,
				4 * width * height,
				IPC_CREAT | 0600);
			globox->x11_shm.shmaddr = shmat(globox->x11_shm.shmid, 0, 0);

			xcb_shm_attach(globox->x11_conn, globox->x11_shm.shmseg, globox->x11_shm.shmid, 0);
			shmctl(globox->x11_shm.shmid, IPC_RMID, 0);

			globox->argb = (uint32_t*) globox->x11_shm.shmaddr;
		}

		globox->buf_width = width;
		globox->buf_height = height;
	}

	globox->x11_pixmap_update = true;

	return (globox->argb != NULL);
}

// updates the internal title to reflect the actual window title
static inline void handle_title(struct globox* globox)
{
	// update internal title
	xcb_generic_error_t* error = NULL;
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t* reply;
	char* value;

	cookie = xcb_get_property(
		globox->x11_conn,
		0,
		globox->x11_win,
		XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING,
		0,
		32);

	reply = xcb_get_property_reply(
		globox->x11_conn,
		cookie,
		&error);

	if (error == NULL)
	{
		value = (char*) xcb_get_property_value(reply);

		if (value != NULL)
		{
			globox_set_title_x11(globox, value);
		}
	}

	free(reply);
}

// updates the internal state to reflect the actual window state
static inline void handle_state(struct globox* globox)
{
	xcb_generic_error_t* error = NULL;
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t* reply;
	xcb_atom_t* value;

	cookie = xcb_get_property(
		globox->x11_conn,
		0,
		globox->x11_win,
		globox->x11_atoms[ATOM_STATE],
		XCB_ATOM_ATOM,
		0,
		32);

	reply = xcb_get_property_reply(
		globox->x11_conn,
		cookie,
		&error);

	if (error != NULL)
	{
		free(reply);

		return;
	}

	value = (xcb_atom_t*) xcb_get_property_value(reply);

	if (value == NULL)
	{
		free(reply);

		return;
	}

	if (*value == globox->x11_atoms[ATOM_STATE_FULLSCREEN])
	{
		globox->state = GLOBOX_STATE_FULLSCREEN;
	}
	else if ((*value == globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT])
		|| (*value == globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ]))
	{
		globox->state = GLOBOX_STATE_MAXIMIZED;
	}
	else if (*value == globox->x11_atoms[ATOM_STATE_HIDDEN])
	{
		globox->state = GLOBOX_STATE_MINIMIZED;
	}
	else
	{
		globox->state = GLOBOX_STATE_REGULAR;
	}

	free(reply);
}

static inline void handle_expose(struct globox* globox, uint32_t* arr, uint8_t cur)
{
	for (uint8_t i = 0; i < cur; ++i)
	{
		globox_copy_x11(
			globox,
			arr[(4 * i) + 0],
			arr[(4 * i) + 1],
			arr[(4 * i) + 2],
			arr[(4 * i) + 3]);
	}
}

// event queue processor with smart skipping for resizing and moving operations
inline bool globox_handle_events_x11(struct globox* globox)
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

inline bool globox_shrink_x11(struct globox* globox)
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
inline void globox_copy_x11(
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

inline void globox_commit_x11(struct globox* globox)
{
	xcb_flush(globox->x11_conn);
}

// direct icon change
inline void globox_set_icon_x11(struct globox* globox, uint32_t* pixmap, uint32_t len)
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
inline void globox_set_title_x11(struct globox* globox, const char* title)
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

// ask the server to change the window state
//
// there is a bug in ewmh that prevents fullscreen from working properly
// since keeping xcb-ewmh around only for initialization would be kind
// of silly we removed the dependency and used raw xcb all the way
static void set_state(
	struct globox* globox,
	xcb_atom_t atom,
	uint32_t action)
{
	xcb_client_message_event_t ev;

	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.type = globox->x11_atoms[ATOM_STATE];
	ev.format = 32;
	ev.window = globox->x11_win;
	ev.data.data32[0] = action;
	ev.data.data32[1] = atom;
	ev.data.data32[2] = XCB_ATOM_NONE;
	ev.data.data32[3] = 0;
	ev.data.data32[4] = 0;

	xcb_send_event(
		globox->x11_conn,
		1,
		globox->x11_win,
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
		(const char*)(&ev));
}

// window states are really just "flags" for the server and can be combined
// because of this we need to set each relevant "flag" when changing state
inline void globox_set_state_x11(struct globox* globox, enum globox_state state)
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
inline void globox_set_pos_x11(struct globox* globox, uint32_t x, uint32_t y)
{
	uint32_t values[2] = {x, y};

	xcb_configure_window(
		globox->x11_conn,
		globox->x11_win,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
		values);
}

// ask the server to resize the window
inline bool globox_set_size_x11(struct globox* globox, uint32_t width, uint32_t height)
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

	return ret;
}

inline char* globox_get_title_x11(struct globox* globox)
{
	return globox->title;
}

inline enum globox_state globox_get_state_x11(struct globox* globox)
{
	return globox->state;
}

inline void globox_get_pos_x11(struct globox* globox, int32_t* x, int32_t* y)
{
	*x = globox->x;
	*y = globox->y;
}

inline void globox_get_size_x11(struct globox* globox, uint32_t* width, uint32_t* height)
{
	*width = globox->width;
	*height = globox->height;
}

#endif
