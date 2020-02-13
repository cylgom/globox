#ifndef H_GLOBOX_QUARTZ
#define H_GLOBOX_QUARTZ

#include "globox.h"
#include <objc/message.h>
#include <objc/runtime.h>

enum NSWindowStyleMask
{
	NSWindowStyleMaskTitled = 1,
	NSWindowStyleMaskClosable = 2,
	NSWindowStyleMaskMiniaturizable = 4,
	NSWindowStyleMaskResizable = 8,
	NSWindowStyleMaskFullScreen = 16384,
};

enum NSBackingStoreType
{
	NSBackingStoreBuffered = 2,
};

struct quartz_point
{
	double x;
	double y;
};

struct quartz_size
{
	double width;
	double height;
};

struct quartz_rect
{
	struct quartz_point origin;
	struct quartz_size size;
};

struct quartz_app
{
	Class isa;
	id window;
};

extern id(*quartz_msg_id)(id, SEL);
extern void(*quartz_msg_void)(id, SEL);
extern void(*quartz_msg_ptr)(id, SEL, void*);
extern id(*quartz_msg_rect)(id, SEL, struct quartz_rect);
extern id(*quartz_msg_win)(id, SEL, struct quartz_rect, int, int, bool);
extern void NSRectFill(struct quartz_rect rect);

void set_pointer(struct globox* globox);
void quartz_rect(id app, SEL cmd, struct quartz_rect rect);
bool post_launch(struct quartz_app* app, SEL cmd, id msg);

#endif
