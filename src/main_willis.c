#include "globox.h"
#include "willis.h"

#include <stdio.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

struct globox ctx = {0};

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

	if (willis->utf8_string != NULL)
	{
		printf(
			"%s\n",
			willis->utf8_string);
	}

	if (event_code == WILLIS_MOUSE_MOTION)
	{
		printf(
			"%i %i\n",
			willis->mouse_x,
			willis->mouse_y);
	}

	printf("\n");
}

int main()
{
	struct willis willis;
	void* willis_backend_link;

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
		willis_backend_link = ctx.x11_conn;
#elif defined(WILLIS_WAYLAND)
		willis_backend_link = NULL;
#elif defined(WILLIS_WIN)
		willis_backend_link = NULL;
#elif defined(WILLIS_QUARTZ)
#endif

		willis_init(
			&willis,
			willis_backend_link,
			true,
			callback,
			NULL);

		while (!ctx.closed)
		{
			globox_prepoll(&ctx);

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
