#ifndef H_GLOBOX_WAYLAND_SOFTWARE
#define H_GLOBOX_WAYLAND_SOFTWARE

#include <stdint.h>
#include <pthread.h>
#include <wayland-client.h>

struct globox_wayland_software
{
	int globox_software_fd;
	struct wl_shm_pool* globox_software_pool;
	struct wl_buffer* globox_software_buffer;
	uint64_t globox_software_buffer_len;

	pthread_mutex_t globox_software_copy_mutex;
	struct wl_buffer_listener globox_software_buffer_listener;
};

#endif
