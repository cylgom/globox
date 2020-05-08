#define _XOPEN_SOURCE 700
#ifdef GLOBOX_WAYLAND

#include "wayland.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "globox.h"

inline bool surface_init(struct globox* globox)
{
	// surface init
	bool err;

	globox->wl_surface = wl_compositor_create_surface(globox->wl_compositor);
	globox->xdg_surface = xdg_wm_base_get_xdg_surface(
		globox->xdg_wm_base,
		globox->wl_surface);
	err = allocate_buffer(globox);

	if (!err)
	{
		return false;
	}

	xdg_surface_add_listener(
		globox->xdg_surface,
		&(globox->xdg_surface_listener),
		globox);

	globox->xdg_toplevel = xdg_surface_get_toplevel(globox->xdg_surface);

	wl_surface_commit(globox->wl_surface);

	// register surface events
	globox->wl_frame_callback = wl_surface_frame(globox->wl_surface);
	wl_callback_add_listener(
		globox->wl_frame_callback,
		&(globox->wl_surface_frame_listener),
		globox);

	wl_display_dispatch(globox->wl_display);
	globox_copy(globox, 0, 0, globox->width, globox->height);

	// register window events (resize, close, etc)
	xdg_toplevel_add_listener(
		globox->xdg_toplevel,
		&(globox->xdg_toplevel_listener),
		globox);

	return true;
}

inline void random_id(char *str)
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    uint64_t random = time.tv_nsec;

    for (int i = 0; i < 6; ++i)
	{
        str[i] = 'A' + (random&15) + ((random&16) * 2);
        random >>= 5;
    }
}

int create_shm_file(void)
{
    uint8_t retries = 100;

    do
	{
        char name[] = "/wl_shm-XXXXXX";
        random_id(name + (sizeof (name)) - 7);

        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        --retries;

        if (fd >= 0)
		{
            shm_unlink(name);

            return fd;
        }
    }
	while ((retries > 0) && (errno == EEXIST));

    return -1;
}

int allocate_shm_file(size_t size)
{
	// create file descriptor
    int fd = create_shm_file();

    if (fd < 0)
	{
        return -1;
	}

	// truncate
    int ret;

    do
	{
        ret = ftruncate(fd, size);
    }
	while ((ret < 0) && (errno == EINTR));

    if (ret < 0)
	{
        close(fd);

        return -1;
    }

    return fd;
}

bool allocate_buffer(struct globox* globox)
{
    int size = globox->buf_width * globox->buf_height * 4;
    int fd = allocate_shm_file(size);

    if (fd == -1)
	{
        return false;
    }

    globox->argb = mmap(
		NULL,
		size,
        PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fd,
		0);

    if (globox->argb == MAP_FAILED)
	{
        close(fd);

        return NULL;
    }

	globox->wl_buffer_fd = fd;

    globox->wl_pool = wl_shm_create_pool(
		globox->wl_shm,
		fd,
		size);

    globox->wl_buffer = wl_shm_pool_create_buffer(
		globox->wl_pool,
		0,
		globox->width,
		globox->height,
		globox->width * 4,
		WL_SHM_FORMAT_XRGB8888);

	return true;
}

// callbacks
void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
	wl_buffer_destroy(wl_buffer);
}

void xdg_surface_configure(
	void *data,
	struct xdg_surface *xdg_surface,
	uint32_t serial)
{
    struct globox* globox = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_attach(globox->wl_surface, globox->wl_buffer, 0, 0);
}

