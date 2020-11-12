#ifndef H_GLOBOX_MACOS_HELPERS
#define H_GLOBOX_MACOS_HELPERS

#include <objc/message.h>
#include <objc/runtime.h>

// missing native defines
#define NSEventMaskAny 0xFFFFFFFFFFFFFFFF

// missing native enums
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

// missing native structs
struct macos_point
{
	double x;
	double y;
};

struct macos_size
{
	double width;
	double height;
};

struct macos_rect
{
	struct macos_point origin;
	struct macos_size size;
};

struct macos_appdelegate
{
	Class isa;
	id window;
};

// window state events
enum macos_events_subtype
{
	GLOBOX_MACOS_EVENT_WINDOW_STATE,
	GLOBOX_MACOS_EVENT_WINDOW_CLOSE,
};

enum macos_events_window_state
{
	GLOBOX_MACOS_EVENT_WINDOW_MINIMIZE_ON,
	GLOBOX_MACOS_EVENT_WINDOW_MINIMIZE_OFF,
	GLOBOX_MACOS_EVENT_WINDOW_FULLSCREEN_ON,
	GLOBOX_MACOS_EVENT_WINDOW_FULLSCREEN_OFF,
	GLOBOX_MACOS_EVENT_WINDOW_MAXIMIZE_ON,
	GLOBOX_MACOS_EVENT_WINDOW_MAXIMIZE_OFF,
};

// declare objc_msgSend variants
void (*macos_msg_void_none)(id, SEL);
id (*macos_msg_id_none)(id, SEL);
bool (*macos_msg_bool_none)(id, SEL);
id* (*macos_msg_idptr_none)(id, SEL);
unsigned long (*macos_msg_ulong_none)(id*, SEL);
short (*macos_msg_short_none)(id*, SEL);
long (*macos_msg_long_none)(id*, SEL);

void (*macos_msg_void_id)(
	id,
	SEL,
	id);

void (*macos_msg_void_int)(
	id,
	SEL,
	int);

void (*macos_msg_void_idptr)(
	id,
	SEL,
	id*);

void (*macos_msg_void_voidptr)(
	id,
	SEL,
	void*);

id (*macos_msg_id_rect)(
	id,
	SEL,
	struct macos_rect);

id* (*macos_msg_event)(
	id,
	SEL,
	unsigned long,
	struct macos_point,
	unsigned long,
	double,
	long,
	id,
	short,
	long,
	long);

id (*macos_msg_win)(
	id,
	SEL,
	struct macos_rect,
	int,
	int,
	bool);

void (*macos_msg_post)(
	id,
	SEL,
	id*,
	bool);

id* (*macos_msg_poll)(
	id,
	SEL,
	unsigned
	long long,
	id*,
	id*,
	bool);

id (*macos_msg_string_charptr_int)(
	id,
	SEL,
	char*,
	int);

// extern native functions
extern void* CGBitmapContextGetData(id);
extern size_t CGBitmapContextGetWidth(id);
extern size_t CGBitmapContextGetHeight(id);
extern size_t CGBitmapContextGetBytesPerRow(id);
extern size_t CGBitmapContextGetBitsPerComponent(id);
extern size_t CGBitmapContextGetBitsPerPixel(id);

extern void NSRectFill(struct macos_rect rect);
extern SEL NSSelectorFromString(const char*);
extern id* NSDefaultRunLoopMode;

#endif
