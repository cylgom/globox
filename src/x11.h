#ifndef H_GLOBOX_X11_UTIL
#define H_GLOBOX_X11_UTIL

#include "globox.h"
#include <xcb/xcb.h>

enum x11_atom_types
{
	ATOM_STATE_MAXIMIZED_HORZ = 0,
	ATOM_STATE_MAXIMIZED_VERT,
	ATOM_STATE_FULLSCREEN,
	ATOM_STATE_HIDDEN,
	ATOM_STATE,
	ATOM_ICON,
	ATOM_COUNT // used to get size
};

bool init_atoms(struct globox* globox);
xcb_screen_t* get_screen(struct globox* globox);
void create_window(struct globox* globox, xcb_screen_t* screen);
void create_gfx(struct globox* globox, xcb_screen_t* screen);
bool visual_compatible(struct globox* globox, xcb_screen_t* screen);
bool buffer_socket(struct globox* globox);
void buffer_shm(struct globox* globox);
bool globox_reserve(
	struct globox* globox,
	uint32_t width,
	uint32_t height);
void handle_title(struct globox* globox);
void handle_state(struct globox* globox);
void handle_expose(struct globox* globox, uint32_t* arr, uint8_t cur);
void set_state(
	struct globox* globox,
	xcb_atom_t atom,
	uint32_t action);

#endif
