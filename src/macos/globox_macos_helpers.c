#include "globox.h"
#include "macos/globox_macos.h"
#include "macos/globox_macos_helpers.h"

// implement objc_msgSend variants
void (*macos_msg_void_none)(id, SEL) =
	(void (*)(id, SEL)) objc_msgSend;
id (*macos_msg_id_none)(id, SEL) =
	(id (*)(id, SEL)) objc_msgSend;
bool (*macos_msg_bool_none)(id, SEL) =
	(bool (*)(id, SEL)) objc_msgSend;
id* (*macos_msg_idptr_none)(id, SEL) =
	(id* (*)(id, SEL)) objc_msgSend;
id* (*macos_msg_idptr_int)(id, SEL, int) =
	(id* (*)(id, SEL, int)) objc_msgSend;
unsigned long (*macos_msg_ulong_none)(id*, SEL) =
	(unsigned long (*)(id*, SEL)) objc_msgSend;
struct macos_rect (*macos_msg_rect_none)(id, SEL) =
	(struct macos_rect (*)(id, SEL)) objc_msgSend_stret;
short (*macos_msg_short_none)(id*, SEL) =
	(short (*)(id*, SEL)) objc_msgSend;
long (*macos_msg_long_none)(id*, SEL) =
	(long (*)(id*, SEL)) objc_msgSend;
struct macos_point (*macos_msg_loc_none)(id*, SEL) =
	(struct macos_point (*)(id*, SEL)) objc_msgSend;
struct macos_point (*macos_msg_point_none)(id, SEL) =
	(struct macos_point (*)(id, SEL)) objc_msgSend;

void (*macos_msgptr_void_bool)(
	id*,
	SEL,
	bool) =
(void (*)(id*, SEL, bool))
	objc_msgSend;

void (*macos_msg_void_id)(
	id,
	SEL,
	id) =
(void (*)(id, SEL, id))
	objc_msgSend;

void (*macos_msg_void_int)(
	id,
	SEL,
	int) =
(void (*)(id, SEL, int))
	objc_msgSend;

void (*macos_msg_void_ulong)(
	id,
	SEL,
	unsigned long) =
(void (*)(id, SEL, unsigned long))
	objc_msgSend;

void (*macos_msg_void_bool)(
	id,
	SEL,
	bool) =
(void (*)(id, SEL, bool))
	objc_msgSend;

void (*macos_msg_void_idptr)(
	id,
	SEL,
	id*) =
(void (*)(id, SEL, id*))
	objc_msgSend;

void (*macos_msg_void_voidptr)(
	id,
	SEL,
	void*) =
(void (*)(id, SEL, void*))
	objc_msgSend;

void (*macos_msg_void_size)(
	id,
	SEL,
	struct macos_size) =
(void (*)(id, SEL, struct macos_size))
	objc_msgSend;

void (*macos_msg_void_point)(
	id,
	SEL,
	struct macos_point) =
(void (*)(id, SEL, struct macos_point))
	objc_msgSend;

id (*macos_msg_id_size)(
	id,
	SEL,
	struct macos_size) =
(id (*)(id, SEL, struct macos_size))
	objc_msgSend;

id (*macos_msg_id_rect)(
	id,
	SEL,
	struct macos_rect) =
(id (*)(id, SEL, struct macos_rect))
	objc_msgSend;

void (*macos_msg_super_framesize)(
	struct objc_super*,
	SEL,
	struct macos_size) =
(void (*)(struct objc_super*, SEL, struct macos_size))
	objc_msgSendSuper;

id* (*macos_msg_subview)(
	id,
	SEL,
	id,
	int,
	id) =
(id*(*)(id, SEL, id, int, id))
	objc_msgSend;

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
	long) =
(id*(*)(id, SEL, unsigned long, struct macos_point, unsigned long, double, long, id, short, long, long))
	objc_msgSend;

id (*macos_msg_win)(
	id,
	SEL,
	struct macos_rect,
	int,
	int,
	bool) =
(id(*)(id, SEL, struct macos_rect, int, int, bool))
	objc_msgSend;

void (*macos_msg_post)(
	id,
	SEL,
	id*,
	bool) =
(void(*)(id, SEL, id*, bool))
	objc_msgSend;

id* (*macos_msg_poll)(
	id,
	SEL,
	unsigned
	long long,
	id*,
	id*,
	bool) =
(id*(*)(id, SEL, unsigned long long, id*, id*, bool))
	objc_msgSend;

id (*macos_msg_string_charptr_int)(
	id,
	SEL,
	char*,
	int) =
(id (*)(id, SEL, char*, int))
 	objc_msgSend;

void (*macos_msg_resize)(
	id,
	SEL,
	struct macos_rect,
	bool) =
(void (*)(id, SEL, struct macos_rect, bool))
	objc_msgSend;
