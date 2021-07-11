#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"

#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol.h"
#include "kde-blur-client-protocol.h"
#include "zwp-relative-pointer-protocol.h"
#include "zwp-pointer-constraints-protocol.h"

#include "wayland/globox_wayland.h"
#include "wayland/globox_wayland_callbacks.h"

void null_or_free(void* var)
{
	if (var != NULL)
	{
		free(var);
	}
}

void update_decorations(struct globox* globox)
{
	int error;
	struct globox_platform* platform = globox->globox_platform;

	if (platform->globox_wayland_xdg_decoration_manager == NULL)
	{
		return;
	}

	// create decoration listener
	platform->globox_wayland_xdg_decoration =
		zxdg_decoration_manager_v1_get_toplevel_decoration(
			platform->globox_wayland_xdg_decoration_manager,
			platform->globox_wayland_xdg_toplevel);

	if (platform->globox_wayland_xdg_decoration == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	error =
		zxdg_toplevel_decoration_v1_add_listener(
			platform->globox_wayland_xdg_decoration,
			&(platform->globox_wayland_xdg_decoration_listener),
			globox);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_LISTENER);
		return;
	}

	// send decoration method preferences
	uint32_t mode;

	if (globox->globox_frameless == true)
	{
		mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;
	}
	else
	{
		mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
	}

	zxdg_toplevel_decoration_v1_set_mode(
		platform->globox_wayland_xdg_decoration,
		mode);
}

// initalize the display system
void globox_platform_init(
	struct globox* globox,
	bool transparent,
	bool frameless,
	bool blurred)
{
	struct globox_platform* platform = malloc(sizeof (struct globox_platform));

	globox->globox_platform = platform;
	globox->globox_redraw = true;
	globox->globox_transparent = transparent;
	globox->globox_frameless = frameless;
	globox->globox_blurred = blurred;

	int error;

	platform->globox_wayland_saved_serial = 0;
	platform->globox_wayland_shm = NULL;
	platform->globox_wayland_compositor = NULL;
	platform->globox_wayland_xdg_decoration = NULL;
	platform->globox_wayland_kde_blur_manager = NULL;
	platform->globox_wayland_xdg_wm_base = NULL;
	platform->globox_wayland_output = NULL;
	platform->globox_wayland_seat = NULL;

	platform->globox_wayland_registry = NULL;
	platform->globox_wayland_xdg_toplevel = NULL;
	platform->globox_wayland_xdg_surface = NULL;
	platform->globox_wayland_surface = NULL;
	platform->globox_wayland_xdg_decoration_manager = NULL;
	platform->globox_wayland_kde_blur = NULL;

	platform->globox_wayland_output_registry = NULL;
	platform->globox_wayland_output_name = 0;
	platform->globox_wayland_output_data = globox;

	// base
	platform->globox_wayland_xdg_wm_base_listener.ping =
		callback_xdg_wm_base_ping;

	// output
	platform->globox_wayland_output_listener.geometry =
		callback_output_geometry;
	platform->globox_wayland_output_listener.mode =
		callback_output_mode;
	platform->globox_wayland_output_listener.done =
		callback_output_done;
	platform->globox_wayland_output_listener.scale =
		callback_output_scale;

	// registry
	platform->globox_wayland_registry_listener.global =
		callback_registry_global;
	platform->globox_wayland_registry_listener.global_remove =
		callback_registry_global_remove;

	// surfaces
	platform->globox_wayland_xdg_toplevel_listener.configure =
		callback_xdg_toplevel_configure;
	platform->globox_wayland_xdg_toplevel_listener.close =
		callback_xdg_toplevel_close;
	platform->globox_wayland_xdg_surface_listener.configure =
		callback_xdg_surface_configure;

	// decoration
	platform->globox_wayland_xdg_decoration_listener.configure =
		callback_xdg_decoration_configure;

	// connect to display
	platform->globox_wayland_display =
		wl_display_connect(
			NULL);

	if (platform->globox_wayland_display == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_DISPLAY);
		return;
	}

	// get register proxy
	platform->globox_wayland_registry =
		wl_display_get_registry(
			platform->globox_wayland_display);

	if (platform->globox_wayland_registry == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// get register proxy
	error =
		wl_registry_add_listener(
			platform->globox_wayland_registry,
			&(platform->globox_wayland_registry_listener),
			globox);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_LISTENER);
		return;
	}

	error =
		wl_display_roundtrip(
			platform->globox_wayland_display);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_ROUNDTRIP);
		return;
	}

	// event handle
	platform->globox_platform_event_handle =
		wl_display_get_fd(
			platform->globox_wayland_display);

	return;
}

