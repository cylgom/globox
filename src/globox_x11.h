#ifndef H_GLOBOX_X11
#define H_GLOBOX_X11

#include "globox.h"

#include <stdbool.h>

bool globox_open_x11(struct globox* globox);
void globox_close_x11(struct globox* globox);
void globox_commit_x11(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

#endif
