#ifdef GLOBOX_WIN

#include "globox.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include "win.h"

// dummy event callback we exceptionally put here to avoid pointless complexity
LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// we must post some messages manually for reasons
	switch (msg)
	{
		case WM_SIZE:
		{
			PostMessage(hwnd, WM_SIZE, wParam, lParam);

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void resize(struct globox* globox)
{
	// update bitmap info
	globox->bmp_info.bmiHeader.biWidth = globox->width;
	globox->bmp_info.bmiHeader.biHeight = globox->height;

	if (globox->hbm != NULL)
	{
		DeleteObject(globox->hbm);
	}

	// re-create buffer
	HDC hdc = GetDC(globox->win_handle);

	globox->hbm = CreateDIBSection(
		hdc,
		&(globox->bmp_info),
		DIB_RGB_COLORS,
		(void**) &(globox->argb),
		NULL, // automatic memory allocation by uncle Windows
		0); // buffer offset

	ReleaseDC(globox->win_handle, hdc);
}

inline bool globox_open(
	struct globox* globox,
	enum globox_state state,
	const char* title,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height,
	bool frame_event)
{
	// common init
	globox->init_x = x;
	globox->init_y = y;
	globox->width = width;
	globox->height = height;
	globox->buf_width = width;
	globox->buf_height = height;
	globox->redraw = true;
	globox->frame_event = frame_event;
	globox->closed = false;

#if 0
	globox->fd_frame = timerfd_create(CLOCK_REALTIME, 0);
#endif

	globox->win_classname = to_wide(title);

	globox->win_class.cbSize = sizeof (globox->win_class);
	globox->win_class.style = 0;
	globox->win_class.lpfnWndProc = win_proc;
	globox->win_class.cbClsExtra = 0;
	globox->win_class.cbWndExtra = 0;
	globox->win_class.hInstance = GetModuleHandle(NULL);
	globox->win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    globox->win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	globox->win_class.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	globox->win_class.lpszMenuName = NULL;
	globox->win_class.lpszClassName = globox->win_classname;
	globox->win_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&(globox->win_class)))
	{
		MessageBox(
			NULL,
			TEXT("Window Registration Failed!"),
			TEXT("Error!"),
			MB_ICONERROR | MB_OK);

		return false;
	}

	globox->win_handle = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		globox->win_classname,
		TEXT(""),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		globox->win_class.hInstance,
		NULL);

	if (globox->win_handle == NULL)
	{
		MessageBox(
			NULL,
			TEXT("Window Creation Failed!"),
			TEXT("Error!"),
			MB_ICONERROR | MB_OK);

		return false;
	}

	// finish window initialization
	globox->fd.handle = globox->win_handle;
	UpdateWindow(globox->win_handle);

	globox->title = NULL;
	globox_set_title(globox, title);

	globox_set_state(globox, state);
	ShowWindow(globox->win_handle, SW_SHOWNORMAL); // TODO use state

	// prepare bitmap info
	// beware, some fields are not initialized here:
	// globox->bmp_info.bmiHeader.biWidth is set to globox->width in resize()
	// globox->bmp_info.bmiHeader.biHeight is set to globox->height in resize()
	// globox->bmp_info.bmiColors is NULL if biCompression is BI_RGB
	globox->bmp_info.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	globox->bmp_info.bmiHeader.biPlanes = 1; // de-facto
	globox->bmp_info.bmiHeader.biBitCount = 32; // XRGB
	globox->bmp_info.bmiHeader.biCompression = BI_RGB; // raw unpaletted format
	globox->bmp_info.bmiHeader.biSizeImage = 0;
	globox->bmp_info.bmiHeader.biXPelsPerMeter = 0; // fucking DPI
	globox->bmp_info.bmiHeader.biYPelsPerMeter = 0; // fucking orientation-dependant DPI
	globox->bmp_info.bmiHeader.biClrUsed = 0; // maximum as defined by biCompression
	globox->bmp_info.bmiHeader.biClrImportant = 0; // all colors required

	// set bitmap handle to NULL so it is not freed in resize()
	globox->hbm = NULL;

	// allocate buffer
	resize(globox);

	return true;
}

inline void globox_close(struct globox* globox)
{
	free(globox->win_classname);
	free(globox->title);
}

inline bool globox_handle_events(struct globox* globox)
{
	switch (globox->win_msg.message)
	{
		case WM_CLOSE:
		{
			DestroyWindow(globox->win_handle);

			break;
		}
		case WM_DESTROY:
		{
			DeleteObject(globox->hbm);
			PostQuitMessage(0);

			break;
		}
		case WM_PAINT:
		{
			globox->redraw = true;

			break;
		}
		case WM_SIZE:
		{
			globox->width = LOWORD(globox->win_msg.lParam);
			globox->height = HIWORD(globox->win_msg.lParam);
			globox->redraw = true;

			resize(globox);

			break;
		}
		default:
		{
			TranslateMessage(&(globox->win_msg));
			DispatchMessage(&(globox->win_msg));

			break;
		}
	}

	return true;
}

inline bool globox_shrink(struct globox* globox)
{
	return true;
}

inline void globox_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	// damage region
	RECT update =
	{
		.left = x,
		.top = y,
		.right = width,
		.bottom = height,
	};

	InvalidateRect(globox->win_handle, &update, TRUE);

	// paint
	PAINTSTRUCT paint;

	HDC hdc_window = BeginPaint(globox->win_handle, &paint);
	HDC hdc_compatible = CreateCompatibleDC(hdc_window);
	HBITMAP hbm_compatible_old = SelectObject(hdc_compatible, globox->hbm);

	BitBlt(
		hdc_window,
		x,
		y,
		width,
		height,
		hdc_compatible,
		x,
		y,
		SRCCOPY);

	SelectObject(hdc_compatible, hbm_compatible_old);
	DeleteDC(hdc_compatible);
	EndPaint(globox->win_handle, &paint);

	// done
	globox->redraw = false;
}

inline void globox_commit(struct globox* globox)
{

}

inline void globox_prepoll(struct globox* globox)
{
	// not used ATM
}

inline void globox_set_icon(struct globox* globox, uint32_t* pixmap, uint32_t len)
{

}

inline void globox_set_title(struct globox* globox, const char* title)
{
	if (globox->title != NULL)
	{
		free(globox->title);
	}

	globox->title = strdup(title);
}

inline void globox_set_state(struct globox* globox, enum globox_state state)
{
	switch (state)
	{
		case GLOBOX_STATE_REGULAR:
		{
			break;
		}
		case GLOBOX_STATE_MAXIMIZED:
		{
			break;
		}
		case GLOBOX_STATE_MINIMIZED:
		{
			break;
		}
		case GLOBOX_STATE_FULLSCREEN:
		{
			break;
		}
	}

	globox->state = state;
}

inline char* globox_get_title(struct globox* globox)
{
	return globox->title;
}

inline enum globox_state globox_get_state(struct globox* globox)
{
	return globox->state;
}

inline void globox_get_size(struct globox* globox, uint32_t* width, uint32_t* height)
{
	*width = globox->width;
	*height = globox->height;
}

#endif
