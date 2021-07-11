#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

#include "wayland/globox_wayland.h"
#include "wayland/software/globox_wayland_software_helpers.h"

void globox_context_software_init(
	struct globox* globox,
	int version_major,
	int version_minor)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_software* context = &(platform->globox_wayland_software);

	// set buffer real size
	context->globox_software_buffer_len =
		4
		* globox->globox_width
		* globox->globox_height;

	// copy mutex
	pthread_mutex_init(&(context->globox_software_copy_mutex), NULL);

	context->globox_software_buffer_listener.release =
		globox_software_callback_buffer_release;

	// set callbacks function pointers
	platform->globox_wayland_unminimize_start =
		globox_software_callback_unminimize_start;
	platform->globox_wayland_unminimize_finish =
		globox_software_callback_unminimize_finish;
	platform->globox_wayland_callback_xdg_toplevel_configure =
		globox_software_callback_resize;
	platform->globox_wayland_callback_xdg_surface_configure =
		globox_software_callback_attach;
	platform->globox_wayland_callback_wait_copy =
		globox_software_callback_wait_copy;

	return;
}

void globox_context_software_create(struct globox* globox)
{
	int error;
	struct globox_platform* platform = globox->globox_platform;

	// create shm, allocate buffer
	globox_software_callback_allocate(globox);

	if (globox_error_catch(globox))
	{
		return;
	}

	// commit and dispatch
	globox_platform_commit(globox);

	error =
		wl_display_roundtrip(
			platform->globox_wayland_display);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_ROUNDTRIP);
	}

	return;
}

void globox_context_software_shrink(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_software* context = &(platform->globox_wayland_software);
	int error;

	wl_shm_pool_destroy(context->globox_software_pool);

	close(context->globox_software_fd);

	error =
		munmap(
			platform->globox_platform_argb,
			context->globox_software_buffer_len);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_MUNMAP);
	}

	context->globox_software_buffer_len =
		4
		* globox->globox_width
		* globox->globox_height;

	globox_software_callback_allocate(globox);

	if (globox_error_catch(globox))
	{
		return;
	}

	return;
}

void globox_context_software_free(struct globox* globox)
{
	int error;
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_software* context = &(platform->globox_wayland_software);

	wl_shm_pool_destroy(context->globox_software_pool);

	close(context->globox_software_fd);

	error =
		munmap(
			platform->globox_platform_argb,
			context->globox_software_buffer_len);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_MUNMAP);
	}

	wl_buffer_destroy(context->globox_software_buffer);

	return;
}

void globox_context_software_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_software* context = &(platform->globox_wayland_software);

	wl_surface_attach(
		platform->globox_wayland_surface,
		context->globox_software_buffer,
		0,
		0);

	wl_surface_damage_buffer(
		platform->globox_wayland_surface,
		x,
		y,
		width,
		height);

	globox->globox_redraw = false;

	globox_platform_commit(globox);

	return;
}
