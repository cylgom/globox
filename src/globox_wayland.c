#ifdef GLOBOX_WAYLAND

#include "globox_wayland.h"
#include "globox.h"

#include <wayland-client.h>

bool globox_open_wayland(struct globox* globox)
{
	globox->backend = GLOBOX_BACKEND_WAYLAND;

	return false;
}

void globox_close_wayland(struct globox* globox)
{
}

void globox_commit_wayland(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
}

#endif
