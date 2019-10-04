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

	printf("%s\n", ok ? "ok" : "oops");

	if (ok)
	{
#if 0
		for (int i = 0; i < (100 * 100); ++i)
		{
			usleep(100);
			ctx.rgba[i] = 0xFFFFFFFF;
			globox_commit(&ctx);
		}
#endif

		sleep(10);

		globox_close(&ctx);
	}

	return 0;
}
