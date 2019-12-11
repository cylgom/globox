#define _XOPEN_SOURCE 700
#ifdef GLOBOX_X11

#include "x11.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/timerfd.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include "globox.h"

// get access to _NET_WM atoms without using ewmh
inline bool init_atoms(struct globox* globox)
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

inline xcb_screen_t* get_screen(struct globox* globox)
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
inline void create_window(struct globox* globox, xcb_screen_t* screen)
{
	// NTS
	// The XFCE background pixmap glitch can't be addressed
	// and is present everywhere, even in firefox and xfce4-terminal.
	// Since this is fucking X11 and the issue is minor anyway
	// it is probably better left as-is.
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
		globox->init_x,
		globox->init_y,
		globox->width,
		globox->height,
		0, // TODO window class
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		root_visual,
		XCB_CW_BACK_PIXMAP | XCB_CW_EVENT_MASK,
		values);
}

inline void create_gfx(struct globox* globox, xcb_screen_t* screen)
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
inline bool visual_compatible(struct globox* globox, xcb_screen_t* screen)
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
inline bool buffer_socket(struct globox* globox)
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
inline void buffer_shm(struct globox* globox)
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

// will loose all buffer information when resizing
inline bool globox_reserve(
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
inline void handle_title(struct globox* globox)
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
			globox_set_title(globox, value);
		}
	}

	free(reply);
}

// updates the internal state to reflect the actual window state
inline void handle_state(struct globox* globox)
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

inline void handle_expose(struct globox* globox, uint32_t* arr, uint8_t cur)
{
	for (uint8_t i = 0; i < cur; ++i)
	{
		globox_copy(
			globox,
			arr[(4 * i) + 0],
			arr[(4 * i) + 1],
			arr[(4 * i) + 2],
			arr[(4 * i) + 3]);
	}
}

// ask the server to change the window state
//
// there is a bug in ewmh that prevents fullscreen from working properly
// since keeping xcb-ewmh around only for initialization would be kind
// of silly we removed the dependency and used raw xcb all the way
void set_state(
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

void set_frame_timer(struct globox* globox)
{
	if (globox->frame_event)
	{
		// gets screen refresh rate
		xcb_randr_get_screen_info_cookie_t cookie = xcb_randr_get_screen_info(
			globox->x11_conn,
			globox->x11_win);

		xcb_generic_error_t* err = NULL;

		xcb_randr_get_screen_info_reply_t* reply = xcb_randr_get_screen_info_reply(
			globox->x11_conn,
			cookie,
			&err);

		// abort
		if (err != NULL)
		{
			return;
		}

		// timer init
		struct itimerspec timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_nsec = 1000000000 / reply->rate;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_nsec = 1000000000 / reply->rate;

		free(reply);
		timerfd_settime(globox->fd_frame, 0, &timer, NULL);
	}
}

#endif
