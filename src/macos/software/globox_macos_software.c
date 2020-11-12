#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"
#include "macos/globox_macos.h"
// system includes
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
// macos includes
#include "macos/globox_macos_helpers.h"

void view_drawrect_software(
	id view,
	SEL cmd,
	struct macos_rect rect)
{
	// get the globox context
	void* out;

	object_getInstanceVariable(
		(id) view,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_software* context = &(platform->globox_macos_software);

	// get contexts
	id nsgraphicscontext =
		macos_msg_id_none(
			(id) objc_getClass("NSGraphicsContext"),
			sel_getUid("currentContext"));

	id cgcontext =
		macos_msg_id_none(
			nsgraphicscontext,
			sel_getUid("CGContext"));

	// get the memory address of the raw window buffer
	// this will fail if the context used is not a bitmap!
	// in theory, the pointer obtained is only valid in drawRect
	// in practice, it should remain valid when used in the main thread
	// we strive to respect Apple's rules however, mainly to help robustness
	uint8_t* buffer = (uint8_t*) CGBitmapContextGetData(cgcontext);

	// get the remaining information about the window buffer
	uint32_t width = CGBitmapContextGetWidth(cgcontext);
	uint32_t height = CGBitmapContextGetHeight(cgcontext);
	uint32_t width_bytes_padded = CGBitmapContextGetBytesPerRow(cgcontext);

	if (platform->globox_platform_argb == NULL)
	{
		// TODO error
		return;
	}

	if ((context->globox_software_buffer_width * context->globox_software_buffer_height) < (width * height))
	{
		// realloc the buffer if needed
		free(platform->globox_platform_argb);
		platform->globox_platform_argb = (uint32_t*) malloc(4 * width * height);
		context->globox_software_buffer_width = width;
		context->globox_software_buffer_height = height;
		globox->globox_width = width;
		globox->globox_height = height;
		globox->globox_redraw = true;
	}
	else if ((globox->globox_width == width) && (globox->globox_height == height))
	{
		// copy the buffer if the size matches
		uint32_t width_bytes = width * 4;

		for (uint32_t i = 0; i < height; ++i)
		{
			memcpy(
				buffer + (i * width_bytes_padded),
				((uint8_t*) platform->globox_platform_argb) + (i * width_bytes),
				width_bytes);
		}
	}
	else
	{
		// update the size if needed and wait for the next cycle
		// until a properly sized image is rendered by the app
		globox->globox_width = width;
		globox->globox_height = height;
		globox->globox_redraw = true;
	}
}

bool appdelegate_init_software(
	struct macos_appdelegate* appdelegate,
	SEL cmd,
	id msg)
{
	// get the globox context
	void* out;

	object_getInstanceVariable(
		(id) appdelegate,
		"globox",
		&out);

	struct globox* globox = (struct globox*) out;
	struct globox_platform* platform = &(globox->globox_platform);

	// declare a view frame
	struct macos_rect frame =
	{
		.origin.x = 0,
		.origin.y = 0,
		.size.width = globox->globox_width,
		.size.height = globox->globox_height,
	};

	// initialize the view object with the view frame
	platform->globox_macos_obj_view =
		macos_msg_id_rect(
			platform->globox_macos_obj_view,
			sel_getUid("initWithFrame:"),
			frame);

	return true;
}

void globox_context_software_init(
	struct globox* globox,
	int version_major,
	int version_minor)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_software* context = &(platform->globox_macos_software);

	context->globox_software_buffer_width = globox->globox_width;
	context->globox_software_buffer_height = globox->globox_height;

	// inject the drawRect callback
	class_addMethod(
		platform->globox_macos_class_view,
		sel_getUid("drawRect:"),
		(IMP) view_drawrect_software,
		"v@:");

	// add the AppDelegate backend callback
	platform->globox_macos_appdelegate_callback = appdelegate_init_software;

	// allocate memory
	platform->globox_platform_argb =
		(uint32_t*) malloc(
			4 * globox->globox_width * globox->globox_height);

	if (platform->globox_platform_argb == NULL)
	{
		// TODO error
	}
}

void globox_context_software_create(struct globox* globox)
{
}

void globox_context_software_shrink(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_software* context = &(platform->globox_macos_software);

	context->globox_software_buffer_width = globox->globox_width;
	context->globox_software_buffer_height = globox->globox_height;
}

void globox_context_software_free(struct globox* globox)
{
	// TODO remove from the drawRect method from the view class
}

void globox_context_software_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	globox->globox_redraw = false;

	globox_platform_commit(globox);
}

// getters
uint32_t globox_software_get_buffer_width(struct globox* globox)
{
	return globox->globox_platform.globox_macos_software.globox_software_buffer_width;
}

uint32_t globox_software_get_buffer_height(struct globox* globox)
{
	return globox->globox_platform.globox_macos_software.globox_software_buffer_height;
}
