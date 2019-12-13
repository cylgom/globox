#ifndef GLOBOX_WAYLAND_UTIL
#define GLOBOX_WAYLAND_UTIL

#include "globox.h"
#include "xdg-shell-client-protocol.h"
#include <wayland-client.h>

void random_id(char *str);
int create_shm_file(void);
int allocate_shm_file(size_t size);
bool allocate_buffer(struct globox* globox);
void wl_buffer_release(void *data, struct wl_buffer *wl_buffer);
void xdg_surface_configure(
	void *data,
	struct xdg_surface *xdg_surface,
	uint32_t serial);
void xdg_wm_base_ping(
	void *data,
	struct xdg_wm_base *xdg_wm_base,
	uint32_t serial);
void registry_global(
	void *data,
	struct wl_registry *wl_registry,
	uint32_t name,
	const char *interface,
	uint32_t version);
void registry_global_remove(
	void *data,
	struct wl_registry *wl_registry,
	uint32_t name);
void wl_surface_frame_done(
	void *data,
	struct wl_callback *frame_callback,
	uint32_t time);
void xdg_toplevel_configure(
	void *data,
	struct xdg_toplevel *xdg_toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states);
void xdg_toplevel_close(
	void *data,
	struct xdg_toplevel *toplevel);
void wl_output_geometry(
	void *data,
	struct wl_output *wl_output,
	int32_t x,
	int32_t y,
	int32_t physical_width,
	int32_t physical_height,
	int32_t subpixel,
	const char *make,
	const char *model,
	int32_t output_transform);
void wl_output_mode(
	void *data,
	struct wl_output *wl_output,
	uint32_t flags,
	int32_t width,
	int32_t height,
	int32_t refresh);
void wl_output_done(void *data, struct wl_output *wl_output);
void wl_output_scale(void *data, struct wl_output *wl_output, int32_t scale);

#endif