// create the window
void globox_platform_create_window(struct globox* globox)
{
	int error;
	struct globox_platform* platform = globox->globox_platform;

	// wayland surface
	platform->globox_wayland_surface =
		wl_compositor_create_surface(
			platform->globox_wayland_compositor);

	if (platform->globox_wayland_surface == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// get xdg surface
	platform->globox_wayland_xdg_surface =
		xdg_wm_base_get_xdg_surface(
			platform->globox_wayland_xdg_wm_base,
			platform->globox_wayland_surface);

	if (platform->globox_wayland_xdg_surface == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// add surface listener
	error =
		xdg_surface_add_listener(
			platform->globox_wayland_xdg_surface,
			&(platform->globox_wayland_xdg_surface_listener),
			globox);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_LISTENER);
		return;
	}

	// get toplevel surface
	platform->globox_wayland_xdg_toplevel =
		xdg_surface_get_toplevel(
			platform->globox_wayland_xdg_surface);

	if (platform->globox_wayland_xdg_toplevel == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// add toplevel listener
	error =
		xdg_toplevel_add_listener(
			platform->globox_wayland_xdg_toplevel,
			&(platform->globox_wayland_xdg_toplevel_listener),
			globox);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_LISTENER);

		return;
	}

	return;
}

void globox_platform_hooks(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

	// platform update
	globox_platform_set_title(globox, globox->globox_title);
	globox_platform_set_state(globox, globox->globox_state);

	// register output callbacks
	if (platform->globox_wayland_output_registry != NULL)
	{
		platform->globox_wayland_output =
			wl_registry_bind(
				platform->globox_wayland_output_registry,
				platform->globox_wayland_output_name,
				&wl_output_interface,
				1);

		if (platform->globox_wayland_output == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_WAYLAND_REQUEST);
			return;
		}

		int error =
			wl_output_add_listener(
				platform->globox_wayland_output,
				&(platform->globox_wayland_output_listener),
				platform->globox_wayland_output_data);

		if (error == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_WAYLAND_LISTENER);
		}
	}

	// blur
	if ((platform->globox_wayland_kde_blur_manager != NULL)
		&& (globox->globox_transparent == true)
		&& (globox->globox_blurred == true))
	{
		platform->globox_wayland_kde_blur =
			org_kde_kwin_blur_manager_create(
				platform->globox_wayland_kde_blur_manager,
				platform->globox_wayland_surface);

		if (platform->globox_wayland_kde_blur == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_WAYLAND_REQUEST);
			return;
		}

		org_kde_kwin_blur_commit(
			platform->globox_wayland_kde_blur);
	}

	// epoll initialization
	platform->globox_wayland_epoll = epoll_create(1);

	if (platform->globox_wayland_epoll == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EPOLL_CREATE);
		return;
	}

	struct epoll_event ev =
	{
		EPOLLIN | EPOLLET,
		{0},
	};

	int error_epoll =
		epoll_ctl(
			platform->globox_wayland_epoll,
			EPOLL_CTL_ADD,
			platform->globox_platform_event_handle,
			&ev);

	if (error_epoll == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EPOLL_CTL);
		return;
	}
}

void globox_platform_commit(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

	wl_surface_commit(
		platform->globox_wayland_surface);
}

