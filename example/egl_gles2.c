#include "globox.h"
#include <GLES2/gl2.h>

#if 0
// we try to avoid ifdefs here by relying on helper files
// helper headers for extensions
#include "example_helper_willis.h"
#include "example_helper_dpishit.h"
#include "example_helper_cursoryx.h"
// helper headers for rendering backends
#include "example_helper_software.h"
#include "example_helper_vulkan.h"
#include "example_helper_egl.h"
#endif

extern unsigned char iconpix_beg;
extern unsigned char iconpix_end;
extern unsigned char iconpix_len;

#define VERTEX_ATTR_POSITION 0

void render(struct globox* globox)
{
	globox_platform_events_handle(
		globox);

	if (globox_error_catch(globox))
	{
		return;
	}

	if (globox_get_redraw(globox) == true)
	{
		int32_t width = globox_get_width(globox);
		int32_t height = globox_get_height(globox);
		GLint viewport_rect[4];

		// we can make OpenGL 1 calls without any loader
		glGetIntegerv(GL_VIEWPORT, viewport_rect);

		if ((viewport_rect[2] != width) || (viewport_rect[3] != height))
		{
			glViewport(0, 0, width, height);
		}

		glClearColor(0.2f, 0.4f, 0.9f, (0x22 / 255.0f));
		glClear(GL_COLOR_BUFFER_BIT);

		GLfloat vertices[] =
		{
			-100.0f / width, +100.0f / height, 1.0f,
			-100.0f / width, -100.0f / height, 1.0f,
			+100.0f / width, -100.0f / height, 1.0f,
			+100.0f / width, +100.0f / height, 1.0f,
		};

		glEnableVertexAttribArray(VERTEX_ATTR_POSITION);

		glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			vertices);

		glDrawArrays(
			GL_TRIANGLE_FAN,
			0,
			4);

		globox_context_egl_copy(
			globox,
			0,
			0,
			width,
			height);
	}
}

int main(void)
{
	struct globox globox = {0};

	globox_open(
		&globox,
		0,
		0,
		500,
		500,
		"globox",
		GLOBOX_STATE_REGULAR,
		NULL,
		NULL);

	if (globox_error_catch(&globox))
	{
		return 1;
	}

	globox_platform_init(&globox, true, false, true);

	if (globox_error_catch(&globox))
	{
		globox_close(&globox);
		return 1;
	}

	// use OpenGL 2 or glES 2
	globox_context_egl_init(&globox, 2, 0);

	if (globox_error_catch(&globox))
	{
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_platform_create_window(&globox);

	if (globox_error_catch(&globox))
	{
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_context_egl_create(&globox);

	if (globox_error_catch(&globox))
	{
		globox_context_egl_free(&globox);
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_platform_hooks(&globox);

	if (globox_error_catch(&globox))
	{
		globox_context_egl_free(&globox);
		globox_platform_free(&globox);
		globox_close(&globox);
		return 1;
	}

	globox_platform_set_icon(
		&globox,
		(uint32_t*) &iconpix_beg,
		2 + (16 * 16) + 2 + (32 * 32) + 2 + (64 * 64));

	// prepare OpenGL or glES
	const char* vertex_shader_src =
		"attribute vec4 vPosition;"
		"void main()"
		"{"
		"	gl_Position = vPosition;"
		"}";

	const char* fragment_shader_src =
		"precision mediump float;"
		"void main()"
		"{"
		"	gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);"
		"}";

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, 0);
	glCompileShader(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, 0);
	glCompileShader(fragment_shader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(program);
	glUseProgram(program);

	// continue initializing globox
	globox_platform_commit(&globox);

	render(&globox);

	while (globox_get_closed(&globox) == false)
	{
		globox_platform_prepoll(&globox);

		if (globox_error_catch(&globox))
		{
			globox_context_egl_free(&globox);
			globox_platform_free(&globox);
			globox_close(&globox);
			return 1;
		}

		globox_platform_events_wait(&globox); // TODO compatible with windows bullshit?

		if (globox_error_catch(&globox))
		{
			globox_context_egl_free(&globox);
			globox_platform_free(&globox);
			globox_close(&globox);
			return 1;
		}

		render(&globox);

		if (globox_error_catch(&globox))
		{
			globox_context_egl_free(&globox);
			globox_platform_free(&globox);
			globox_close(&globox);
			return 1;
		}
	}

	globox_context_egl_free(&globox);
	globox_platform_free(&globox);
	globox_close(&globox);

	return 0;
}
