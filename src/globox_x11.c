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
#include <stdio.h>

enum x11_atom_types
{
	ATOM_STATE_MAXIMIZED_HORZ = 0,
	ATOM_STATE_MAXIMIZED_VERT,
	ATOM_STATE_FULLSCREEN,
	ATOM_STATE,
	ATOM_ICON,
};

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

static inline void create_window(struct globox* globox, xcb_screen_t* screen)
{
	uint32_t values[2] =
	{
		screen->black_pixel,
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
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
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
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

static inline bool buffer_socket(struct globox* globox)
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

	return true;
}

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

static inline bool init_atoms(struct globox* globox)
{
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;
	xcb_generic_error_t* error;
	char* atoms_names[5] =
	{
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE",
		"_NET_WM_ICON",
	};

	for(uint8_t i = 0; i < 5; ++i)
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
			return false;
		}

		globox->x11_atoms[i] = reply->atom;

		free(reply);
	}

	return true;
}

bool globox_open_x11(struct globox* globox)
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

	// otherwise we confirm the X11 client successfully initialized
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

void globox_commit_x11(struct globox* globox)
{
	xcb_flush(globox->x11_conn);
}

void globox_copy_x11(
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

		xcb_create_pixmap(
			globox->x11_conn,
			24, // force 24bpp instead of geometry->depth
			globox->x11_pix,
			globox->x11_win,
			width,
			height);

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
					0,
					0,
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
			0,
			0,
			0,
			24,
			4 * width * height2,
			(void*) (globox->rgba + x + (y2 * globox->width)));

		xcb_copy_area(
			globox->x11_conn,
			globox->x11_pix,
			globox->x11_win,
			globox->x11_gfx,
			0,
			0,
			x,
			y,
			width,
			height);

		xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
	}
	else
	{
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
	}
}

// potentially loose all info in the buffer
inline bool globox_reserve_x11(
	struct globox* globox,
	uint32_t width,
	uint32_t height)
{
	if ((globox->buf_width * globox->buf_height) < (width * height))
	{
		printf("entering reserve | cur %d %d | new %d %d\n", globox->buf_width, globox->buf_height, width, height);
		xcb_generic_error_t* error;
		xcb_randr_get_screen_info_cookie_t screen_cookie;
		xcb_randr_get_screen_info_reply_t* screen_reply;
		screen_cookie = xcb_randr_get_screen_info(globox->x11_conn, globox->x11_win);
		screen_reply = xcb_randr_get_screen_info_reply(globox->x11_conn, screen_cookie, &error);

		if (error != NULL)
		{
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
			return false;
		}

		width = (1 + (width / win_reply->width)) * win_reply->width;
		height = (1 + (height / win_reply->height)) * win_reply->height;
		free(win_reply);

		globox->buf_width = width;
		globox->buf_height = height;

		printf("quitting reserve | %d %d\n", width, height);

		if (globox->x11_socket)
		{
			// should be faster than realloc
			free(globox->rgba);
			globox->rgba = malloc(4 * width * height);
		}
		else
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

			globox->rgba = (uint32_t*) globox->x11_shm.shmaddr;
		}
	}

	return (globox->rgba != NULL);
}

inline bool globox_shrink_x11(struct globox* globox)
{
	globox->buf_width = globox->width;
	globox->buf_height = globox->height;
	printf("shrinked to | %d %d\n", globox->buf_width, globox->buf_height);

	if (globox->x11_socket)
	{
		globox->rgba = realloc(globox->rgba, 4 * globox->width * globox->height);
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
		memcpy((uint32_t*) tmpaddr, globox->rgba, 4 * globox->width * globox->height);

		shmdt(globox->x11_shm.shmaddr);
		globox->x11_shm.shmaddr = tmpaddr;

		globox->rgba = (uint32_t*) globox->x11_shm.shmaddr;
	}

	return (globox->rgba != NULL);
}

bool globox_handle_events_x11(struct globox* globox)
{
	xcb_generic_event_t* event = xcb_poll_for_event(globox->x11_conn);
	bool ret = true;

	while ((event != NULL) && ret)
	{
		switch (event->response_type & ~0x80)
		{
			case XCB_EXPOSE:
			{
				xcb_expose_event_t* expose = (xcb_expose_event_t*) event;

				globox_copy_x11(globox,
					expose->x,
					expose->y,
					expose->width,
					expose->height);

				globox_commit_x11(globox);

				break;
			}
			case XCB_CONFIGURE_NOTIFY:
			{
				xcb_configure_notify_event_t* resize = (xcb_configure_notify_event_t*) event;
				ret = globox_reserve_x11(globox, resize->width, resize->height);

				break;
			}
		}

		free(event);
		event = xcb_poll_for_event(globox->x11_conn);
	}

	return ret;
}

void globox_set_title_x11(struct globox* globox, const char* title)
{
	xcb_change_property(
		globox->x11_conn,
		XCB_PROP_MODE_REPLACE,
		globox->x11_win,
		XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING,
		8,
		strlen(title),
		title);
}

// there is a bug in ewmh that prevents fullscreen from working properly
// since keeping xcb-ewmh around only for initialization would be kind
// of silly we removed the dependency and used raw xcb all the way
static inline void set_state(
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

void globox_set_state_x11(struct globox* globox, enum globox_state state)
{
	switch (state)
	{
		case GLOBOX_STATE_REGULAR:
		{
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 0);

			break;
		}
		case GLOBOX_STATE_MAXIMIZED:
		{
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 1);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 1);

			break;
		}
		case GLOBOX_STATE_FULLSCREEN:
		{
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_HORZ], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_MAXIMIZED_VERT], 0);
			set_state(globox, globox->x11_atoms[ATOM_STATE_FULLSCREEN], 1);

			break;
		}
	}

	globox->state = state;
}

void globox_set_pos_x11(struct globox* globox, uint32_t x, uint32_t y)
{
	uint32_t values[2] = {x, y};

	xcb_configure_window(
		globox->x11_conn,
		globox->x11_win,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
		values);
}

void globox_set_size_x11(struct globox* globox, uint32_t width, uint32_t height)
{
	uint32_t values[2] = {width, height};

	xcb_free_pixmap(globox->x11_conn, globox->x11_pix);
	xcb_shm_create_pixmap(
		globox->x11_conn,
		globox->x11_pix,
		globox->x11_win,
		width,
		height,
		24, // force 24bpp instead of geometry->depth
		globox->x11_shm.shmseg,
		0);

	xcb_configure_window(
		globox->x11_conn,
		globox->x11_win,
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
		values);
}

void globox_set_visible_x11(struct globox* globox, bool visible)
{
	if (globox->x11_visible && !visible)
	{
		xcb_unmap_window(globox->x11_conn, globox->x11_win);
	}
	else if (!(globox->x11_visible) && visible)
	{
		xcb_map_window(globox->x11_conn, globox->x11_win);
	}

	globox->x11_visible = visible;
}

void globox_set_icon_x11(struct globox* globox, uint32_t* pixmap, uint32_t len)
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

#endif
