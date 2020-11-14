#ifndef H_GLOBOX_PLATFORM
#define H_GLOBOX_PLATFORM

/// this file provides the platform substructure for the main globox context

// stdlib
#include <stdint.h>
#include <stdbool.h>
// include the macOS structures and functions
#include "macos/globox_macos_helpers.h"
// include the graphic context structures and functions
// this will also have the effect of exposing the graphic context functions
#if defined(GLOBOX_CONTEXT_SOFTWARE)
	#include "software/globox_macos_software.h"
#elif defined(GLOBOX_CONTEXT_VULKAN)
	#include "vulkan/globox_macos_vulkan.h"
#elif defined(GLOBOX_CONTEXT_EGL)
	#include "egl/globox_macos_egl.h"
#elif defined(GLOBOX_CONTEXT_GLX)
	#include "glx/globox_macos_glx.h"
#endif

// platform structure
struct globox_platform
{
	// event handle that can be used to get window events notifications
	// this is available on all supported platforms and can be accessed
	// with `globox_get_platform_handle()`
	id globox_platform_event_handle;
	uint32_t* globox_platform_argb;

	// graphic contexts
#if defined(GLOBOX_CONTEXT_SOFTWARE)
	struct globox_macos_software globox_macos_software;
#elif defined(GLOBOX_CONTEXT_VULKAN)
	struct globox_macos_vulkan globox_macos_vulkan;
#elif defined(GLOBOX_CONTEXT_EGL)
	struct globox_macos_egl globox_macos_egl;
#elif defined(GLOBOX_CONTEXT_GLX)
	struct globox_macos_glx globox_macos_glx;
#endif

        Class globox_macos_class_appdelegate;
        Class globox_macos_class_view;

        id globox_macos_obj_appdelegate;
        id globox_macos_obj_window;

        id globox_macos_obj_masterview;
        id globox_macos_obj_view;
        id globox_macos_obj_blur;

	// use an `int` instead of an `enum globox_state`
	// because enums forward-declarations do not exist
	int globox_macos_state_old;

	bool (*globox_macos_appdelegate_callback)(
		struct macos_appdelegate* appdelegate,
		SEL cmd,
		id msg);
};

enum globox_error
{
	// special error code to signal everything is fine
	GLOBOX_ERROR_OK = 0,

	// error codes describing the nature of the problem
	GLOBOX_ERROR_NULL,
	GLOBOX_ERROR_ALLOC,
	GLOBOX_ERROR_BOUNDS,
	GLOBOX_ERROR_DOMAIN,

	// extra high-level error codes for use by the developer
	// they can be used to throw a more generic error and
	// describe the context instead of the error itself
	GLOBOX_ERROR_FUNC_OPEN,
	GLOBOX_ERROR_FUNC_CLOSE,

	GLOBOX_ERROR_FUNC_PLATFORM_INIT,
	GLOBOX_ERROR_FUNC_PLATFORM_FREE,
	GLOBOX_ERROR_FUNC_PLATFORM_CREATE_WINDOW,
	GLOBOX_ERROR_FUNC_PLATFORM_HOOKS,
	GLOBOX_ERROR_FUNC_PLATFORM_COMMIT,
	GLOBOX_ERROR_FUNC_PLATFORM_PREPOLL,
	GLOBOX_ERROR_FUNC_PLATFORM_EVENTS_POLL,
	GLOBOX_ERROR_FUNC_PLATFORM_EVENTS_WAIT,
	GLOBOX_ERROR_FUNC_PLATFORM_EVENTS_HANDLE,
	GLOBOX_ERROR_FUNC_PLATFORM_SET_ICON,
	GLOBOX_ERROR_FUNC_PLATFORM_SET_TITLE,
	GLOBOX_ERROR_FUNC_PLATFORM_SET_STATE,

	GLOBOX_ERROR_FUNC_SOFTWARE_INIT,
	GLOBOX_ERROR_FUNC_SOFTWARE_FREE,
	GLOBOX_ERROR_FUNC_SOFTWARE_CREATE,
	GLOBOX_ERROR_FUNC_SOFTWARE_SHRINK,
	GLOBOX_ERROR_FUNC_SOFTWARE_RESERVE,
	GLOBOX_ERROR_FUNC_SOFTWARE_EXPOSE,
	GLOBOX_ERROR_FUNC_SOFTWARE_COPY,

	// platform errors
	GLOBOX_ERROR_MACOS_CONN,

	// special value used to get the total number of error codes
	GLOBOX_ERROR_SIZE,
};

#endif
