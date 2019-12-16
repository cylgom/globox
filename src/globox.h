#ifndef H_GLOBOX
#define H_GLOBOX

#include <stdint.h>
#include <stdbool.h>

#ifdef GLOBOX_X11
	#ifdef GLOBOX_RENDER_SWR
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
	#endif

	#ifdef GLOBOX_RENDER_OGL
#include <GL/glx.h>
	#endif

#include <xcb/xcb.h>
#include "x11_helpers.h"
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
	// these only represent the initial window position
	int32_t init_x;
	int32_t init_y;
	// window size
	uint32_t width;
	uint32_t height;
	// image buffer *maximum* size
	uint32_t buf_width;
	uint32_t buf_height;
	uint32_t* argb;

	char* title;
	enum globox_state state;

	bool redraw;
	int fd;

	bool frame_event;
	bool closed;
	int fd_frame;

#ifdef GLOBOX_X11
	xcb_connection_t* x11_conn;
	xcb_atom_t x11_atoms[ATOM_COUNT];
	xcb_window_t x11_win;
	int x11_screen;

	#ifdef GLOBOX_RENDER_OGL
	Display *xlib_display;
	int xlib_screen;
	GLXFBConfig xlib_fb_config;
	GLXContext xlib_context;
	GLXWindow xlib_glx;
	#endif

	#ifdef GLOBOX_RENDER_SWR
	xcb_shm_segment_info_t x11_shm;
	xcb_gcontext_t x11_gfx;
	xcb_pixmap_t x11_pix;
	bool x11_socket;
	bool x11_pixmap_update;
	#endif
#endif

#ifdef GLOBOX_WAYLAND
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;

    struct wl_surface *wl_surface;
	struct wl_callback *wl_frame_callback;
	struct wl_callback_listener wl_surface_frame_listener;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
	struct xdg_toplevel_listener xdg_toplevel_listener;
	struct wl_output* wl_output;
	struct wl_output_listener wl_output_listener;

	int wl_buffer_fd;
	struct wl_shm_pool *wl_pool;
	struct wl_buffer *wl_buffer;

	uint32_t wl_screen_width;
	uint32_t wl_screen_height;

	struct wl_buffer_listener wl_buffer_listener;
	struct xdg_surface_listener xdg_surface_listener;
	struct xdg_wm_base_listener xdg_wm_base_listener;
	struct wl_registry_listener wl_registry_listener;

	uint32_t* wl_icon;
	uint32_t wl_icon_len;
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
	uint32_t height,
	bool frame_event);

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

void globox_prepoll(
	struct globox* globox);

// setters
void globox_set_icon(struct globox* globox, uint32_t* bgra, uint32_t len);
void globox_set_title(struct globox* globox, const char* title);
void globox_set_state(struct globox* globox, enum globox_state state);

// getters
char* globox_get_title(struct globox* globox);
enum globox_state globox_get_state(struct globox* globox);
void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height);

#endif
