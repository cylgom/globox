#ifndef H_QUARTZ
#define H_QUARTZ

#include "quartz_helpers.h"

enum globox_quartz_app_event
{
	GLOBOX_QUARTZ_EVENT_WINDOW_STATE,
	GLOBOX_QUARTZ_EVENT_WINDOW_CLOSE,
};

enum globox_quartz_window_event
{
	GLOBOX_QUARTZ_WINDOW_EVENT_MAXIMIZE_ON,
	GLOBOX_QUARTZ_WINDOW_EVENT_MAXIMIZE_OFF,
	GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_ON,
	GLOBOX_QUARTZ_WINDOW_EVENT_MINIMIZE_OFF,
	GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_ON,
	GLOBOX_QUARTZ_WINDOW_EVENT_FULLSCREEN_OFF,
};

void quartz_view_draw_rect_callback(
	id app,
	SEL cm,
	struct quartz_rect rect);

void quartz_window_event(
	short event_subtype,
	long event_value);

struct quartz_rect quartz_window_event_maximize_on(
	id window_delegate,
	SEL cmd,
	id* window,
	struct quartz_rect rect);

void quartz_window_event_move(
	id window_delegate,
	SEL cmd,
	id* notif);

void quartz_window_event_minimize_on(
	id window_delegate,
	SEL cmd,
	id* notif);

void quartz_window_event_minimize_off(
	id window_delegate,
	SEL cmd,
	id* notif);

void quartz_window_event_fullscreen_on(
	id window_delegate,
	SEL cmd,
	id* notif);

void quartz_window_event_fullscreen_off(
	id window_delegate,
	SEL cmd,
	id* notif);

void quartz_window_event_close(
	id window_delegate,
	SEL cmd,
	id* notif);

bool quartz_app_delegate_init_callback(
	struct quartz_app_delegate* app_delegate,
	SEL cmd,
	id msg);

#endif
