#ifndef H_GLOBOX
#define H_GLOBOX

#include <stdint.h>
#include <stdbool.h>

#ifdef GLOBOX_X11
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#endif

#ifdef GLOBOX_WAYLAND
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#endif

// structures
enum globox_state
{
	GLOBOX_STATE_REGULAR,
	GLOBOX_STATE_MINIMIZED,
	GLOBOX_STATE_MAXIMIZED,
	GLOBOX_STATE_FULLSCREEN,
};

struct globox
{
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t buf_width;
	uint32_t buf_height;
	uint32_t* argb;

	char* title;
	enum globox_state state;
	bool redraw;
	int fd;

#ifdef GLOBOX_X11
	xcb_connection_t* x11_conn;
	xcb_window_t x11_win;
	int x11_screen;

	xcb_shm_segment_info_t x11_shm;
	xcb_gcontext_t x11_gfx;
	xcb_pixmap_t x11_pix;
	bool x11_socket;
	bool x11_pixmap_update;
	bool x11_visible;

	xcb_atom_t x11_atoms[6];
#endif

#ifdef GLOBOX_WAYLAND
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;

    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

	struct wl_buffer_listener wl_buffer_listener;
	struct xdg_surface_listener xdg_surface_listener;
	struct xdg_wm_base_listener xdg_wm_base_listener;
	struct wl_registry_listener wl_registry_listener;
#endif
};

// window creation
bool globox_open(
	struct globox* globox,
	enum globox_state state,
	const char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_close(
	struct globox* globox);

// buffer management
bool globox_handle_events(struct globox* globox);

bool globox_shrink(struct globox* globox);

// buffer transfer
void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_commit(
	struct globox* globox);

// setters
void globox_set_icon(struct globox* globox, uint32_t* bgra, uint32_t len);
void globox_set_title(struct globox* globox, const char* title);
void globox_set_state(struct globox* globox, enum globox_state state);
void globox_set_pos(struct globox* globox, uint32_t x, uint32_t y);
bool globox_set_size(struct globox* globox, uint32_t width, uint32_t height);

// getters
char* globox_get_title(struct globox* globox);
enum globox_state globox_get_state(struct globox* globox);
void globox_get_pos(struct globox* globox, int32_t* x, int32_t* y);
void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height);

#endif
