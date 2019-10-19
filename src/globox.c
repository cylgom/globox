#include "globox.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include "globox_x11.h"
#endif

#ifdef GLOBOX_WAYLAND
#include "globox_wayland.h"
#endif

// window creation
bool globox_open(
	struct globox* globox,
	enum globox_state state,
	char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	globox->x = x;
	globox->y = y;
	globox->width = width;
	globox->height = height;
	globox->buf_width = width;
	globox->buf_height = height;

#ifdef GLOBOX_X11
	return globox_open_x11(globox, title);
#endif

#ifdef GLOBOX_WAYLAND
	return globox_open_wayland(globox, title);
#endif
}

void globox_close(struct globox* globox)
{
#ifdef GLOBOX_X11
	globox_close_x11(globox);
#endif

#ifdef GLOBOX_WAYLAND
	globox_close_wayland(globox);
#endif
}

// buffer management
bool globox_handle_events(struct globox* globox)
{
#ifdef GLOBOX_X11
	return globox_handle_events_x11(globox);
#endif

#ifdef GLOBOX_WAYLAND
	return globox_handle_events_wayland(globox);
#endif
}

bool globox_shrink(struct globox* globox)
{
#ifdef GLOBOX_X11
	return globox_shrink_x11(globox);
#endif

#ifdef GLOBOX_WAYLAND
	return globox_shrink_wayland(globox);
#endif
}

// buffer transfer
void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
#ifdef GLOBOX_X11
	globox_copy_x11(
		globox,
		x,
		y,
		width,
		height);
#endif

#ifdef GLOBOX_WAYLAND
	globox_copy_wayland(
		globox,
		x,
		y,
		width,
		height);
#endif
}

void globox_commit(struct globox* globox)
{
#ifdef GLOBOX_X11
	globox_commit_x11(globox);
#endif

#ifdef GLOBOX_WAYLAND
	globox_commit_wayland(globox);
#endif
}

// setters
void globox_set_icon(struct globox* globox, uint32_t* bgra)
{
#ifdef GLOBOX_X11
	globox_set_icon_x11(globox, bgra, 2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
#endif

#ifdef GLOBOX_WAYLAND
	globox_set_icon_wayland(globox, bgra, 2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
#endif
}

void globox_set_title(struct globox* globox, char* title)
{
#ifdef GLOBOX_X11
	globox_set_title_x11(globox, title);
#endif

#ifdef GLOBOX_WAYLAND
	globox_set_title_wayland(globox, title);
#endif
}

void globox_set_state(struct globox* globox, enum globox_state state)
{
#ifdef GLOBOX_X11
	globox_set_state_x11(globox, state);
#endif

#ifdef GLOBOX_WAYLAND
	globox_set_state_wayland(globox, state);
#endif
}

void globox_set_pos(struct globox* globox, uint32_t x, uint32_t y)
{
#ifdef GLOBOX_X11
	globox_set_pos_x11(globox, x, y);
#endif

#ifdef GLOBOX_WAYLAND
	globox_set_pos_wayland(globox, x, y);
#endif
}

bool globox_set_size(struct globox* globox, uint32_t width, uint32_t height)
{
	bool ret = true;

#ifdef GLOBOX_X11
	ret = globox_set_size_x11(globox, width, height);
#endif

#ifdef GLOBOX_WAYLAND
	ret = globox_set_size_wayland(globox, width, height);
#endif

	globox->width = width;
	globox->height = height;

	return ret;
}

// getters
char* globox_get_title(struct globox* globox)
{
#ifdef GLOBOX_X11
	return globox_get_title_x11(globox);
#endif

#ifdef GLOBOX_WAYLAND
	return globox_get_title_wayland(globox);
#endif
}

enum globox_state globox_get_state(struct globox* globox)
{
#ifdef GLOBOX_X11
	return globox_get_state_x11(globox);
#endif

#ifdef GLOBOX_WAYLAND
	return globox_get_state_wayland(globox);
#endif
}

void globox_get_pos(struct globox* globox, int32_t* x, int32_t* y)
{
#ifdef GLOBOX_X11
	globox_get_pos_x11(globox, x, y);
#endif

#ifdef GLOBOX_WAYLAND
	globox_get_pos_wayland(globox, x, y);
#endif
}

void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height)
{
#ifdef GLOBOX_X11
	globox_get_size_x11(globox, width, height);
#endif

#ifdef GLOBOX_WAYLAND
	globox_get_size_wayland(globox, width, height);
#endif
}
