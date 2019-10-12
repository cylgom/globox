#define _XOPEN_SOURCE 500

#include "globox.h"
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

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
		globox_set_icon(&ctx, (uint32_t*) &iconpix_beg);
		sleep(1);
		globox_set_state(&ctx, GLOBOX_STATE_MINIMIZED);
		globox_commit(&ctx);
		sleep(1);
		globox_set_state(&ctx, GLOBOX_STATE_REGULAR);
		globox_commit(&ctx);

		for (int i = 0; i < (100 * 100); ++i)
		{
			usleep(1000);
			ctx.rgba[i] = 0xFFFFFFFF;
			globox_copy(&ctx, 10, 10, 80, 80);
			globox_commit(&ctx);
		}

		sleep(1);

		globox_set_state(&ctx, GLOBOX_STATE_MAXIMIZED);
		globox_commit(&ctx);

		sleep(2);

		globox_set_state(&ctx, GLOBOX_STATE_FULLSCREEN);
		globox_commit(&ctx);

		sleep(2);

		globox_set_state(&ctx, GLOBOX_STATE_REGULAR);
		globox_commit(&ctx);

		sleep(1);

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
			++size;
			globox_set_size(&ctx, size, size);
			globox_commit(&ctx);
			usleep(1000);
		}

		int pos;

		for (int i = 0; i < (100 * 100); ++i)
		{
			pos = ((i / 100) * ctx.width) + (i % 100);
			ctx.rgba[pos] = 0xFFFFFFFF;
		}

		globox_copy(&ctx, 10, 10, 80, 80);
		globox_commit(&ctx);

		sleep(1);

		globox_shrink(&ctx);

		globox_copy(&ctx, 10, 10, 80, 80);
		globox_commit(&ctx);

		sleep(1);

		while (size > 0)
		{
			usleep(1000);
			globox_set_size(&ctx, size, size);
			globox_commit(&ctx);
			--size;
		}

		globox_set_size(&ctx, 100, 100);
		globox_commit(&ctx);

		sleep(1);

		struct timeval chrono_struct;
		uint64_t new;
		uint64_t old;

		gettimeofday(&chrono_struct, NULL);
		new = (uint64_t) chrono_struct.tv_sec;
		old = new;

		uint32_t old_width = ctx.width;
		uint32_t old_height = ctx.height;

		while ((new - old) < 10)
		{
			globox_handle_events(&ctx);

			if ((ctx.width != old_width) || (ctx.height != old_height))
			{
				// background
				for (uint32_t i = 0; i < ctx.height * ctx.width; ++i)
				{
					ctx.rgba[i] = 0x88888888;
				}

				// square
				uint32_t pos;

				for (uint32_t i = 0; i < (100*100); ++i)
				{
					pos = ((ctx.height / 2) - 50 + (i / 100)) * ctx.width
						+ (ctx.width / 2) - 50 + (i % 100);

					ctx.rgba[pos] = 0xFFFFFFFF;
				}

				globox_copy(&ctx, 0, 0, ctx.width, ctx.height);
				old_width = ctx.width;
				old_height = ctx.height;
			}

			gettimeofday(&chrono_struct, NULL);
			new = (uint64_t) chrono_struct.tv_sec;
		}

		globox_close(&ctx);
	}

	return 0;
}
