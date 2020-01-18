#include "globox.h"
#include <string.h>
#include <stdio.h>

#define MAX_EVENTS 1000

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

// global context because I'm lazy
struct globox ctx = {0};

// update window on SIGALRM
static inline void handler(int sig)
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

int main()
{
	// create window
	bool ok = globox_open(
		&ctx,
		GLOBOX_STATE_REGULAR,
		"test",
		0,
		0,
		200,
		200,
		false);

	if (ok)
	{
		// basic window config
		globox_set_title(&ctx, "HELO");
		globox_set_icon(
			&ctx,
			(uint32_t*) &iconpix_beg,
			2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
		globox_commit(&ctx);

		BOOL win_ok = 1;

		while (win_ok > 0)
		{
			globox_prepoll(&ctx);
			win_ok = GetMessage(&ctx.win_msg, ctx.fd.handle, 0, 0);
			handler(0);
		}

		globox_close(&ctx);
	}

	return 0;
}