void globox_platform_prepoll(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

	int error =
		wl_display_dispatch(
			platform->globox_wayland_display);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_DISPATCH);
	}
}

void globox_platform_events_poll(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

	int error = wl_display_flush(platform->globox_wayland_display);

	if (error == -1)
	{
		do
		{
			epoll_wait(
				platform->globox_wayland_epoll,
				platform->globox_wayland_epoll_event,
				GLOBOX_CONST_MAX_WAYLAND_EVENTS,
				-1);

			error = wl_display_flush(platform->globox_wayland_display);
		}
		while (error == -1);
	}
	else
	{
		error =
			epoll_wait(
				platform->globox_wayland_epoll,
				platform->globox_wayland_epoll_event,
				GLOBOX_CONST_MAX_WAYLAND_EVENTS,
				0);

		if (error == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_WAYLAND_EPOLL_WAIT);
			return;
		}
	}

	if (error == 0)
	{
		return;
	}

	error = wl_display_dispatch(platform->globox_wayland_display);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_DISPATCH);
	}
}

void globox_platform_events_wait(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

	int error = wl_display_dispatch(platform->globox_wayland_display);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_DISPATCH);
	}
}

// TODO not tested
void globox_platform_interactive_mode(struct globox* globox, enum globox_interactive_mode mode)
{
	struct globox_platform* platform = globox->globox_platform;

	if ((mode != GLOBOX_INTERACTIVE_STOP)
		&& (globox->globox_interactive_mode != mode))
	{
		uint32_t edge;
		uint32_t serial = platform->globox_wayland_saved_serial;

		switch (mode)
		{
			case GLOBOX_INTERACTIVE_MOVE:
			{
				xdg_toplevel_move(
					platform->globox_wayland_xdg_toplevel,
					platform->globox_wayland_seat,
					serial);

				edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
				break;
			}
			case GLOBOX_INTERACTIVE_N:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP;
				break;
			}
			case GLOBOX_INTERACTIVE_NW:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
				break;
			}
			case GLOBOX_INTERACTIVE_W:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
				break;
			}
			case GLOBOX_INTERACTIVE_SW:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
				break;
			}
			case GLOBOX_INTERACTIVE_S:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
				break;
			}
			case GLOBOX_INTERACTIVE_SE:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
				break;
			}
			case GLOBOX_INTERACTIVE_E:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
				break;
			}
			case GLOBOX_INTERACTIVE_NE:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
				break;
			}
			default:
			{
				edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
				break;
			}
		}

		if (edge != XDG_TOPLEVEL_RESIZE_EDGE_NONE)
		{
			xdg_toplevel_resize(
				platform->globox_wayland_xdg_toplevel,
				platform->globox_wayland_seat,
				serial,
				edge);
		}

		globox->globox_interactive_mode = mode;
	}
	else
	{
		globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;
	}
}

void globox_platform_events_handle(
	struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

#if 0
	int error =
		wl_display_dispatch(
			platform->globox_wayland_display);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_DISPATCH);
		return;
	}
#endif

	platform->globox_wayland_callback_wait_copy(globox);
}

void globox_platform_free(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;

	if (platform->globox_wayland_kde_blur_manager != NULL)
	{
		free(platform->globox_wayland_kde_blur_manager);
		org_kde_kwin_blur_destroy(platform->globox_wayland_kde_blur);
	}

	if (platform->globox_wayland_xdg_decoration != NULL)
	{
		free(platform->globox_wayland_xdg_decoration);
		zxdg_decoration_manager_v1_destroy(platform->globox_wayland_xdg_decoration_manager);
	}

	null_or_free(platform->globox_wayland_seat);
	null_or_free(platform->globox_wayland_output);
	null_or_free(platform->globox_wayland_xdg_wm_base);
	null_or_free(platform->globox_wayland_compositor);
	null_or_free(platform->globox_wayland_shm);

	close(platform->globox_wayland_epoll);

	if (platform->globox_wayland_xdg_toplevel != NULL)
	{
		xdg_toplevel_destroy(platform->globox_wayland_xdg_toplevel);
	}

	if (platform->globox_wayland_xdg_surface != NULL)
	{
		xdg_surface_destroy(platform->globox_wayland_xdg_surface);
	}

	if (platform->globox_wayland_surface != NULL)
	{
		wl_surface_destroy(platform->globox_wayland_surface);
	}

	if (platform->globox_wayland_registry != NULL)
	{
		wl_registry_destroy(platform->globox_wayland_registry);
	}

	wl_display_disconnect(platform->globox_wayland_display);

	free(platform);
}

