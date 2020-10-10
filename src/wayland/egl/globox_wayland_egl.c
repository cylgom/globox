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
	int error;
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_wayland_egl* context = &(platform->globox_wayland_egl);

	// extremely important for the window to work!
	globox->globox_redraw = true;

	// set buffer real size
	context->globox_egl_buffer_width = globox->globox_width;
	context->globox_egl_buffer_height = globox->globox_height;

	// set callbacks function pointers
	platform->globox_wayland_unminimize_start =
		globox_egl_callback_unminimize_start;
	platform->globox_wayland_unminimize_finish =
		globox_egl_callback_unminimize_finish;
	platform->globox_wayland_callback_xdg_toplevel_configure =
		globox_egl_callback_resize;
	platform->globox_wayland_callback_xdg_surface_configure =
		globox_egl_callback_attach;

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
		EGL_CONTEXT_CLIENT_VERSION, 2,
		//EGL_CONTEXT_MAJOR_VERSION, version_major,
		//EGL_CONTEXT_MINOR_VERSION, version_minor,
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

	// wayland surface
	platform->globox_wayland_surface =
		wl_compositor_create_surface(
			platform->globox_wayland_compositor);

	if (platform->globox_wayland_surface == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// get xdg surface
	platform->globox_wayland_xdg_surface =
		xdg_wm_base_get_xdg_surface(
			platform->globox_wayland_xdg_wm_base,
			platform->globox_wayland_surface);

	if (platform->globox_wayland_xdg_surface == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// add surface listener
	error =
		xdg_surface_add_listener(
			platform->globox_wayland_xdg_surface,
			&(platform->globox_wayland_xdg_surface_listener),
			globox);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_LISTENER);
		return;
	}

	// get toplevel surface
	platform->globox_wayland_xdg_toplevel =
		xdg_surface_get_toplevel(
			platform->globox_wayland_xdg_surface);

	if (platform->globox_wayland_xdg_toplevel == NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_REQUEST);
		return;
	}

	// add toplevel listener
	error =
		xdg_toplevel_add_listener(
			platform->globox_wayland_xdg_toplevel,
			&(platform->globox_wayland_xdg_toplevel_listener),
			globox);

	if (error == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_WAYLAND_LISTENER);

		return;
	}

#if 0
	// create shm, allocate buffer
	globox_egl_callback_allocate(globox);

	if (globox_error_catch(globox))
	{
		return;
	}
#else
	context->globox_egl_window =
		wl_egl_window_create(
			platform->globox_wayland_surface,
			context->globox_egl_buffer_width,
			context->globox_egl_buffer_height);

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
#endif

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

void globox_context_egl_create(struct globox* globox)
{
	// not needed
}

void globox_context_egl_shrink(struct globox* globox)
{
	// not needed
}

void globox_context_egl_free(struct globox* globox)
{
	// TODO

	return;
}

void globox_context_egl_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	struct globox_platform* platform = &(globox->globox_platform);
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

// getters

// TODO

EGLDisplay globox_egl_get_display(struct globox* globox)
{
	return globox->globox_platform.globox_wayland_egl.globox_egl_display;
}

EGLContext globox_egl_get_context(struct globox* globox)
{
	return globox->globox_platform.globox_wayland_egl.globox_egl_context;
}

EGLSurface globox_egl_get_surface(struct globox* globox)
{
	return globox->globox_platform.globox_wayland_egl.globox_egl_surface;
}

EGLConfig globox_egl_get_config(struct globox* globox)
{
	return globox->globox_platform.globox_wayland_egl.globox_egl_config;
}

EGLint globox_egl_config_get_config_size(struct globox* globox)
{
	return globox->globox_platform.globox_wayland_egl.globox_egl_config_size;
}