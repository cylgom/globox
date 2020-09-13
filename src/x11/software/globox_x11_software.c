
#define _XOPEN_SOURCE 700

#include "globox.h"
#include "globox_error.h"
#include "x11/globox_x11.h"
// system includes
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
// x11 includes
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_image.h>
#include <xcb/shm.h>

void globox_context_x11_software_init(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);
	platform->globox_x11_visual_id = platform->globox_x11_screen_obj->root_visual;
	platform->globox_x11_software.globox_x11_software_buffer_width = globox->globox_width;
	platform->globox_x11_software.globox_x11_software_buffer_height = globox->globox_height;
}

void shm_create(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	int shmid =
		shmget(
			IPC_PRIVATE,
			4 * globox->globox_width * globox->globox_height,
			IPC_CREAT | 0600);

	if (shmid == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_SHMID);
		return;
	}

	platform->globox_x11_software.globox_x11_software_shm.shmid = shmid;

	platform->globox_x11_software.globox_x11_software_shm.shmaddr =
		shmat(
			platform->globox_x11_software.globox_x11_software_shm.shmid,
			0,
			0);

	if (platform->globox_x11_software.globox_x11_software_shm.shmaddr == ((void*) -1))
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_SHMADDR);
		return;
	}

	xcb_void_cookie_t cookie_attach =
		xcb_shm_attach(
			platform->globox_x11_conn,
			platform->globox_x11_software.globox_x11_software_shm.shmseg,
			platform->globox_x11_software.globox_x11_software_shm.shmid,
			0);

	xcb_generic_error_t* error_attach =
		xcb_request_check(
			platform->globox_x11_conn,
			cookie_attach);

	if (error_attach != NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_SHM_ATTACH);
		return;
	}

	int error_shmctl =
		shmctl(
			platform->globox_x11_software.globox_x11_software_shm.shmid,
			IPC_RMID,
			0);

	if (error_shmctl == -1)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_SHMCTL);
		return;
	}

	platform->globox_platform_argb =
		(uint32_t*) platform->globox_x11_software.globox_x11_software_shm.shmaddr;
}

