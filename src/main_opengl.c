#include "globox.h"
#include <GL/gl.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

struct globox ctx = {0};

void handler()
{
	globox_handle_events(&ctx);

	if (ctx.redraw)
	{
		glClearColor(0.2f, 0.4f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		globox_copy(&ctx, 0, 0, ctx.width, ctx.height);
	}
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
		true);

	if (ok)
	{
		// basic window config
		globox_set_title(&ctx, "HELO");
		globox_set_icon(
			&ctx,
			(uint32_t*) &iconpix_beg,
			2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
		globox_commit(&ctx);

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
