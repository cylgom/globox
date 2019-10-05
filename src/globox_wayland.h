#ifndef H_GLOBOX_WAYLAND
#define H_GLOBOX_WAYLAND

#include "globox.h"

#include <stdbool.h>

bool globox_open_wayland(struct globox* globox);
void globox_close_wayland(struct globox* globox);
void globox_copy_wayland(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

#endif
