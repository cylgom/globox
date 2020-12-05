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

enum NSEventSubtype
{
	NSEventSubtypeApplicationActivated = 1,
};

enum NSEventType
{
	NSEventTypeLeftMouseDown = 1,
	NSEventTypeLeftMouseUp = 2,
	NSEventTypeMouseMoved = 5,
	NSEventTypeLeftMouseDragged = 6,
	NSEventTypeAppKitDefined = 13,
	NSEventTypeApplicationDefined = 15,
};

enum NSApplicationPresentationOptions
{
	NSApplicationPresentationHideDock = 2,
	NSApplicationPresentationHideMenuBar = 8,
	NSApplicationPresentationDisableAppleMenu = 16,
	NSApplicationPresentationFullScreen = 1024,
};

enum NSEventModifierFlags
{
	NSEventModifierFlagShift = 1 << 17,
	NSEventModifierFlagOption = 1 << 19,
};

enum NSWindowStyleMask
{
	NSWindowStyleMaskTitled = 1,
	NSWindowStyleMaskClosable = 2,
	NSWindowStyleMaskMiniaturizable = 4,
	NSWindowStyleMaskResizable = 8,
	NSWindowStyleMaskFullScreen = 16384,
};

enum NSWindowCollectionBehavior
{
	NSWindowCollectionBehaviorParticipatesInCycle = 1 << 5,
	NSWindowCollectionBehaviorFullScreenPrimary = 1 << 7,
	NSWindowCollectionBehaviorFullScreenAllowsTiling = 1 << 11,
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
	GLOBOX_MACOS_EVENT_WINDOW_RESIZE,
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
id* (*macos_msg_idptr_int)(id, SEL, int);
unsigned long (*macos_msg_ulong_none)(id*, SEL);
struct macos_rect (*macos_msg_rect_none)(id, SEL);
short (*macos_msg_short_none)(id*, SEL);
long (*macos_msg_long_none)(id*, SEL);
struct macos_point (*macos_msg_loc_none)(id*, SEL);
struct macos_point (*macos_msg_point_none)(id, SEL);

void (*macos_msgptr_void_bool)(
	id*,
	SEL,
	bool);

void (*macos_msg_void_id)(
	id,
	SEL,
	id);

void (*macos_msg_void_int)(
	id,
	SEL,
	int);

void (*macos_msg_void_ulong)(
	id,
	SEL,
	unsigned long);

void (*macos_msg_void_bool)(
	id,
	SEL,
	bool);

void (*macos_msg_void_idptr)(
	id,
	SEL,
	id*);

void (*macos_msg_void_voidptr)(
	id,
	SEL,
	void*);

void (*macos_msg_void_size)(
	id,
	SEL,
	struct macos_size);

void (*macos_msg_void_point)(
	id,
	SEL,
	struct macos_point);

id (*macos_msg_id_size)(
	id,
	SEL,
	struct macos_size);

id (*macos_msg_id_rect)(
	id,
	SEL,
	struct macos_rect);

void (*macos_msg_super_framesize)(
	struct objc_super*,
	SEL,
	struct macos_size);

id* (*macos_msg_subview)(
	id,
	SEL,
	id,
	int,
	id);

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

void (*macos_msg_resize)(
	id,
	SEL,
	struct macos_rect,
	bool);

// extern native functions
extern void* CGBitmapContextGetData(id);
extern size_t CGBitmapContextGetWidth(id);
extern size_t CGBitmapContextGetHeight(id);
extern size_t CGBitmapContextGetBytesPerRow(id);
extern size_t CGBitmapContextGetBitsPerComponent(id);
extern size_t CGBitmapContextGetBitsPerPixel(id);
extern id CGLayerGetContext(id);

extern void NSRectFill(struct macos_rect rect);
extern SEL NSSelectorFromString(const char*);
extern id* NSDefaultRunLoopMode;

extern id CGBitmapContextCreateImage(id);
extern id CGColorSpaceCreateDeviceRGB();
extern id CGBitmapContextCreate(
	void*,
	size_t,
	size_t,
	size_t,
	size_t,
	id,
	uint32_t);

#endif