void globox_context_x11_software_create(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	// create XCB graphics context
	uint32_t values[2] =
	{
		platform->globox_x11_screen_obj->black_pixel,
		0,
	};

	platform->globox_x11_software.globox_x11_software_gfx =
		xcb_generate_id(
			platform->globox_x11_conn);

	xcb_void_cookie_t cookie_gc =
		xcb_create_gc_checked(
			platform->globox_x11_conn,
			platform->globox_x11_software.globox_x11_software_gfx,
			platform->globox_x11_win,
			XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES,
			values);

	xcb_generic_error_t* error_gc =
		xcb_request_check(
			platform->globox_x11_conn,
			cookie_gc);

	if (error_gc != NULL)
	{
		// throw error
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_GC);
		return;
	}

	// check display server settings compatibility
	xcb_visualtype_t* visual = NULL;
	xcb_visualid_t visual_root = platform->globox_x11_visual_id;

	xcb_depth_iterator_t depth_iter =
		xcb_screen_allowed_depths_iterator(
			platform->globox_x11_screen_obj);

	xcb_visualtype_iterator_t visual_iter;

	while (depth_iter.rem)
	{
		visual_iter = xcb_depth_visuals_iterator(depth_iter.data);

		while (visual_iter.rem)
		{
			if (visual_root == visual_iter.data->visual_id)
			{
				visual = visual_iter.data;
				break;
			}

			xcb_visualtype_next(&visual_iter);
		}

		if (visual != NULL)
		{
			if ((visual->_class != XCB_VISUAL_CLASS_TRUE_COLOR)
				&& (visual->_class != XCB_VISUAL_CLASS_DIRECT_COLOR))
			{
				globox_error_throw(
					globox,
					GLOBOX_ERROR_X11_VISUAL_NOT_COMPATIBLE);
				return;
			}
			else
			{
				break;
			}
		}

		xcb_depth_next(&depth_iter);
	}

	if (!depth_iter.rem)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_VISUAL_NOT_FOUND);
	}

	// we are not done yet as we wish to bypass the xcb drawing API to
	// write directly to a shared memory buffer (just like CPU wayland)
	xcb_generic_error_t* error_shm;

	xcb_shm_query_version_cookie_t cookie_shm =
		xcb_shm_query_version(
			platform->globox_x11_conn);

	xcb_shm_query_version_reply_t* reply_shm =
		xcb_shm_query_version_reply(
			platform->globox_x11_conn,
			cookie_shm,
			&error_shm);

	if ((error_shm != NULL) || (reply_shm == NULL))
	{
		// throw error
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_SHM_VERSION_REPLY);
		return;
	}

	platform->globox_x11_software.globox_x11_software_shared_pixmaps = reply_shm->shared_pixmaps;
	free(reply_shm);

	xcb_void_cookie_t cookie_pixmap;
	xcb_generic_error_t* error_pixmap;

	// unlike wayland, X can't automatically copy buffers from cpu to gpu
	// so if the display server is running in DRM we need to do it manually
	// for this we can use xcb_put_image() to transfer the data using a socket
	if (platform->globox_x11_software.globox_x11_software_shared_pixmaps == false)
	{
		platform->globox_platform_argb = malloc(4 * globox->globox_width * globox->globox_height);

		if (platform->globox_platform_argb == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_ALLOC);
		}

		platform->globox_x11_software.globox_x11_software_pixmap = xcb_generate_id(platform->globox_x11_conn);
		
		cookie_pixmap =
			xcb_create_pixmap_checked(
				platform->globox_x11_conn,
				24, // force 24bpp instead of geometry->depth
				platform->globox_x11_software.globox_x11_software_pixmap,
				platform->globox_x11_win,
				globox->globox_width,
				globox->globox_height);

		error_pixmap =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_pixmap);

		if (error_pixmap != NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_PIXMAP);
			return;
		}
	}
	else
	{
		platform->globox_x11_software.globox_x11_software_shm.shmseg =
			xcb_generate_id(
				platform->globox_x11_conn);

		platform->globox_x11_software.globox_x11_software_pixmap =
			xcb_generate_id(
				platform->globox_x11_conn);

		shm_create(globox);

		if (globox_error_catch(globox))
		{
			return;
		}
#if 0
		platform->globox_x11_software.globox_x11_software_shm.shmid =
			shmget(
				IPC_PRIVATE,
				4 * globox->globox_width * globox->globox_height,
				IPC_CREAT | 0600);

		if (platform->globox_x11_software.globox_x11_software_shm.shmid == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMID);
			return;
		}

		platform->globox_x11_software.globox_x11_software_shm.shmaddr =
			shmat(
				platform->globox_x11_software.globox_x11_software_shm.shmid,
				0,
				0);

		if (platform->globox_x11_software.globox_x11_software_shm.shmaddr == ((void*) -1))
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMADDR);
			return;
		}

		xcb_void_cookie_t cookie_attach =
			xcb_shm_attach(
				platform->globox_x11_conn,
				platform->globox_x11_software.globox_x11_software_shm.shmseg,
				platform->globox_x11_software.globox_x11_software_shm.shmid,
				0);

		xcb_generic_error_t* error_attach =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_attach);

		if (error_attach != NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHM_ATTACH);
			return;
		}

		int error_shmctl =
			shmctl(
				platform->globox_x11_software.globox_x11_software_shm.shmid,
				IPC_RMID,
				0);

		if (shmctl == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMCTL);
			return;
		}

		platform->globox_platform_argb =
			(uint32_t*) platform->globox_x11_software.globox_x11_software_shm.shmaddr;
#endif

		cookie_pixmap = xcb_shm_create_pixmap_checked(
			platform->globox_x11_conn,
			platform->globox_x11_software.globox_x11_software_pixmap,
			platform->globox_x11_win,
			globox->globox_width,
			globox->globox_height,
			24, // force 24bpp instead of geometry->depth
			platform->globox_x11_software.globox_x11_software_shm.shmseg,
			0);

		error_pixmap =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_pixmap);

		if (error_pixmap != NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHM_PIXMAP);
			return;
		}
	}
}

