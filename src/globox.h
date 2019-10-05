#ifndef H_GLOBOX
#define H_GLOBOX

#include <stdbool.h>
#include <stdint.h>

#ifdef GLOBOX_X11
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>

#ifdef GLOBOX_EWMH
#include <xcb/xcb_ewmh.h>
#endif
#endif

// structures
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
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t* rgba;

	char* title;
	enum globox_state state;

#ifdef GLOBOX_X11
	xcb_connection_t* x11_conn;
	xcb_window_t x11_win;
	int x11_screen;

	xcb_shm_segment_info_t x11_shm;
	xcb_gcontext_t x11_gfx;
	xcb_pixmap_t x11_pix;
	bool x11_socket;
	bool x11_visible;

#ifdef GLOBOX_EWMH
	xcb_ewmh_connection_t ewmh_conn;
	xcb_atom_t ewmh_atoms[3];
#endif
#endif
};

// main window operations
bool globox_open(
	struct globox* globox,
	enum globox_state state,
	char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_close(struct globox* globox);

void globox_commit(struct globox* globox);

void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

// setters
void globox_set_title(struct globox* globox, char* title);
void globox_set_state(struct globox* globox, enum globox_state state);
void globox_set_pos(struct globox* globox, uint32_t x, uint32_t y);
void globox_set_size(struct globox* globox, uint32_t width, uint32_t height);
void globox_set_visible(struct globox* globox, bool visible);

// getters
char* globox_get_title(struct globox* globox);
enum globox_state globox_get_state(struct globox* globox);
void globox_get_pos(struct globox* globox, int32_t* x, int32_t* y);
void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height);
bool globox_get_visible(struct globox* globox);

#endif
