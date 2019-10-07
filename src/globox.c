#include "globox.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include "globox_x11.h"
#endif

#ifdef GLOBOX_WAYLAND
#include "globox_wayland.h"
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
	globox->width = width;
	globox->height = height;
	globox->buf_width = width;
	globox->buf_height = height;

#ifdef GLOBOX_WAYLAND
	ok = globox_open_wayland(globox);
#endif

#ifdef GLOBOX_X11
	#ifdef GLOBOX_WAYLAND
	if (ok == false)
	{
	#endif
		ok = globox_open_x11(globox);

		if (ok == false)
		{
			xcb_disconnect(globox->x11_conn);
		}
	#ifdef GLOBOX_WAYLAND
	}
	#endif
#endif

	return ok;
}

void globox_close(struct globox* globox)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_close_x11(globox);

		return;
	}
#endif

#ifdef GLOBOX_WAYLAND
	if (globox->backend == GLOBOX_BACKEND_WAYLAND)
	{
		globox_close_wayland(globox);

		return;
	}
#endif
}

void globox_commit(struct globox* globox)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_commit_x11(globox);
	}
#endif
}

void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_copy_x11(
			globox,
			x,
			y,
			width,
			height);
	}
#endif

#ifdef GLOBOX_WAYLAND
	if (globox->backend == GLOBOX_BACKEND_WAYLAND)
	{
		globox_copy_wayland(
			globox,
			x,
			y,
			width,
			height);
	}
#endif
}

bool globox_handle_events(struct globox* globox)
{
	bool ret = true;

#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		ret = globox_handle_events_x11(globox);
	}
#endif

	return ret;
}

void globox_set_title(struct globox* globox, char* title)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_set_title_x11(globox, title);
	}
#endif
}

void globox_set_state(struct globox* globox, enum globox_state state)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_set_state_x11(globox, state);
	}
#endif
}

void globox_set_pos(struct globox* globox, uint32_t x, uint32_t y)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_set_pos_x11(globox, x, y);
	}
#endif
}

bool globox_set_size(struct globox* globox, uint32_t width, uint32_t height)
{
	bool ret = true;

#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		ret = globox_set_size_x11(globox, width, height);
	}
#endif

	globox->width = width;
	globox->height = height;

	return ret;
}

bool globox_shrink(struct globox* globox)
{
	bool ret = true;

#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		ret = globox_shrink_x11(globox);
	}
#endif

	return ret;
}

void globox_set_visible(struct globox* globox, bool visible)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_set_visible_x11(globox, visible);
	}
#endif
}

void globox_set_icon(struct globox* globox, uint32_t* bgra)
{
#ifdef GLOBOX_X11
	if (globox->backend == GLOBOX_BACKEND_X11)
	{
		globox_set_icon_x11(globox, bgra, 2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
	}
#endif
}