void globox_platform_set_icon(
	struct globox* globox,
	uint32_t* pixmap,
	uint32_t len)
{
	struct globox_platform* platform = globox->globox_platform;

	platform->globox_wayland_icon = pixmap;
	platform->globox_wayland_icon_len = len;
}

void globox_platform_set_title(
	struct globox* globox,
	const char* title)
{
	struct globox_platform* platform = globox->globox_platform;

	char* tmp = strdup(title);

	if (globox->globox_title != NULL)
	{
		free(globox->globox_title);
	}

	globox->globox_title = tmp;

	xdg_toplevel_set_title(
		platform->globox_wayland_xdg_toplevel,
		tmp);
}

void globox_platform_set_state(
	struct globox* globox,
	enum globox_state state)
{
	struct globox_platform* platform = globox->globox_platform;

	switch (state)
	{
		case GLOBOX_STATE_REGULAR:
		{
			xdg_toplevel_unset_maximized(
				platform->globox_wayland_xdg_toplevel);

			xdg_toplevel_unset_fullscreen(
				platform->globox_wayland_xdg_toplevel);

			if (globox->globox_state == GLOBOX_STATE_MINIMIZED)
			{
				// unmap memory (on software backend)
				platform->globox_wayland_unminimize_start(globox);

				if (globox_error_catch(globox))
				{
					return;
				}

				// destroy surfaces
				wl_surface_destroy(
					platform->globox_wayland_surface);
				xdg_surface_destroy(
					platform->globox_wayland_xdg_surface);
				xdg_toplevel_destroy(
					platform->globox_wayland_xdg_toplevel);

				// remap memory & create surfaces (on software backend)
				platform->globox_wayland_unminimize_finish(globox);

				if (globox_error_catch(globox))
				{
					return;
				}

				// reset title
				globox_platform_set_title(
					globox,
					globox->globox_title);
			}

			update_decorations(globox);

			break;
		}
		case GLOBOX_STATE_MAXIMIZED:
		{
			xdg_toplevel_unset_fullscreen(
				platform->globox_wayland_xdg_toplevel);
			xdg_toplevel_set_maximized(
				platform->globox_wayland_xdg_toplevel);

			update_decorations(globox);

			break;
		}
		case GLOBOX_STATE_MINIMIZED:
		{
			xdg_toplevel_unset_maximized(
				platform->globox_wayland_xdg_toplevel);
			xdg_toplevel_unset_fullscreen(
				platform->globox_wayland_xdg_toplevel);
			xdg_toplevel_set_minimized(
				platform->globox_wayland_xdg_toplevel);

			break;
		}
		case GLOBOX_STATE_FULLSCREEN:
		{
			xdg_toplevel_unset_maximized(
				platform->globox_wayland_xdg_toplevel);
			xdg_toplevel_set_fullscreen(
				platform->globox_wayland_xdg_toplevel,
				NULL);

			break;
		}
	}

	globox->globox_state = state;
}

// setters

void globox_wayland_save_serial(struct globox* globox, uint32_t serial)
{
	struct globox_platform* platform = globox->globox_platform;

	platform->globox_wayland_saved_serial = serial;
}

// getters

uint32_t* globox_platform_get_argb(struct globox* globox)
{
	return globox->globox_platform->globox_platform_argb;
}

int globox_platform_get_event_handle(struct globox* globox)
{
	return globox->globox_platform->globox_platform_event_handle;
}
