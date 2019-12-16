#define _XOPEN_SOURCE 700

#include "globox.h"
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <GL/gl.h>

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
		glClearColor(0.2, 0.4, 0.9, 1.0);
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

		// event polling initialization
		int fd = epoll_create(2);

		struct epoll_event ev =
		{
			EPOLLIN,
			{0},
		};

		// main events
		epoll_ctl(
			fd,
			EPOLL_CTL_ADD,
			ctx.fd,
			&ev);

		// frame callback timer event
		epoll_ctl(
			fd,
			EPOLL_CTL_ADD,
			ctx.fd_frame,
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
	}

	return 0;
}
