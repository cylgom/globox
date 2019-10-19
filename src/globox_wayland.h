#ifndef H_GLOBOX_WAYLAND
#define H_GLOBOX_WAYLAND

#include <stdbool.h>
#include "globox.h"

bool globox_open_wayland(struct globox* globox, const char* title);
void globox_close_wayland(struct globox* globox);

bool globox_handle_events_wayland(struct globox* globox);
bool globox_shrink_wayland(struct globox* globox);

void globox_copy_wayland(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_commit_wayland(
	struct globox* globox);

void globox_set_icon_wayland(struct globox* globox, uint32_t* pixmap, uint32_t len);
void globox_set_title_wayland(struct globox* globox, const char* title);
void globox_set_state_wayland(struct globox* globox, enum globox_state state);
void globox_set_pos_wayland(struct globox* globox, uint32_t x, uint32_t y);
bool globox_set_size_wayland(struct globox* globox, uint32_t width, uint32_t height);

char* globox_get_title_wayland(struct globox* globox);
enum globox_state globox_get_state_wayland(struct globox* globox);
void globox_get_pos_wayland(struct globox* globox, int32_t* x, int32_t* y);
void globox_get_size_wayland(struct globox* globox, uint32_t* width, uint32_t* height);

#endif
