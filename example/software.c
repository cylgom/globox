#include "globox.h"
#include <stddef.h>

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

void render(struct globox* globox)
{
	if (globox_get_redraw(globox) == true)
	{
		globox_platform_events_handle(
			globox);

		if (globox_error_catch(globox))
		{
			return;
		}

		uint32_t width = globox_get_width(globox);
		uint32_t height = globox_get_height(globox);
		uint32_t* argb = globox_platform_get_argb(globox);

		for (uint32_t i = 0; i < height * width; ++i)
		{
			argb[i] = 0x22000000;
		}

		uint32_t pos;

		unsigned int square_width = 100;
		unsigned int square_height = 100;

		if (width < square_width)
		{
			square_width = width;
		}

		if (height < square_height)
		{
			square_height = height;
		}

		for (uint32_t i = 0; i < (square_width * square_height); ++i)
		{
			pos =
				((height - square_height) / 2
				+ (i / square_width)) * width
				+ (width - square_width) / 2
				+ (i % square_width);

			argb[pos] = 0xFFFFFFFF;
		}

		globox_context_software_copy(
			globox,
			0,
			0,
			width,
			height);
	}
}

int main(void)
{
	struct globox globox = {0};

	globox_open(
		&globox,
		0,
		0,
		500,
		500,
		"globox",
		GLOBOX_STATE_REGULAR,
		NULL,
		NULL);

	if (globox_error_catch(&globox))
	{
		return 1;
	}

	globox_platform_init(&globox, true, false, true);

	if (globox_error_catch(&globox))
	{
		globox_close(&globox);
		return 1;
	}

	globox_context_software_init(&globox, 0, 0);

	if (globox_error_catch(&globox))
	{
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_platform_create_window(&globox);

	if (globox_error_catch(&globox))
	{
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_context_software_create(&globox);

	if (globox_error_catch(&globox))
	{
		globox_context_software_free(&globox);
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_platform_hooks(&globox);

	if (globox_error_catch(&globox))
	{
		globox_context_software_free(&globox);
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_platform_set_icon(
		&globox,
		(uint32_t*) &iconpix_beg,
		2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));

	globox_platform_commit(&globox);

	render(&globox);

	globox_platform_prepoll(&globox);

	if (globox_error_catch(&globox))
	{
		globox_context_software_free(&globox);
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	while (globox_get_closed(&globox) == false)
	{
		globox_platform_events_poll(&globox);

		if (globox_error_catch(&globox))
		{
			globox_context_software_free(&globox);
			globox_platform_free(&globox);
			globox_close(&globox);
			return 1;
		}

		render(&globox);

		if (globox_error_catch(&globox))
		{
			globox_context_software_free(&globox);
			globox_platform_free(&globox);
			globox_close(&globox);
			return 1;
		}
	}

	globox_context_software_free(&globox);
	globox_platform_free(&globox);
	globox_close(&globox);

	return 0;
}