void globox_context_x11_software_shrink(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	platform->globox_x11_software.globox_x11_software_buffer_width = globox->globox_width;
	platform->globox_x11_software.globox_x11_software_buffer_height = globox->globox_height;

	if (platform->globox_x11_software.globox_x11_software_shared_pixmaps == false)
	{
		platform->globox_platform_argb =
			realloc(
				platform->globox_platform_argb,
				4 * globox->globox_width * globox->globox_height);

		if (platform->globox_platform_argb == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_ALLOC);
		}
	}
	else
	{
		int shmid =
			shmget(
				IPC_PRIVATE,
				4 * globox->globox_width * globox->globox_height,
				IPC_CREAT | 0600);

		if (shmid == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMID);
			return;
		}

		platform->globox_x11_software.globox_x11_software_shm.shmid = shmid;

		uint8_t* tmpaddr =
			shmat(
				platform->globox_x11_software.globox_x11_software_shm.shmid,
				0,
				0);

		if (tmpaddr == ((void*) -1))
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMADDR);
			return;
		}

		xcb_void_cookie_t cookie_shm;
		xcb_generic_error_t* error_shm;

		cookie_shm =
			xcb_shm_detach_checked(
				platform->globox_x11_conn,
				platform->globox_x11_software.globox_x11_software_shm.shmseg);

		error_shm =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_shm);

		if (error_shm == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHM_DETACH);
			return;
		}

		cookie_shm =
			xcb_shm_attach_checked(
				platform->globox_x11_conn,
				platform->globox_x11_software.globox_x11_software_shm.shmseg,
				platform->globox_x11_software.globox_x11_software_shm.shmid,
				0);

		error_shm =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_shm);

		if (error_shm == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHM_ATTACH);
			return;
		}

		int error_shmctl =
			shmctl(
				platform->globox_x11_software.globox_x11_software_shm.shmid,
				IPC_RMID,
				0);

		if (error_shmctl == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMCTL);
			return;
		}

		memcpy(
			(uint32_t*) tmpaddr,
			platform->globox_platform_argb,
			4 * globox->globox_width * globox->globox_height);

		int error_shmdt =
			shmdt(
				platform->globox_x11_software.globox_x11_software_shm.shmaddr);

		if (error_shmdt == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMDT);
			return;
		}

		platform->globox_x11_software.globox_x11_software_shm.shmaddr = tmpaddr;
		platform->globox_platform_argb = (uint32_t*) tmpaddr;
	}
}

void globox_context_x11_software_free(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	xcb_free_pixmap(
		platform->globox_x11_conn,
		platform->globox_x11_software.globox_x11_software_pixmap);

	if (platform->globox_x11_software.globox_x11_software_shared_pixmaps == false)
	{
		free(platform->globox_platform_argb);
	}
	else
	{
		xcb_void_cookie_t cookie_shm =
			xcb_shm_detach_checked(
				platform->globox_x11_conn,
				platform->globox_x11_software.globox_x11_software_shm.shmseg);

		xcb_generic_error_t* error_shm =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_shm);

		if (error_shm == NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHM_DETACH);
			return;
		}

		int error_shmdt =
			shmdt(
				platform->globox_x11_software.globox_x11_software_shm.shmaddr);

		if (error_shmdt == -1)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_SHMDT);
			return;
		}
	}
}