void xdg_wm_base_ping(
	void *data,
	struct xdg_wm_base *xdg_wm_base,
	uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void registry_global(
	void *data,
	struct wl_registry *wl_registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
    struct globox* globox = data;

    if (strcmp(interface, wl_shm_interface.name) == 0)
	{
        globox->wl_shm = wl_registry_bind(
			wl_registry,
			name,
			&wl_shm_interface,
			1);
    }
	else if (strcmp(interface, wl_compositor_interface.name) == 0)
	{
        globox->wl_compositor = wl_registry_bind(
			wl_registry,
			name,
			&wl_compositor_interface,
			4);
    }
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
	{
        globox->xdg_wm_base = wl_registry_bind(
			wl_registry,
			name,
			&xdg_wm_base_interface,
			1);

        xdg_wm_base_add_listener(
			globox->xdg_wm_base,
			&(globox->xdg_wm_base_listener),
			globox);
    }
	else if (strcmp(interface, wl_output_interface.name) == 0)
	{
		globox->wl_output = wl_registry_bind(
			wl_registry,
			name,
			&wl_output_interface,
			1);

		wl_output_add_listener(
			globox->wl_output,
			&(globox->wl_output_listener),
			globox);
	}
	else if ((strcmp(interface, wl_seat_interface.name) == 0)
		&& (globox->event_callback != NULL))
	{
		globox->wl_seat = wl_registry_bind(
			wl_registry,
			name,
			&wl_seat_interface,
			7); // beware updated very often

		// While `wl_registry_bind` and `wl_seat_add_listener` are very nicely
		// separated in libwayland-client it seems the protocol does not handle
		// calling the latter outside of the global registry callback.
		// This is why we somehow end up with worse an integration than XCB's,
		// and wonder what happened in the *seven* revisions of this interface.
		globox->event_callback(globox->wl_seat, globox->event_callback_data);
	}
}

void registry_global_remove(
	void *data,
	struct wl_registry *wl_registry,
	uint32_t name)
{

}

void wl_surface_frame_done(
	void *data,
	struct wl_callback *frame_callback,
	uint32_t time)
{
	static const uint64_t one = 1;
	struct globox* globox = data;

	wl_callback_destroy(frame_callback);
	globox->wl_frame_callback = wl_surface_frame(globox->wl_surface);
	wl_callback_add_listener(
		globox->wl_frame_callback,
		&(globox->wl_surface_frame_listener),
		globox);

	write(globox->fd_frame, &one, 8);
	fsync(globox->fd_frame);
}

void xdg_toplevel_configure(
	void *data,
	struct xdg_toplevel *xdg_toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states)
{
	struct globox* globox = data;

	if ((width == 0) || (height == 0))
	{
		return;
	}

	globox->width = width;
	globox->height = height;
	globox->redraw = true;

	if ((globox->buf_width * globox->buf_height) < (uint32_t) (width * height))
	{
		wl_shm_pool_destroy(globox->wl_pool);
		close(globox->wl_buffer_fd);
		munmap(globox->argb, globox->buf_width * globox->buf_height * 4);
		wl_buffer_destroy(globox->wl_buffer);

		globox->buf_width = (1 + (width / globox->wl_screen_width))
			* globox->wl_screen_width;
		globox->buf_height = (1 + (height / globox->wl_screen_height))
			* globox->wl_screen_height;

		allocate_buffer(globox);
	}
	else
	{
		wl_buffer_destroy(globox->wl_buffer);
		globox->wl_buffer = wl_shm_pool_create_buffer(
			globox->wl_pool,
			0,
			globox->width,
			globox->height,
			globox->width * 4,
			WL_SHM_FORMAT_XRGB8888);
	}
}

void xdg_toplevel_close(
	void *data,
	struct xdg_toplevel *xdg_toplevel)
{
	struct globox* globox = data;
	globox->closed = true;
}

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
	int32_t output_transform)
{

}

void wl_output_mode(
	void *data,
	struct wl_output *wl_output,
	uint32_t flags,
	int32_t width,
	int32_t height,
	int32_t refresh)
{
	struct globox* globox = data;

	if (flags & WL_OUTPUT_MODE_CURRENT)
	{
		globox->wl_screen_width = width;
		globox->wl_screen_height = height;
	}
}

void wl_output_done(void *data, struct wl_output *wl_output)
{

}

void wl_output_scale(void *data, struct wl_output *wl_output, int32_t scale)
{

}

#endif
