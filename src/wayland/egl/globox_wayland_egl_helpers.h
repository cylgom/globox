#ifndef H_GLOBOX_WAYLAND_EGL_HELPERS
#define H_GLOBOX_WAYLAND_EGL_HELPERS

#include "globox.h"
#include <stdint.h>

void globox_egl_callback_wait_copy(struct globox* globox);
void globox_egl_callback_allocate(struct globox* globox);
void globox_egl_callback_unminimize_start(struct globox* globox);
void globox_egl_callback_unminimize_finish(struct globox* globox);
void globox_egl_callback_attach(struct globox* globox);
void globox_egl_callback_resize(
	struct globox* globox,
	int32_t width,
	int32_t height);

#endif