void globox_context_x11_software_copy(
	struct globox* globox,
	int32_t x,
	int32_t y,
	uint32_t width,
	uint32_t height)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	if (platform->globox_x11_software.globox_x11_software_shared_pixmaps == false)
	{
		int32_t y2 = y;
		uint32_t height2 = height;

		size_t len = sizeof (xcb_get_image_request_t);
		size_t len_theoric = (len + (4 * globox->globox_width * height2)) >> 2;

		uint64_t len_max =
			xcb_get_maximum_request_length(
				platform->globox_x11_conn);

		if (platform->globox_x11_software.globox_x11_software_pixmap_update == true)
		{
			xcb_free_pixmap(
				platform->globox_x11_conn,
				platform->globox_x11_software.globox_x11_software_pixmap);

			xcb_void_cookie_t cookie_pixmap =
				xcb_create_pixmap_checked(
					platform->globox_x11_conn,
					24, // force 24bpp instead of geometry->depth
					platform->globox_x11_software.globox_x11_software_pixmap,
					platform->globox_x11_win,
					globox->globox_width,
					globox->globox_height);

			xcb_generic_error_t* error_pixmap =
				xcb_request_check(
					platform->globox_x11_conn,
					cookie_pixmap);

			if (error_pixmap != NULL)
			{
				globox_error_throw(
					globox,
					GLOBOX_ERROR_X11_PIXMAP);
				return;
			}

			platform->globox_x11_software.globox_x11_software_pixmap_update = false;
		}

		xcb_void_cookie_t cookie_image;
		xcb_generic_error_t* error_image;

		if (len_theoric >= len_max)
		{
			uint64_t rows_batch = ((len_max << 2) - len) / (4 * globox->globox_width);

			while (rows_batch <= height2)
			{
				cookie_image = xcb_put_image_checked(
					platform->globox_x11_conn,
					XCB_IMAGE_FORMAT_Z_PIXMAP,
					platform->globox_x11_software.globox_x11_software_pixmap,
					platform->globox_x11_software.globox_x11_software_gfx,
					globox->globox_width,
					rows_batch,
					0,
					y2,
					0,
					24,
					4 * globox->globox_width * rows_batch,
					(void*) (platform->globox_platform_argb + (y2 * globox->globox_width)));

				error_image =
					xcb_request_check(
						platform->globox_x11_conn,
						cookie_image);

				if (error_image != NULL)
				{
					globox_error_throw(
						globox,
						GLOBOX_ERROR_X11_IMAGE);
					return;
				}

				y2 += rows_batch;
				height2 -= rows_batch;
			}
		}

		cookie_image = xcb_put_image_checked(
			platform->globox_x11_conn,
			XCB_IMAGE_FORMAT_Z_PIXMAP,
			platform->globox_x11_software.globox_x11_software_pixmap,
			platform->globox_x11_software.globox_x11_software_gfx,
			globox->globox_width,
			height2,
			0,
			y2,
			0,
			24,
			4 * globox->globox_width * height2,
			(void*) (platform->globox_platform_argb + (y2 * globox->globox_width)));

		error_image =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_image);

		if (error_image != NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_IMAGE);
			return;
		}
	}
	else if (platform->globox_x11_software.globox_x11_software_pixmap_update == true)
	{
		xcb_free_pixmap(
			platform->globox_x11_conn,
			platform->globox_x11_software.globox_x11_software_pixmap);

		xcb_void_cookie_t cookie_pixmap =
			xcb_shm_create_pixmap_checked(
				platform->globox_x11_conn,
				platform->globox_x11_software.globox_x11_software_pixmap,
				platform->globox_x11_win,
				globox->globox_width,
				globox->globox_height,
				24, // force 24bpp instead of geometry->depth
				platform->globox_x11_software.globox_x11_software_shm.shmseg,
				0);

		xcb_generic_error_t* error_pixmap =
			xcb_request_check(
				platform->globox_x11_conn,
				cookie_pixmap);

		if (error_pixmap != NULL)
		{
			globox_error_throw(
				globox,
				GLOBOX_ERROR_X11_PIXMAP);
			return;
		}

		platform->globox_x11_software.globox_x11_software_pixmap_update = false;
	}

	xcb_void_cookie_t cookie_copy =
		xcb_copy_area_checked(
			platform->globox_x11_conn,
			platform->globox_x11_software.globox_x11_software_pixmap,
			platform->globox_x11_win,
			platform->globox_x11_software.globox_x11_software_gfx,
			x,
			y,
			x,
			y,
			width,
			height);

	xcb_generic_error_t* error_copy =
		xcb_request_check(
			platform->globox_x11_conn,
			cookie_copy);

	if (error_copy != NULL)
	{
		globox_error_throw(
			globox,
			GLOBOX_ERROR_X11_COPY);
		return;
	}

	globox_platform_commit(globox);
}

