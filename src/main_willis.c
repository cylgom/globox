#include "globox.h"
#include "willis.h"
#include "cursoryx.h"

#include <stdio.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

struct globox ctx = {0};
struct cursoryx cursoryx = {0};

void handler()
{
	globox_handle_events(&ctx);

	if (ctx.redraw)
	{
		// background
		for (uint32_t i = 0; i < ctx.height * ctx.width; ++i)
		{
			ctx.argb[i] = 0x00888888;
		}

		// square
		uint32_t pos;

		for (uint32_t i = 0; i < (100*100); ++i)
		{
			pos = ((ctx.height / 2) - 50 + (i / 100)) * ctx.width
				+ (ctx.width / 2) - 50 + (i % 100);

			ctx.argb[pos] = 0x00FFFFFF;
		}

		globox_copy(&ctx, 0, 0, ctx.width, ctx.height);
	}
}

void callback(
	struct willis* willis,
	enum willis_event_code event_code,
	enum willis_event_state event_state,
	void* data)
{
	printf(
		"%s\n%s\n",
		willis_event_code_names[event_code],
		willis_event_state_names[event_state]);

	if ((event_code == WILLIS_KEY_Q)
	&& (event_state == WILLIS_STATE_PRESS))
	{
		willis_mouse_grab(willis);
	}

	if ((event_code == WILLIS_KEY_W)
	&& (event_state == WILLIS_STATE_PRESS))
	{
		willis_mouse_ungrab(willis);
	}

	if (willis->utf8_string != NULL)
	{
		printf(
			"%s\n",
			willis->utf8_string);
	}

	if (event_code == WILLIS_MOUSE_MOTION)
	{
		if (willis->mouse_grab == true)
		{
			printf(
				"diff: %x %x\n",
				(uint32_t) (willis->diff_x >> 32),
				(uint32_t) (willis->diff_y >> 32));
		}
		else
		{
			printf(
				"pos: %i %i\n",
				willis->mouse_x,
				willis->mouse_y);
		}
	}

	printf("\n");
}

int main()
{
	struct willis willis;
	void* willis_backend_link;
	void* cursoryx_backend_link;

	// create window
	bool ok = globox_open(
		&ctx,
		GLOBOX_STATE_REGULAR,
		"test",
		0,
		0,
		100,
		100,
		false, // true for frame event
		willis_handle_events,
		&willis);

	if (ok)
	{
		// basic window config
		globox_set_title(&ctx, "HELO");
		globox_set_icon(
			&ctx,
			(uint32_t*) &iconpix_beg,
			2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
		globox_commit(&ctx);

		// event handling
#if defined(WILLIS_X11)
		struct willis_x11_data data =
		{
			.x11_conn = ctx.x11_conn,
			.x11_root = ctx.x11_root,
			.x11_window = ctx.x11_win,
		};

		willis_backend_link = &data;
#elif defined(WILLIS_WAYLAND)
		struct willis_wl_data data =
		{
			.wl_surface = ctx.wl_surface,
			.wl_relative_pointer = ctx.wl_relative_pointer,
			.wl_pointer_constraints = ctx.wl_pointer_constraints,
		};

		willis_backend_link = &data;
#elif defined(WILLIS_WIN)
		willis_backend_link = NULL;
#elif defined(WILLIS_QUARTZ)
		willis_backend_link = NULL;
#endif

		willis_init(
			&willis,
			willis_backend_link,
			true,
			callback,
			NULL);

		// init cursoryx
#if defined (CURSORYX_WAYLAND)
		struct cursoryx_wayland cursoryx_data =
		{
			.compositor = ctx.wl_compositor,
			.pointer = willis.wl_pointer,
			.shm = ctx.wl_shm,
		};

		cursoryx_backend_link = &cursoryx_data;
#elif defined (CURSORYX_X11)
		struct cursoryx_x11 cursoryx_data =
		{
			.conn = ctx.x11_conn,
			.window = ctx.x11_win,
			.screen = ctx.x11_screen_obj,
		};

		cursoryx_backend_link = &cursoryx_data;
#elif defined (CURSORYX_WIN)
		struct cursoryx_win cursoryx_data =
		{
			.handle = ctx.win_handle,
		};

		cursoryx_backend_link = &cursoryx_data;
#elif defined (CURSORYX_QUARTZ)
#endif
		cursoryx_start(
			&cursoryx,
			cursoryx_backend_link);

		while (!ctx.closed)
		{
			globox_prepoll(&ctx);

			// use cursoryx
			cursoryx_set(
				&cursoryx,
				CURSORYX_BUSY);

			// internal event dispatching by globox
			// `globox_poll_events` is an alternative
			if (!globox_wait_events(&ctx))
			{
				ctx.closed = true;
				break;
			}

			handler();
		}

		willis_free(&willis);

		globox_close(&ctx);
	}

	return 0;
}
