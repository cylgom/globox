#define _XOPEN_SOURCE 500

#include "globox.h"
#include <unistd.h>
#include <stdio.h>

int main()
{
	struct globox ctx;

	bool ok = globox_open(
		&ctx,
		GLOBOX_STATE_REGULAR,
		"test",
		0,
		0,
		100,
		100);

	if (ok)
	{
		globox_set_title(&ctx, "HELO");
		sleep(1);
		globox_set_visible(&ctx, false);
		globox_commit(&ctx);
		sleep(1);
		globox_set_visible(&ctx, true);
		globox_commit(&ctx);

		for (int i = 0; i < (100 * 100); ++i)
		{
			usleep(1000);
			ctx.rgba[i] = 0xFFFFFF;
			globox_copy(&ctx, 10, 10, 80, 80);
			globox_commit(&ctx);
		}

		uint32_t x = 0;
		uint32_t y = 0;

		while (x < 500)
		{
			usleep(1000);
			globox_set_pos(&ctx, x, y);
			globox_commit(&ctx);
			++x;
		}

		while (y < 500)
		{
			usleep(1000);
			globox_set_pos(&ctx, x, y);
			globox_commit(&ctx);
			++y;
		}

		while (x > 0)
		{
			usleep(1000);
			globox_set_pos(&ctx, x, y);
			globox_commit(&ctx);
			--x;
		}

		while (y > 0)
		{
			usleep(1000);
			globox_set_pos(&ctx, x, y);
			globox_commit(&ctx);
			--y;
		}

		uint32_t size = 100;

		while (size < 500)
		{
			usleep(1000);
			globox_set_size(&ctx, size, size);
			globox_commit(&ctx);
			++size;
		}

		while (size > 0)
		{
			usleep(1000);
			globox_set_size(&ctx, size, size);
			globox_commit(&ctx);
			--size;
		}

		sleep(1);

		globox_close(&ctx);
	}

	return 0;
}
