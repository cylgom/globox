#ifndef H_GLOBOX_X11_UTIL
#define H_GLOBOX_X11_UTIL

#include "globox.h"
#include <xcb/xcb.h>
#include "x11_helpers.h"

bool init_atoms(struct globox* globox);
xcb_screen_t* get_screen(struct globox* globox);
void create_window(struct globox* globox, xcb_screen_t* screen);
void create_gfx(struct globox* globox, xcb_screen_t* screen);
void create_glx(struct globox* globox);
void create_vlk(struct globox* globox);
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
void set_frame_timer(struct globox* globox);

#endif
