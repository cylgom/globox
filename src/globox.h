#ifndef H_GLOBOX
#define H_GLOBOX

#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include <xcb/xcb.h>
#endif

enum globox_backend
{
	GLOBOX_BACKEND_WAYLAND,
	GLOBOX_BACKEND_X11,
};

enum globox_state
{
	GLOBOX_STATE_REGULAR,
	GLOBOX_STATE_MAXIMIZED,
	GLOBOX_STATE_FULLSCREEN,
};

struct globox
{
	enum globox_backend backend;
	enum globox_state state;
	char* title;
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;

	uint8_t* rgba;

#ifdef GLOBOX_X11
	xcb_connection_t* server_conn;
	xcb_window_t window_id;
	int preferred_screen;
#endif
};

bool globox_open(
	struct globox* globox,
	enum globox_state state,
	char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_close(struct globox* globox);

bool globox_change_title(
	struct globox* globox,
	char* title);

bool globox_change_state(
	struct globox* globox,
	enum globox_state state);

#endif
