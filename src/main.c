#define _XOPEN_SOURCE 700

#include "globox.h"

#if defined(GLOBOX_X11) || defined(GLOBOX_WAYLAND)
	#include <sys/epoll.h>
	#define MAX_EVENTS 1000
#elif defined(GLOBOX_QUARTZ)
	#include "quartz_helpers.h"
#endif

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
		globox_set_icon(
			&ctx,
			(uint32_t*) &iconpix_beg,
			2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));
		globox_commit(&ctx);

		// initializes epoll for x11 event handling
#if defined(GLOBOX_X11) || defined(GLOBOX_WAYLAND)
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

		struct epoll_event list[MAX_EVENTS];
#endif

		while (!ctx.closed)
		{
			globox_prepoll(&ctx);

			// directly handles the display system's events
#if defined(GLOBOX_X11) || defined(GLOBOX_WAYLAND)
			epoll_wait(fd, list, MAX_EVENTS, -1);
#elif defined(GLOBOX_WIN)
			if (!GetMessage(&ctx.win_msg, ctx.fd.handle, 0, 0))
			{
				ctx.closed = true;
				break;
			}
#elif defined(GLOBOX_QUARTZ)
			id* future = quartz_msg_date(
				(id) objc_getClass("NSDate"),
				sel_getUid("distantFuture"));

			quartz_msg_poll(
				ctx.fd.app,
				sel_getUid("nextEventMatchingMask:untilDate:inMode:dequeue:"),
				NSEventMaskAny,
				future,
				NSDefaultRunLoopMode,
				false);
#endif

			handler();
		}

		globox_close(&ctx);
	}

	return 0;
}
