#define _POSIX_C_SOURCE 199506L

#include "globox.h"
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

// global context because I'm lazy
struct globox ctx = {0};

// fake handle used to register for SIGALRM interruption
void nothing()
{

}

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
		100,
		100);

	if (ok)
	{
		// basic window config
		globox_set_title(&ctx, "HELO");
		globox_set_icon(&ctx, (uint32_t*) &iconpix_beg);
		globox_commit(&ctx);

		// set SIGALRM fake handle
		struct sigaction sa;
		memset(&sa, 0, sizeof (sa));
		sa.sa_handler = &nothing;
		sigaction(SIGALRM, &sa, NULL);

		// prepare timer
		timer_t timer;
		timer_create(CLOCK_REALTIME, NULL, &timer);

		// 120 fps ftw
		struct itimerspec time_struct;
		time_struct.it_value.tv_sec = 0;
		time_struct.it_value.tv_nsec = 8333333;
		time_struct.it_interval.tv_sec = 0;
		time_struct.it_interval.tv_nsec = 8333333;

		// register timer
		timer_settime(timer, 0, &time_struct, NULL);

		// prepare sigset watchpoint
		int ret;
		sigset_t set;

		// actually handle SIGALRM signals
		while (1)
		{
			sigaddset(&set, SIGALRM);
			sigwait(&set, &ret);
			handler(0);
		}

		globox_close(&ctx);
	}

	return 0;
}
