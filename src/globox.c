/// this file implements platform-independent functions

#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"
// system includes
#include <stdlib.h>
#include <string.h>

#if defined(GLOBOX_PLATFORM_WAYLAND)
	#include "wayland/globox_wayland.h"
#elif defined(GLOBOX_PLATFORM_X11)
	#include "x11/globox_x11.h"
#elif defined(GLOBOX_PLATFORM_WINDOWS)
	#include "windows/globox_windows.h"
#elif defined(GLOBOX_PLATFORM_MACOS)
	#include "macos/globox_macos.h"
#endif

void globox_open(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height,
	const char* title,
	enum globox_state state,
	void (*callback)(
		void* event,
		void* data),
	void* callback_data)
{
	char** log = globox->globox_log;

	// in this case this *is* a valid way to initialize the string array
	log[GLOBOX_ERROR_OK] = "out-of-bounds log message";
	log[GLOBOX_ERROR_NULL] = "null pointer";
	log[GLOBOX_ERROR_ALLOC] = "failed memory allocation";
	log[GLOBOX_ERROR_BOUNDS] = "out-of-bounds index";
	log[GLOBOX_ERROR_DOMAIN] = "invalid domain";

	log[GLOBOX_ERROR_FUNC_OPEN] = "error in globox_open";
	log[GLOBOX_ERROR_FUNC_PLATFORM_INIT] = "error in globox_platform_init";

	// error system initialization
	globox_error_init(globox);

	// common variables initialization
	globox->globox_x = x;
	globox->globox_y = y;
	globox->globox_width = width;
	globox->globox_height = height;
	globox->globox_title = strdup(title); // should be freed
	globox->globox_state = state;
	globox->globox_event_callback = callback;
	globox->globox_event_callback_data = callback_data;
	globox->globox_closed = false;
	globox->globox_interactive_mode = GLOBOX_INTERACTIVE_STOP;

	globox->globox_transparent = false;
	globox->globox_frameless = false;
	globox->globox_blurred = false;
}

void globox_close(struct globox* globox)
{
	free(globox->globox_title);
}

// getters

inline int32_t globox_get_x(struct globox* globox)
{
	return globox->globox_x;
}

inline int32_t globox_get_y(struct globox* globox)
{
	return globox->globox_y;
}

inline uint32_t globox_get_width(struct globox* globox)
{
	return globox->globox_width;
}

inline uint32_t globox_get_height(struct globox* globox)
{
	return globox->globox_height;
}

inline char* globox_get_title(struct globox* globox)
{
	return globox->globox_title;
}

inline bool globox_get_closed(struct globox* globox)
{
	return globox->globox_closed;
}

inline bool globox_get_redraw(struct globox* globox)
{
	return globox->globox_redraw;
}

inline bool globox_get_transparent(struct globox* globox)
{
	return globox->globox_transparent;
}

inline bool globox_get_frameless(struct globox* globox)
{
	return globox->globox_frameless;
}

inline bool globox_get_blurred(struct globox* globox)
{
	return globox->globox_blurred;
}

inline enum globox_state globox_get_state(struct globox* globox)
{
	return globox->globox_state;
}

inline void* globox_get_event_callback_data(struct globox* globox)
{
	return globox->globox_event_callback_data;
}

inline void (*globox_get_event_callback(struct globox* globox))(void* event, void* data)
{
	return globox->globox_event_callback;
}
