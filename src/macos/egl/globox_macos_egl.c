#include "globox.h"
#include "globox_error.h"
#include "macos/globox_macos.h"
#include <EGL/egl.h>
// macos includes
#include "macos/globox_macos_helpers.h"

bool appdelegate_init_software(
	struct macos_appdelegate* appdelegate,
	SEL cmd,
	id msg)
{
	// get the globox context
	void* out;

	object_getInstanceVariable(
		(id) appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	// declare a view frame
	struct macos_rect frame =
	{
		.origin.x = 0,
		.origin.y = 0,
		.size.width = globox->globox_width,
		.size.height = globox->globox_height,
	};

	// initialize the view object with the view frame
	platform->globox_macos_obj_masterview =
		macos_msg_id_rect(
			platform->globox_macos_obj_masterview,
			sel_getUid("initWithFrame:"),
			frame);

	platform->globox_macos_obj_view =
		macos_msg_id_rect(
			platform->globox_macos_obj_view,
			sel_getUid("initWithFrame:"),
			frame);

	macos_msg_void_bool(
		platform->globox_macos_obj_view,
		sel_getUid("setWantsLayer:"),
		true);

	platform->globox_macos_layer =
		macos_msg_id_none(
			platform->globox_macos_obj_view,
			sel_getUid("layer"));

	platform->globox_macos_obj_blur =
		macos_msg_id_rect(
			platform->globox_macos_obj_blur,
			sel_getUid("initWithFrame:"),
			frame);

	return true;
}


void globox_context_egl_init(
	struct globox* globox,
	int version_major,
	int version_minor)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_egl* context = &(platform->globox_macos_egl);

	platform->globox_macos_appdelegate_callback = appdelegate_init_software;

	// get display
	context->globox_egl_display =
		eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (context->globox_egl_display == EGL_NO_DISPLAY)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	// init
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
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	// use OpenGL
	status_egl =
		eglBindAPI(EGL_OPENGL_ES_API);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	// use 8-bit RGBA
	EGLint egl_config_attrib[] =
	{
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
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
			GLOBOX_ERROR_MACOS_EGL_FAIL);
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
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	// get visual id from EGL
	EGLint visual_id;

	status_egl =
		eglGetConfigAttrib(
			context->globox_egl_display,
			context->globox_egl_config,
			EGL_NATIVE_VISUAL_ID,
			&visual_id);

	// get visual depth from EGL
	EGLint visual_depth;

	status_egl =
		eglGetConfigAttrib(
			context->globox_egl_display,
			context->globox_egl_config,
			EGL_DEPTH_SIZE,
			&visual_depth);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	return;
}

void globox_context_egl_create(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_egl* context = &(platform->globox_macos_egl);

	// create EGL surface
#if 0
	platform->globox_macos_layer =
		macos_msg_id_none(
			(id) objc_getClass("CALayer"),
			sel_getUid("new"));

	macos_msg_void_id(
		platform->globox_macos_obj_view,
		sel_getUid("setLayer:"),
		platform->globox_macos_layer);
#endif

	context->globox_egl_surface =
		eglCreateWindowSurface(
			context->globox_egl_display,
			context->globox_egl_config,
			(EGLNativeWindowType) platform->globox_macos_layer,
			NULL);

	if (context->globox_egl_surface == EGL_NO_SURFACE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	// bind context to surface
	EGLBoolean status_egl;

	status_egl =
		eglMakeCurrent(
			context->globox_egl_display,
			context->globox_egl_surface,
			context->globox_egl_surface,
			context->globox_egl_context);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	// set swap interval
	status_egl =
		eglSwapInterval(
			context->globox_egl_display,
			0);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}
}

void globox_context_egl_shrink(struct globox* globox)
{

}

void globox_context_egl_free(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_egl* context = &(platform->globox_macos_egl);

	EGLBoolean status_egl;

	status_egl =
		eglDestroySurface(
			context->globox_egl_display,
			context->globox_egl_surface);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
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
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	status_egl =
		eglTerminate(
			context->globox_egl_display);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}
}

void globox_context_egl_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_egl* context = &(platform->globox_macos_egl);

	EGLBoolean status_egl =
		eglSwapBuffers(
			context->globox_egl_display,
			context->globox_egl_surface);

	if (status_egl == EGL_FALSE)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_MACOS_EGL_FAIL);
		return;
	}

	globox->globox_redraw = false;

	macos_msg_void_none(
		platform->globox_macos_layer,
		sel_getUid("setNeedsDisplay"));

	globox_platform_commit(globox);
}

// getters

EGLDisplay globox_egl_get_display(struct globox* globox)
{
	return globox->globox_platform.globox_macos_egl.globox_egl_display;
}

EGLContext globox_egl_get_context(struct globox* globox)
{
	return globox->globox_platform.globox_macos_egl.globox_egl_context;
}

EGLSurface globox_egl_get_surface(struct globox* globox)
{
	return globox->globox_platform.globox_macos_egl.globox_egl_surface;
}

EGLConfig globox_egl_get_config(struct globox* globox)
{
	return globox->globox_platform.globox_macos_egl.globox_egl_config;
}

EGLint globox_egl_config_get_config_size(struct globox* globox)
{
	return globox->globox_platform.globox_macos_egl.globox_egl_config_size;
}
