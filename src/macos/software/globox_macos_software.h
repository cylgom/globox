#ifndef H_GLOBOX_MACOS_SOFTWARE
#define H_GLOBOX_MACOS_SOFTWARE

#include <stdint.h>
#include <objc/objc.h>

struct globox_macos_software
{
	uint32_t globox_software_buffer_width;
	uint32_t globox_software_buffer_height;

	id globox_software_layer;
};

#endif
