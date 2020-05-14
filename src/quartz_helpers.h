#ifndef H_QUARTZ_HELPERS
#define H_QUARTZ_HELPERS

#include <objc/message.h>
#include <objc/runtime.h>

#define NSEventMaskAny 0xFFFFFFFFFFFFFFFF

enum NSStringEncoding
{
	NSUTF8StringEncoding = 4,
};

enum NSBackingStoreType
{
	NSBackingStoreBuffered = 2,
};

enum NSEventType
{
	NSEventTypeApplicationDefined = 15,
};

enum NSWindowStyleMask
{
	NSWindowStyleMaskTitled = 1,
	NSWindowStyleMaskClosable = 2,
	NSWindowStyleMaskMiniaturizable = 4,
	NSWindowStyleMaskResizable = 8,
	NSWindowStyleMaskFullScreen = 16384,
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

struct quartz_app_delegate
{
	Class isa;
	id window;
};

id(*quartz_msg_id)(id, SEL);
void(*quartz_msg_int)(id, SEL, int);
id*(*quartz_msg_date)(id, SEL);
bool(*quartz_msg_bool)(id, SEL);
void(*quartz_msg_void)(id, SEL);
unsigned long(*quartz_msg_type)(id*, SEL);
void(*quartz_msg_ptr)(id, SEL, void*);
void(*quartz_msg_obj)(id, SEL, id);
void(*quartz_msg_send)(id, SEL, id*);
id(*quartz_msg_string)(id, SEL, const char*, unsigned long);
void(*quartz_msg_post)(id, SEL, id*, bool);
id*(*quartz_msg_event)(id, SEL, unsigned long, struct quartz_point, unsigned long, double, long, id, short, long, long);
id(*quartz_msg_rect)(id, SEL, struct quartz_rect);
id(*quartz_msg_win)(id, SEL, struct quartz_rect, int, int, bool);
id*(*quartz_msg_poll)(id, SEL, unsigned long long, id*, id*, bool);

extern void* CGBitmapContextGetData(id);
extern size_t CGBitmapContextGetWidth(id);
extern size_t CGBitmapContextGetHeight(id);
extern size_t CGBitmapContextGetBytesPerRow(id);
extern size_t CGBitmapContextGetBitsPerComponent(id);
extern size_t CGBitmapContextGetBitsPerPixel(id);
extern void NSRectFill(struct quartz_rect rect);
extern SEL NSSelectorFromString(const char*);
extern id* NSDefaultRunLoopMode;

#endif
