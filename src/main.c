#include "globox.h"
#include "unistd.h"

int main()
{
	struct globox ctx;

	globox_open(
		&ctx,
		GLOBOX_STATE_REGULAR,
		"test",
		0,
		0,
		100,
		100);

	sleep(10);

	globox_close(&ctx);

	return 0;
}
