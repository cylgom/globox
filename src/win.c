#include "win.h"
#include <windows.h>

LPWSTR to_wide(const char* s)
{
    size_t codepoint_count = MultiByteToWideChar(CP_ACP, 0, s, -1, 0, 0);
    wchar_t* buf = malloc(codepoint_count * (sizeof (wchar_t)));

    MultiByteToWideChar(CP_ACP, 0, s, -1, buf, codepoint_count);

    return buf;
}
