#ifndef H_GLOBOX_PLATFORM
#define H_GLOBOX_PLATFORM

/// this file provides the platform substructure for the main globox context

// stdlib
#include <stdint.h>
#include <stdbool.h>
// include the macos structures and functions
#include "macos/globox_macos_types.h"
// include the graphic context structures and functions
// this will also have the effect of exposing the graphic context functions
#if defined(GLOBOX_CONTEXT_SOFTWARE)
	#include "software/globox_macos_software.h"
#elif defined(GLOBOX_CONTEXT_VULKAN)
	#include "vulkan/globox_macos_vulkan.h"
#elif defined(GLOBOX_CONTEXT_EGL)
	#include "egl/globox_macos_egl.h"
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
#endif

	Class globox_macos_class_view;
	Class globox_macos_class_appdelegate;
	Class globox_macos_class_cursor;

	id globox_macos_obj_view;
	id globox_macos_obj_appdelegate;
	id globox_macos_obj_cursor_a;
	id globox_macos_obj_cursor_b;
	id globox_macos_obj_window;
	id globox_macos_obj_masterview;
	id globox_macos_obj_blur;

	struct macos_size globox_macos_window_min_size;
	struct macos_rect globox_macos_buttons[3];

	int16_t globox_macos_titlebar_height;
	int16_t globox_macos_interactive_x;
	int16_t globox_macos_interactive_y;

	bool globox_macos_fullscreen;
	bool globox_macos_inhibit_resize;

	struct macos_rect globox_macos_old_window_frame;
	unsigned int globox_macos_state_old;
	id* globox_macos_obj_cursor_old;
	bool globox_macos_cursor_use_a;
	long globox_macos_cursor;
	unsigned int globox_macos_cursor_hover;

	void (*globox_macos_appdelegate_context_callback)(
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
	GLOBOX_ERROR_MACOS_CLASS_GET,
	GLOBOX_ERROR_MACOS_CLASS_ALLOC,
	GLOBOX_ERROR_MACOS_CLASS_ADDVAR,
	GLOBOX_ERROR_MACOS_CLASS_ADDMETHOD,
	GLOBOX_ERROR_MACOS_OBJ_INIT,
	GLOBOX_ERROR_MACOS_OBJ_NIL,
	GLOBOX_ERROR_MACOS_EGL_FAIL,

	// special value used to get the total number of error codes
	GLOBOX_ERROR_SIZE,
};

#endif
