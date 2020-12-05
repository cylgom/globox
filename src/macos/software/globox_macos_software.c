#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"
#include "macos/globox_macos.h"
// system includes
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
// macos includes
#include "macos/globox_macos_helpers.h"

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
	platform->globox_macos_obj_masterview =
		macos_msg_id_rect(
			platform->globox_macos_obj_masterview,
			sel_getUid("initWithFrame:"),
			frame);

	platform->globox_macos_obj_view =
		macos_msg_id_rect(
			platform->globox_macos_obj_view,
			sel_getUid("initWithFrame:"),
			frame);

	macos_msg_void_bool(
		platform->globox_macos_obj_view,
		sel_getUid("setWantsLayer:"),
		true);

	platform->globox_macos_layer =
		macos_msg_id_none(
			platform->globox_macos_obj_view,
			sel_getUid("layer"));

	platform->globox_macos_obj_blur =
		macos_msg_id_rect(
			platform->globox_macos_obj_blur,
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

#if 0
	// inject the drawRect callback
	class_addMethod(
		platform->globox_macos_class_view,
		sel_getUid("drawRect:"),
		(IMP) view_drawrect_software,
		"v@:");
#endif

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
	// re-write
#if 0
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_software* context = &(platform->globox_macos_software);

	context->globox_software_buffer_width = globox->globox_width;
	context->globox_software_buffer_height = globox->globox_height;
#endif
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
	struct globox_platform* platform = &(globox->globox_platform);
	struct globox_macos_software* context = &(platform->globox_macos_software);

	globox->globox_redraw = false;

	id colorspace =
		CGColorSpaceCreateDeviceRGB();

	id bitmap =
		CGBitmapContextCreate(
			platform->globox_platform_argb,
			context->globox_software_buffer_width,
			context->globox_software_buffer_height,
			8,
			context->globox_software_buffer_width * 4,
			colorspace,
			(4 << 12) | (1));

	id image =
		CGBitmapContextCreateImage(bitmap);

	macos_msg_void_id(
		platform->globox_macos_layer,
		sel_getUid("setContents:"),
		image);

	struct macos_rect frame =
		macos_msg_rect_none(
			platform->globox_macos_obj_view,
			sel_getUid("frame"));

	if ((frame.size.width
			* frame.size.height)
		> (context->globox_software_buffer_width
			* context->globox_software_buffer_height))
	{
		free(platform->globox_platform_argb);
		platform->globox_platform_argb =
			(uint32_t*) malloc(
				4 * frame.size.width * frame.size.height);
		globox->globox_redraw = true;
	}

	context->globox_software_buffer_width = frame.size.width;
	context->globox_software_buffer_height = frame.size.height;
	globox->globox_width = frame.size.width;
	globox->globox_height = frame.size.height;

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
