#ifndef H_GLOBOX
#define H_GLOBOX

/// this is the main globox header and the only one the developer should include

// common includes
#include <stdint.h>
#include <stdbool.h>

// platform-specific includes
#if defined(GLOBOX_PLATFORM_WAYLAND)
	#include "wayland/globox_wayland.h"
#elif defined(GLOBOX_PLATFORM_X11)
	#include "x11/globox_x11.h"
#elif defined(GLOBOX_PLATFORM_WINDOWS)
	#include "windows/globox_windows.h"
#elif defined(GLOBOX_PLATFORM_MACOS)
	#include "macos/globox_macos.h"
	#include "macos/globox_macos_helpers.h"
#endif

#if defined(GLOBOX_CONTEXT_EGL)
	#include <EGL/egl.h>
#endif

// helper structures
enum globox_state
{
	GLOBOX_STATE_REGULAR,
	GLOBOX_STATE_MINIMIZED,
	GLOBOX_STATE_MAXIMIZED,
	GLOBOX_STATE_FULLSCREEN,
};

enum globox_interactive_mode
{
	GLOBOX_INTERACTIVE_STOP = 0,
	GLOBOX_INTERACTIVE_MOVE,
	GLOBOX_INTERACTIVE_N,  // North
	GLOBOX_INTERACTIVE_NW, // North-West
	GLOBOX_INTERACTIVE_W,  // West
	GLOBOX_INTERACTIVE_SW, // South-West
	GLOBOX_INTERACTIVE_S,  // South
	GLOBOX_INTERACTIVE_SE, // South-East
	GLOBOX_INTERACTIVE_E,  // East
	GLOBOX_INTERACTIVE_NE, // North-East
};

// globox
struct globox
{
	// window position (signed)
	int32_t globox_x; 
	int32_t globox_y;
	// window size
	uint32_t globox_width;
	uint32_t globox_height;
	// window title (UTF-8)
	char* globox_title;
	bool globox_closed;
	bool globox_redraw;
	enum globox_interactive_mode globox_interactive_mode;

	bool globox_transparent;
	bool globox_frameless;
	bool globox_blurred;

	// window state (regular, minimized, maximized, fullscreen)
	enum globox_state globox_state;
	// window events callback (resize, hover, click, type)
	void (*globox_event_callback)(
		void* event,
		void* data);
	// window events callback data (developer-provided)
	void* globox_event_callback_data;
	// error handling
	enum globox_error globox_error;
	char* globox_log[GLOBOX_ERROR_SIZE];

	// platform-specific substructure
	struct globox_platform globox_platform;
};

// cleans the error field
void globox_error_reset(struct globox* globox);
// prints the error message
void globox_error_basic_log(struct globox* globox);
// returns a pointer to the error message
char* globox_error_output_log(struct globox* globox);
// returns the error code
enum globox_error globox_error_output_code(struct globox* globox);
// returns true if an error occurred
char globox_error_catch(struct globox* globox);

// generic functions
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
	void* callback_data);

void globox_close(struct globox* globox);

// platform-dependent functions
void globox_platform_free(struct globox* globox);
void globox_platform_create_window(struct globox* globox);
void globox_platform_hooks(struct globox* globox);
void globox_platform_commit(struct globox* globox);
void globox_platform_prepoll(struct globox* globox);
void globox_platform_events_poll(struct globox* globox);
void globox_platform_events_wait(struct globox* globox);

void globox_platform_interactive_mode(
	struct globox* globox,
	enum globox_interactive_mode mode);

void globox_platform_init(
	struct globox* globox,
	bool transparent,
	bool frameless,
	bool blurred);

void globox_platform_events_handle(
	struct globox* globox);

void globox_platform_set_icon(
	struct globox* globox,
	uint32_t* pixmap,
	uint32_t len);

void globox_platform_set_title(
	struct globox* globox,
	const char* title); 

void globox_platform_set_state(
	struct globox* globox,
	enum globox_state state); 

// context-dependent functions
#if defined(GLOBOX_CONTEXT_SOFTWARE)
void globox_context_software_free(struct globox* globox);
void globox_context_software_create(struct globox* globox);
void globox_context_software_shrink(struct globox* globox);
void globox_context_software_init(
	struct globox* globox,
	int version_major,
	int version_minor);
void globox_context_software_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);
#elif defined(GLOBOX_CONTEXT_VULKAN)
#elif defined(GLOBOX_CONTEXT_EGL)
void globox_context_egl_free(struct globox* globox);
void globox_context_egl_create(struct globox* globox);
void globox_context_egl_shrink(struct globox* globox);
void globox_context_egl_init(
	struct globox* globox,
	int version_major,
	int version_minor);
void globox_context_egl_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);
#elif defined(GLOBOX_CONTEXT_GLX)
void globox_context_glx_free(struct globox* globox);
void globox_context_glx_create(struct globox* globox);
void globox_context_glx_shrink(struct globox* globox);
void globox_context_glx_init(
	struct globox* globox,
	int version_major,
	int version_minor);
void globox_context_glx_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);
#endif

// generic getters 
int32_t globox_get_x(struct globox* globox);
int32_t globox_get_y(struct globox* globox);
uint32_t globox_get_width(struct globox* globox);
uint32_t globox_get_height(struct globox* globox);
char* globox_get_title(struct globox* globox);
bool globox_get_closed(struct globox* globox);
bool globox_get_redraw(struct globox* globox);

bool globox_get_transparent(struct globox* globox);
bool globox_get_frameless(struct globox* globox);
bool globox_get_blurred(struct globox* globox);