void globox_context_x11_software_reserve(struct globox* globox)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	uint32_t buf_width = platform->globox_x11_software.globox_x11_software_buffer_width;
	uint32_t buf_height = platform->globox_x11_software.globox_x11_software_buffer_height;

	if (platform->globox_x11_software.globox_x11_software_shared_pixmaps == false)
	{
		if ((buf_width * buf_height) < (globox->globox_width * globox->globox_height))
		{
			xcb_generic_error_t* error;

			// screen info
			xcb_randr_get_screen_info_cookie_t screen_cookie;
			xcb_randr_get_screen_info_reply_t* screen_reply;

			screen_cookie =
				xcb_randr_get_screen_info(
					platform->globox_x11_conn,
					platform->globox_x11_win);

			screen_reply =
				xcb_randr_get_screen_info_reply(
					platform->globox_x11_conn,
					screen_cookie,
					&error);

			if (error != NULL)
			{
				free(screen_reply);
				globox_error_throw(
					globox,
					GLOBOX_ERROR_X11_SCREEN_INFO);
				return;
			}

			xcb_window_t root = screen_reply->root;
			free(screen_reply);

			// window geometry info
			xcb_get_geometry_cookie_t win_cookie;
			xcb_get_geometry_reply_t* win_reply;

			win_cookie =
				xcb_get_geometry(
					platform->globox_x11_conn,
					root);

			win_reply =
				xcb_get_geometry_reply(
					platform->globox_x11_conn,
					win_cookie,
					&error);

			if (error != NULL)
			{
				free(win_reply);
				globox_error_throw(
					globox,
					GLOBOX_ERROR_X11_WIN_INFO);
				return;
			}

			platform->globox_x11_software.globox_x11_software_buffer_width =
				(1 + (platform->globox_x11_software.globox_x11_software_buffer_width / win_reply->width))
					* win_reply->width;

			platform->globox_x11_software.globox_x11_software_buffer_height =
				(1 + (platform->globox_x11_software.globox_x11_software_buffer_height / win_reply->height))
					* win_reply->height;

			free(win_reply);
			free(platform->globox_platform_argb);

			platform->globox_platform_argb =
				malloc(
					4
					* platform->globox_x11_software.globox_x11_software_buffer_width
					* platform->globox_x11_software.globox_x11_software_buffer_height);
		}
	}
	else
	{
		if ((buf_width * buf_height) != (globox->globox_width * globox->globox_height))
		{
			xcb_void_cookie_t cookie_shm;
			xcb_generic_error_t* error_shm;

			cookie_shm =
				xcb_shm_detach_checked(
					platform->globox_x11_conn,
					platform->globox_x11_software.globox_x11_software_shm.shmseg);

			error_shm =
				xcb_request_check(
					platform->globox_x11_conn,
					cookie_shm);

			if (error_shm != NULL)
			{
				globox_error_throw(
					globox,
					GLOBOX_ERROR_X11_SHM_DETACH);
				return;
			}

			int error_shmdt =
				shmdt(
					platform->globox_x11_software.globox_x11_software_shm.shmaddr);

			if (error_shmdt == -1)
			{
				globox_error_throw(
					globox,
					GLOBOX_ERROR_X11_SHMDT);
				return;
			}

			shm_create(globox);
		}

		platform->globox_x11_software.globox_x11_software_buffer_width = globox->globox_width;
		platform->globox_x11_software.globox_x11_software_buffer_height = globox->globox_height;
	}

	platform->globox_x11_software.globox_x11_software_pixmap_update = true;
}

void globox_context_x11_software_expose(struct globox* globox, int len)
{
	// alias for readability
	struct globox_platform* platform = &(globox->globox_platform);

	for (int i = 0; i < len; ++i)
	{
		globox_context_x11_software_copy(
			globox,
			platform->globox_x11_expose_queue[(4 * i) + 0],
			platform->globox_x11_expose_queue[(4 * i) + 1],
			platform->globox_x11_expose_queue[(4 * i) + 2],
			platform->globox_x11_expose_queue[(4 * i) + 3]);

		if (globox_error_catch(globox))
		{
			return;
		}
	}
}