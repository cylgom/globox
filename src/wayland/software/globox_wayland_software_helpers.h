#ifndef H_GLOBOX_WAYLAND_SOFTWARE_HELPERS
#define H_GLOBOX_WAYLAND_SOFTWARE_HELPERS

#include "globox.h"
#include <stdint.h>

void globox_software_callback_wait_copy(struct globox* globox);
void globox_software_callback_buffer_release(void* data, struct wl_buffer* wl_buffer);
void globox_software_callback_allocate(struct globox* globox);
void globox_software_callback_unminimize_start(struct globox* globox);
void globox_software_callback_unminimize_finish(struct globox* globox);
void globox_software_callback_attach(struct globox* globox);
void globox_software_callback_resize(
	struct globox* globox,
	int32_t width,
	int32_t height);

#endif
