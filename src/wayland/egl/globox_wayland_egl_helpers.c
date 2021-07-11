#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"

#include <EGL/egl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wayland-egl.h>

#include "wayland/globox_wayland.h"
#include "wayland/egl/globox_wayland_egl.h"
#include "wayland/egl/globox_wayland_egl_helpers.h"

void globox_egl_callback_wait_copy(struct globox* globox)
{
	// not needed
}

void globox_egl_callback_allocate(struct globox* globox)
{
	// not needed
}

void globox_egl_callback_unminimize_start(struct globox* globox)
{
	// not needed
}

void globox_egl_callback_unminimize_finish(struct globox* globox)
{
	globox_context_egl_init(
		globox,
		0,
		0);

	return;
}

void globox_egl_callback_attach(struct globox* globox)
{
	// not needed
}

void globox_egl_callback_resize(
	struct globox* globox,
	int32_t width,
	int32_t height)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_egl* context = &(platform->globox_wayland_egl);

	wl_egl_window_resize(
		context->globox_egl_window,
		width,
		height,
		0,
		0);
}
