#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"

#include <EGL/egl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>
#include <wayland-egl.h>

#include "wayland/globox_wayland.h"
#include "wayland/egl/globox_wayland_egl.h"
#include "wayland/egl/globox_wayland_egl_helpers.h"

void globox_context_egl_init(
	struct globox* globox,
	int version_major,
	int version_minor)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_egl* context = &(platform->globox_wayland_egl);

	// set callbacks function pointers
	platform->globox_wayland_unminimize_start =
		globox_egl_callback_unminimize_start;
	platform->globox_wayland_unminimize_finish =
		globox_egl_callback_unminimize_finish;
	platform->globox_wayland_callback_xdg_toplevel_configure =
		globox_egl_callback_resize;
	platform->globox_wayland_callback_xdg_surface_configure =
		globox_egl_callback_attach;
	platform->globox_wayland_callback_wait_copy =
		globox_egl_callback_wait_copy;

	// egl
	context->globox_egl_display = EGL_NO_DISPLAY;
	context->globox_egl_context = EGL_NO_CONTEXT;
	context->globox_egl_surface = EGL_NO_SURFACE;

	context->globox_egl_display =
		eglGetDisplay(
			(EGLNativeDisplayType) platform->globox_wayland_display);

	if (context->globox_egl_display == EGL_NO_DISPLAY)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	EGLBoolean status_egl;
	EGLint display_version_major;
	EGLint display_version_minor;

	status_egl =
		eglInitialize(
			context->globox_egl_display,
			&display_version_major,
			&display_version_minor);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	status_egl =
		eglBindAPI(
			EGL_OPENGL_API);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	EGLint egl_config_attrib[] =
	{
		 EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		 EGL_RED_SIZE, 8,
		 EGL_GREEN_SIZE, 8,
		 EGL_BLUE_SIZE, 8,
		 EGL_ALPHA_SIZE, 8,
		 EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		 EGL_NONE,
	};

	status_egl =
		eglChooseConfig(
			context->globox_egl_display,
			egl_config_attrib,
			&(context->globox_egl_config),
			1,
			&(context->globox_egl_config_size));

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	EGLint egl_context_attrib[] =
	{
		EGL_CONTEXT_MAJOR_VERSION, version_major,
		EGL_CONTEXT_MINOR_VERSION, version_minor,
		EGL_NONE,
	};

	context->globox_egl_context =
		eglCreateContext(
			context->globox_egl_display,
			context->globox_egl_config,
			EGL_NO_CONTEXT,
			egl_context_attrib);
	
	if (context->globox_egl_context == EGL_NO_CONTEXT)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	return;
}

void globox_context_egl_create(struct globox* globox)
{
	int error;
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_egl* context = &(platform->globox_wayland_egl);

	context->globox_egl_window =
		wl_egl_window_create(
			platform->globox_wayland_surface,
			globox->globox_width,
			globox->globox_height);

	if (context->globox_egl_window == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	context->globox_egl_surface =
		eglCreateWindowSurface(
			context->globox_egl_display,
			context->globox_egl_config,
			(EGLNativeWindowType) context->globox_egl_window,
			NULL);

	if (context->globox_egl_surface == EGL_NO_SURFACE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	// commit and roundtrip
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

	// egl set-up
	eglMakeCurrent(
		context->globox_egl_display,
		context->globox_egl_surface,
		context->globox_egl_surface,
		context->globox_egl_context);

	eglSwapInterval(
		context->globox_egl_display,
		0);

	return;
}

void globox_context_egl_shrink(struct globox* globox)
{
	// not needed
}

void globox_context_egl_free(struct globox* globox)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_egl* context = &(platform->globox_wayland_egl);

	EGLBoolean status_egl;

	status_egl =
		eglDestroySurface(
			context->globox_egl_display,
			context->globox_egl_surface);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	status_egl =
		eglDestroyContext(
			context->globox_egl_display,
			context->globox_egl_context);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	status_egl =
		eglTerminate(
			context->globox_egl_display);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	free(context->globox_egl_window);

	free(context->globox_egl_config);

	return;
}

void globox_context_egl_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	struct globox_platform* platform = globox->globox_platform;
	struct globox_wayland_egl* context = &(platform->globox_wayland_egl);

	EGLBoolean status_egl =
		eglSwapBuffers(
			context->globox_egl_display,
			context->globox_egl_surface);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_EGL_FAIL);
		return;
	}

	globox->globox_redraw = false;

	globox_platform_commit(globox);

	return;
}
