#include "quartz_helpers.h"

// redefine ambiguous symbols
id(*quartz_msg_id)(id, SEL) =
	(id(*)(id, SEL)) objc_msgSend;

id*(*quartz_msg_date)(id, SEL) =
	(id*(*)(id, SEL)) objc_msgSend;

bool(*quartz_msg_bool)(id, SEL) =
	(bool(*)(id, SEL)) objc_msgSend;

void(*quartz_msg_void)(id, SEL) =
	(void(*)(id, SEL)) objc_msgSend;

unsigned long(*quartz_msg_type)(id*, SEL) =
	(unsigned long(*)(id*, SEL)) objc_msgSend;

void(*quartz_msg_ptr)(id, SEL, void*) =
	(void(*)(id, SEL, void*)) objc_msgSend;

void(*quartz_msg_obj)(id, SEL, id) =
	(void(*)(id, SEL, id)) objc_msgSend;

void(*quartz_msg_send)(id, SEL, id*) =
	(void(*)(id, SEL, id*)) objc_msgSend;

id(*quartz_msg_string)(id, SEL, const char*, unsigned long) =
	(id(*)(id, SEL, const char*, unsigned long)) objc_msgSend;

void(*quartz_msg_post)(id, SEL, id*, bool) =
	(void(*)(id, SEL, id*, bool)) objc_msgSend;

id*(*quartz_msg_event)(id, SEL, unsigned long, struct quartz_point, unsigned long, double, long, id, short, long, long) =
	(id*(*)(id, SEL, unsigned long, struct quartz_point, unsigned long, double, long, id, short, long, long)) objc_msgSend;

id(*quartz_msg_rect)(id, SEL, struct quartz_rect) =
	(id(*)(id, SEL, struct quartz_rect)) objc_msgSend;

id(*quartz_msg_win)(id, SEL, struct quartz_rect, int, int, bool) =
	(id(*)(id, SEL, struct quartz_rect, int, int, bool)) objc_msgSend;

id*(*quartz_msg_poll)(id, SEL, unsigned long long, id*, id*, bool) =
	(id*(*)(id, SEL, unsigned long long, id*, id*, bool)) objc_msgSend;

// fetch accessible symbols
extern void* CGBitmapContextGetData(id);
extern size_t CGBitmapContextGetWidth(id);
extern size_t CGBitmapContextGetHeight(id);
extern size_t CGBitmapContextGetBytesPerRow(id);
extern size_t CGBitmapContextGetBitsPerComponent(id);
extern size_t CGBitmapContextGetBitsPerPixel(id);
extern void NSRectFill(struct quartz_rect rect);
extern SEL NSSelectorFromString(const char*);
extern id* NSDefaultRunLoopMode;
