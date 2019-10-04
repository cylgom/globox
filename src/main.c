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
		for (int i = 0; i < (100 * 100); ++i)
		{
			usleep(1000);
			ctx.rgba[i] = 0xFFFFFF;
			globox_commit(&ctx);
		}

		sleep(5);

		globox_close(&ctx);
	}

	return 0;
}