enum globox_state globox_get_state(struct globox* globox);
void* globox_get_event_callback_data(struct globox* globox);
// the syntax reaches its limits when it's time to return a function pointer
// here is what is returned: void (*globox_event_callback)(void* event, void* data)
void (*globox_get_event_callback(struct globox* globox))(void* event, void* data);

// platform setters
#if defined(GLOBOX_PLATFORM_WAYLAND)
void globox_wayland_save_serial(struct globox* globox, uint32_t serial);
#endif

// platform getters 
uint32_t* globox_platform_get_argb(struct globox* globox);
#if defined(GLOBOX_PLATFORM_WAYLAND)
	int globox_platform_get_event_handle(struct globox* globox);
	int globox_wayland_get_epoll(struct globox* globox);
	struct epoll_event* globox_wayland_get_epoll_event(struct globox* globox);
	uint32_t* globox_wayland_get_icon(struct globox* globox);
	uint32_t globox_wayland_get_icon_len(struct globox* globox);
	uint32_t globox_wayland_get_screen_width(struct globox* globox);
	uint32_t globox_wayland_get_screen_height(struct globox* globox);
	struct wl_display* globox_wayland_get_display(struct globox* globox);
	struct wl_registry* globox_wayland_get_registry(struct globox* globox);
	struct wl_shm* globox_wayland_get_shm(struct globox* globox);
	struct wl_compositor* globox_wayland_get_compositor(struct globox* globox);
	struct wl_output* globox_wayland_get_output(struct globox* globox);
	struct wl_seat* globox_wayland_get_seat(struct globox* globox);
	struct xdg_wm_base* globox_wayland_get_xdg_wm_base(struct globox* globox);
	struct xdg_toplevel* globox_wayland_get_xdg_toplevel(struct globox* globox);
	struct xdg_surface* globox_wayland_get_xdg_surface(struct globox* globox);
	struct wl_surface* globox_wayland_get_surface(struct globox* globox);
	// listeners are not needed
	struct zxdg_decoration_manager_v1* globox_wayland_get_xdg_decoration_manager(struct globox* globox);
	struct zxdg_toplevel_decoration_v1* globox_wayland_get_xdg_decoration(struct globox* globox);
	struct org_kde_kwin_blur_manager* globox_wayland_get_kde_blur_manager(struct globox* globox);
	struct org_kde_kwin_blur* globox_wayland_get_kde_blur(struct globox* globox);
	uint32_t globox_wayland_saved_get_serial(struct globox* globox);
	#if defined(GLOBOX_CONTEXT_SOFTWARE)
		int globox_software_get_shm_fd(struct globox* globox);
		struct wl_shm_pool* globox_software_get_shm_pool(struct globox* globox);
		struct wl_buffer* globox_software_get_buffer(struct globox* globox);
	#elif defined(GLOBOX_CONTEXT_EGL)
		struct wl_egl_window* globox_egl_get_window(struct globox* globox);
	#endif
#elif defined(GLOBOX_PLATFORM_X11)
	int globox_platform_get_event_handle(struct globox* globox);
	xcb_connection_t* globox_x11_get_conn(struct globox* globox);
	xcb_atom_t* globox_x11_get_atom_list(struct globox* globox);
	xcb_window_t globox_x11_get_win(struct globox* globox);
	xcb_window_t globox_x11_get_root_win(struct globox* globox);
	int globox_x11_get_screen_id(struct globox* globox);
	xcb_screen_t* globox_x11_get_screen_obj(struct globox* globox);
	xcb_visualid_t globox_x11_get_visual_id(struct globox* globox);
	uint32_t globox_x11_get_attr_mask(struct globox* globox);
	uint32_t* globox_x11_get_attr_val(struct globox* globox);
	int globox_x11_get_epoll(struct globox* globox);
	struct epoll_event* globox_x11_get_epoll_event(struct globox* globox);
	uint32_t* globox_x11_get_expose_queue(struct globox* globox);
	#if defined(GLOBOX_CONTEXT_SOFTWARE)
		xcb_shm_segment_info_t globox_software_get_shm(struct globox* globox);
		xcb_gcontext_t globox_software_get_gfx(struct globox* globox);
		xcb_pixmap_t globox_software_get_pixmap(struct globox* globox);
		bool globox_software_get_pixmap_update(struct globox* globox);
		bool globox_software_get_shared_pixmaps(struct globox* globox);
	#elif defined(GLOBOX_CONTEXT_GLX)
		Display* globox_glx_get_display(struct globox* globox);
		GLXFBConfig globox_glx_get_fb_config(struct globox* globox);
		GLXContext globox_glx_get_context(struct globox* globox);
		GLXWindow globox_glx_get_win(struct globox* globox);
	#endif
#elif defined(GLOBOX_PLATFORM_WINDOWS)
#elif defined(GLOBOX_PLATFORM_MACOS)
	id globox_platform_get_event_handle(struct globox* globox);
#endif

// platform-independent context getters
#if defined(GLOBOX_CONTEXT_VULKAN)
#elif defined(GLOBOX_CONTEXT_SOFTWARE)
uint32_t globox_software_get_buffer_width(struct globox* globox);
uint32_t globox_software_get_buffer_height(struct globox* globox);
#elif defined(GLOBOX_CONTEXT_EGL)
EGLDisplay globox_egl_get_display(struct globox* globox);
EGLContext globox_egl_get_context(struct globox* globox);
EGLSurface globox_egl_get_surface(struct globox* globox);
EGLConfig globox_egl_get_config(struct globox* globox);
EGLint globox_egl_config_get_config_size(struct globox* globox);
#endif

#endif
