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

void globox_commit(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
#ifdef GLOBOX_X11
	globox_commit_x11(
		globox,
		x,
		y,
		width,
		height);
#endif

#ifdef GLOBOX_WAYLAND
	globox_commit_wayland(
		globox,
		x,
		y,
		width,
		height);
#endif
}

void globox_refresh(struct globox* globox)
{
	globox_commit(
		globox,
		0,
		0,
		globox->width,
		globox->height);
}

bool globox_change_title(
	struct globox* globox,
	char* title)
{
	bool ok = true;

	return ok;
}

bool globox_change_state(
	struct globox* globox,
	enum globox_state state)
{
	bool ok = true;

	return ok;
}
