#define _XOPEN_SOURCE 700

#include "globox.h"
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define MAX_EVENTS 1000

#if 0
extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;
#endif

// global context because I'm lazy
struct globox ctx = {0};

#if 0
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
#endif

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
		false);

	if (ok)
	{
		// basic window config
		globox_set_title(&ctx, "HELO");
#if 0
		globox_set_icon(
			&ctx,
			(uint32_t*) &iconpix_beg,
			2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
		globox_commit(&ctx);

		// event polling initialization
		int fd = epoll_create(1);

		struct epoll_event ev =
		{
			EPOLLIN | EPOLLET,
			{0},
		};

		epoll_ctl(
			fd,
			EPOLL_CTL_ADD,
			ctx.fd.descriptor,
			&ev);

		// loop
		struct epoll_event list[MAX_EVENTS];

		while (1)
		{
			globox_prepoll(&ctx);
			epoll_wait(fd, list, MAX_EVENTS, -1);
			handler(0);

			if (ctx.closed)
			{
				break;
			}
		}

		globox_close(&ctx);
#endif
	}

	return 0;
}
