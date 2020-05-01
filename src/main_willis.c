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
	printf("%d %d\n", event_code, event_state);
}

int main()
{
	// create window
	bool ok = globox_open(
		&ctx,
		GLOBOX_STATE_REGULAR,
		"test",
		0,
		0,
		100,
		100,
		false); // true for frame event

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
		struct willis willis;
		union willis_display_system union_fd;
		union_fd.descriptor = ctx.fd.descriptor;

		willis_init(
			&willis,
			union_fd,
			callback,
			NULL);

		globox_enable_input(&ctx,
			willis_handle_events,
			&willis);

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

		globox_close(&ctx);
	}

	return 0;
}
