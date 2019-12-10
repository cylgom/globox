#define _XOPEN_SOURCE 500
#ifdef GLOBOX_WAYLAND

#include "globox.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xdg-shell-client-protocol.h"
#include "wayland.h"

inline bool globox_open(
	struct globox* globox,
	enum globox_state state,
	const char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	globox->init_x = x;
	globox->init_y = y;
	globox->width = width;
	globox->height = height;
	globox->buf_width = width;
	globox->buf_height = height;
	globox->redraw = true;

	// callbacks
	globox->wl_buffer_listener.release = wl_buffer_release;
	globox->xdg_surface_listener.configure = xdg_surface_configure;
	globox->xdg_wm_base_listener.ping = xdg_wm_base_ping;
	globox->wl_registry_listener.global = registry_global;
	globox->wl_registry_listener.global_remove = registry_global_remove;

	// regular initialization
	globox->wl_display = wl_display_connect(NULL);
    globox->wl_registry = wl_display_get_registry(globox->wl_display);
    wl_registry_add_listener(globox->wl_registry, &(globox->wl_registry_listener), globox);
    wl_display_roundtrip(globox->wl_display);

    globox->wl_surface = wl_compositor_create_surface(globox->wl_compositor);
    globox->xdg_surface = xdg_wm_base_get_xdg_surface(
		globox->xdg_wm_base,
		globox->wl_surface);

    xdg_surface_add_listener(globox->xdg_surface, &(globox->xdg_surface_listener), globox);
    globox->xdg_toplevel = xdg_surface_get_toplevel(globox->xdg_surface);

	globox->title = NULL;
	globox_set_title(globox, title);

    wl_surface_commit(globox->wl_surface);

	return true;
}

inline void globox_close(struct globox* globox)
{

}

inline bool globox_handle_events(struct globox* globox)
{
	return false;
}

inline bool globox_shrink(struct globox* globox)
{
	return false;
}

inline void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
}

inline void globox_commit(
	struct globox* globox)
{
    wl_surface_commit(globox->wl_surface);
}

inline void globox_set_icon(struct globox* globox, uint32_t* pixmap, uint32_t len)
{
	// wayland does not support server-side icons, but we could use one client-side
	globox->wl_icon = pixmap;
	globox->wl_icon_len = len;
}

inline void globox_set_title(struct globox* globox, const char* title)
{
	if (globox->title != NULL)
	{
		free(globox->title);
	}

	globox->title = strdup(title);

    xdg_toplevel_set_title(globox->xdg_toplevel, title);
}

inline void globox_set_state(struct globox* globox, enum globox_state state)
{
}

inline bool globox_set_size(struct globox* globox, uint32_t width, uint32_t height)
{
	return false;
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
