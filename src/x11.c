#define _XOPEN_SOURCE 700
#ifdef GLOBOX_X11

#include "x11.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/timerfd.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>

#ifdef GLOBOX_RENDER_SWR
#include <sys/shm.h>
#include <xcb/shm.h>
#endif

#ifdef GLOBOX_RENDER_OGL
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif

#ifdef GLOBOX_RENDER_VLK
#define VK_USE_PLATFORM_XCB_KHR 1
#define PLATFORM_DEPENDENT_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#include <vulkan/vulkan.h>
#endif

#include "globox.h"

// get access to _NET_WM atoms without using ewmh
inline bool init_atoms(struct globox* globox)
{
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;
	xcb_generic_error_t* error;
	uint8_t replace;
	char* atoms_names[ATOM_COUNT] =
	{
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE_HIDDEN",
		"_NET_WM_STATE",
		"_NET_WM_ICON",
		"WM_PROTOCOLS",
		"WM_DELETE_WINDOW",
	};

	for (uint8_t i = 0; i < ATOM_COUNT; ++i)
	{
		if (i == ATOM_PROTOCOLS)
		{
			replace = 1;
		}
		else
		{
			replace = 0;
		}

		cookie = xcb_intern_atom(
			globox->x11_conn,
			replace,
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

	xcb_change_property(
		globox->x11_conn,
		XCB_PROP_MODE_REPLACE,
		globox->x11_win,
		globox->x11_atoms[ATOM_PROTOCOLS],
		4,
		32,
		1,
		&(globox->x11_atoms[ATOM_DELETE_WINDOW]));

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

#ifdef GLOBOX_RENDER_OGL
static int visual_attribs[] =
{
	GLX_X_RENDERABLE, True,
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE, GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
	GLX_RED_SIZE, 8,
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_ALPHA_SIZE, 8,
	GLX_DEPTH_SIZE, 24,
	GLX_STENCIL_SIZE, 8,
	GLX_DOUBLEBUFFER, True,
	None
};
#endif

// we use a pixmap background instead of a plain color
// to work around resizing artifacts on some desktop environments
//
// NTS: The XFCE background pixmap glitch can't be addressed
// and is present everywhere, even in firefox and xfce4-terminal.
// Since this is fucking X11 and the issue is minor anyway
// it is probably better left as-is.
inline void create_window(struct globox* globox, xcb_screen_t* screen)
{
#if defined(GLOBOX_RENDER_SWR) || defined(GLOBOX_RENDER_VLK)
	xcb_visualid_t visual_id = screen->root_visual;

	uint32_t value_list[2] =
	{
		XCB_BACK_PIXMAP_NONE,
		XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE,
	};

	uint32_t value_mask =
		XCB_CW_BACK_PIXMAP
		| XCB_CW_EVENT_MASK;
#endif

#ifdef GLOBOX_RENDER_OGL
	// get available framebuffer configurations
	int fb_config_count;
	GLXFBConfig *fb_config_list = glXChooseFBConfig(
		globox->xlib_display,
		globox->x11_screen,
		visual_attribs,
		&fb_config_count);

	if ((fb_config_list == NULL) || (fb_config_count == 0))
	{
		// TODO: error
		return;
	}

	// query visual ID
	int visual_id;
	globox->xlib_fb_config = fb_config_list[0];

	glXGetFBConfigAttrib(
		globox->xlib_display,
		globox->xlib_fb_config,
		GLX_VISUAL_ID,
		&visual_id);

	// create OGL context
	globox->xlib_context = glXCreateNewContext(
		globox->xlib_display,
		globox->xlib_fb_config,
		GLX_RGBA_TYPE,
		NULL,
		True);

	if (globox->xlib_context == NULL)
	{
		// TODO: error
		return;
	}

	xcb_colormap_t colormap = xcb_generate_id(globox->x11_conn);
	xcb_create_colormap(
		globox->x11_conn,
		XCB_COLORMAP_ALLOC_NONE,
		colormap,
		screen->root,
		visual_id);

	uint32_t value_list[3] =
	{
		XCB_BACK_PIXMAP_NONE,
		XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE,
		colormap,
	};

	uint32_t value_mask =
		XCB_CW_BACK_PIXMAP
		| XCB_CW_EVENT_MASK
		| XCB_CW_COLORMAP;
#endif

	globox->x11_win = xcb_generate_id(globox->x11_conn);

	xcb_create_window(
		globox->x11_conn,
		24, // force 24bpp instead of XCB_COPY_FROM_PARENT
		globox->x11_win,
		screen->root,
		globox->init_x,
		globox->init_y,
		globox->width,
		globox->height,
		0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		visual_id,
		value_mask,
		value_list);
}

inline void create_gfx(struct globox* globox, xcb_screen_t* screen)
{
#ifdef GLOBOX_RENDER_SWR
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
#endif
}

inline void create_glx(struct globox* globox)
{
#ifdef GLOBOX_RENDER_OGL
	globox->xlib_glx = glXCreateWindow(
		globox->xlib_display,
		globox->xlib_fb_config,
		globox->x11_win,
		NULL);

	if (globox->x11_win == 0)
	{
		xcb_destroy_window(globox->x11_conn, globox->x11_win);
		glXDestroyContext(globox->xlib_display, globox->xlib_context);

		// TODO: error
		return;
	}

	if (glXMakeContextCurrent(
		globox->xlib_display,
		globox->xlib_glx,
		globox->xlib_glx,
		globox->xlib_context) == False)
	{
		xcb_destroy_window(globox->x11_conn, globox->x11_win);
		glXDestroyContext(globox->xlib_display, globox->xlib_context);

		// TODO:error
		return;
	}
#endif
}

inline void create_vlk(struct globox* globox)
{
#ifdef GLOBOX_RENDER_VLK
	// link to the XCB window
	VkXcbSurfaceCreateInfoKHR surface_info =
	{
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = globox->x11_conn,
		.window = globox->x11_win,
	};

	VkResult ok = vkCreateXcbSurfaceKHR(
		globox->vlk_instance,
		&surface_info,
		globox->vlk_allocator,
		&(globox->vlk_surface));

	if (ok != VK_SUCCESS)
	{
		// TODO: error
		return;
	}
#endif
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
#ifdef GLOBOX_RENDER_SWR
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
#endif

	return true;
}

// better buffer allocation based on shared-memory
inline void buffer_shm(struct globox* globox)
{
#ifdef GLOBOX_RENDER_SWR
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
#endif
}

// will loose all buffer information when resizing
inline bool globox_reserve(
	struct globox* globox,
	uint32_t width,
	uint32_t height)
{
#ifdef GLOBOX_RENDER_SWR
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
#else
	return true;
#endif
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
#ifdef GLOBOX_RENDER_SWR
	for (uint8_t i = 0; i < cur; ++i)
	{
		globox_copy(
			globox,
			arr[(4 * i) + 0],
			arr[(4 * i) + 1],
			arr[(4 * i) + 2],
			arr[(4 * i) + 3]);
	}
#endif

#ifdef GLOBOX_RENDER_OGL
	glXSwapBuffers(globox->xlib_display, globox->xlib_glx);
#endif
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
