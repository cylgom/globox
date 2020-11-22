#ifndef H_GLOBOX_MACOS_EGL
#define H_GLOBOX_MACOS_EGL

#include "globox.h"
#include "macos/globox_macos.h"
// context includes
#include <EGL/egl.h>
#include "macos/globox_macos_helpers.h"

struct globox_macos_egl
{
	// egl init
	EGLDisplay globox_egl_display;
	EGLContext globox_egl_context;
	EGLSurface globox_egl_surface;

	// framebuffer info
	EGLConfig globox_egl_config;
	EGLint globox_egl_config_size;
};

#endif
