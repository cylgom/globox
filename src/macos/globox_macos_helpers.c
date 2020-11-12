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
unsigned long (*macos_msg_ulong_none)(id*, SEL) =
	(unsigned long (*)(id*, SEL)) objc_msgSend;
short (*macos_msg_short_none)(id*, SEL) =
	(short (*)(id*, SEL)) objc_msgSend;
long (*macos_msg_long_none)(id*, SEL) =
	(long (*)(id*, SEL)) objc_msgSend;

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

id (*macos_msg_id_rect)(
	id,
	SEL,
	struct macos_rect) =
(id (*)(id, SEL, struct macos_rect))
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

