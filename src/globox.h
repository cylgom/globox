#ifndef H_GLOBOX
#define H_GLOBOX

#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
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

	uint32_t* rgba;
	uint8_t* comp;

#ifdef GLOBOX_X11
	xcb_connection_t* conn;
	xcb_window_t win;
	int screen;

	xcb_shm_segment_info_t shm;
	xcb_gcontext_t gfx;
	xcb_pixmap_t pix;
	bool socket;
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

void globox_commit(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_refresh(struct globox* globox);

#endif
