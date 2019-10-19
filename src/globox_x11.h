#ifndef H_GLOBOX_X11
#define H_GLOBOX_X11

#include <stdbool.h>
#include "globox.h"

bool globox_open_x11(struct globox* globox, const char* title);
void globox_close_x11(struct globox* globox);

bool globox_handle_events_x11(struct globox* globox);
bool globox_shrink_x11(struct globox* globox);

void globox_copy_x11(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height);

void globox_commit_x11(
	struct globox* globox);

void globox_set_icon_x11(struct globox* globox, uint32_t* pixmap, uint32_t len);
void globox_set_title_x11(struct globox* globox, const char* title);
void globox_set_state_x11(struct globox* globox, enum globox_state state);
void globox_set_pos_x11(struct globox* globox, uint32_t x, uint32_t y);
bool globox_set_size_x11(struct globox* globox, uint32_t width, uint32_t height);

char* globox_get_title_x11(struct globox* globox);
enum globox_state globox_get_state_x11(struct globox* globox);
void globox_get_pos_x11(struct globox* globox, int32_t* x, int32_t* y);
void globox_get_size_x11(struct globox* globox, uint32_t* width, uint32_t* height);

#endif
