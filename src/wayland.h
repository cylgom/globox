#ifndef GLOBOX_WAYLAND_UTIL
#define GLOBOX_WAYLAND_UTIL

#include "globox.h"
#include "xdg-shell-client-protocol.h"
#include <wayland-client.h>

void random_id(char *str);
int create_shm_file(void);
int allocate_shm_file(size_t size);
struct wl_buffer* draw_frame(struct globox* globox);
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

#endif
