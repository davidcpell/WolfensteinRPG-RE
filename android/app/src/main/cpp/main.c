#include <SDL.h>
#include <SDL_opengles.h>

int main(int argc, char* argv[])
{
	SDL_Window *window;
	SDL_GLContext context;
	SDL_Event event;
	int drawable_width;
	int drawable_height;
	GLfloat aspect_ratio;
	GLuint checker_texture;
	GLubyte checker_pixels[4 * 4 * 4];
	int checker_x;
	int checker_y;
	int pixel_index;
	int light_square;
	int running = 1;
	const GLfloat quad_vertices[] = {
		-1.2f, -0.7f,
		 1.2f, -0.7f,
		-1.2f,  0.7f,
		 1.2f,  0.7f
	};
	const GLfloat quad_texture_coordinates[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};

	(void)argc;
	(void)argv;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		SDL_Log("SDL_Init failed: %s", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(
		SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_ES
	);

	window = SDL_CreateWindow(
		"Wolfenstein RPG Android Test",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		480,
		320,
		SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
	);

	if (window == NULL) {
		SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	context = SDL_GL_CreateContext(window);

	if (context == NULL) {
		SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);

	SDL_Log(
		"OpenGL vendor: %s",
		(const char*)glGetString(GL_VENDOR)
	);
	SDL_Log(
		"OpenGL renderer: %s",
		(const char*)glGetString(GL_RENDERER)
	);
	SDL_Log(
		"OpenGL version: %s",
		(const char*)glGetString(GL_VERSION)
	);
	SDL_Log(
		"Drawable size: %d x %d",
		drawable_width,
		drawable_height
	);

	for (checker_y = 0; checker_y < 4; ++checker_y) {
		for (checker_x = 0; checker_x < 4; ++checker_x) {
			pixel_index = (checker_y * 4 + checker_x) * 4;
			light_square = ((checker_x + checker_y) % 2 == 0);

			checker_pixels[pixel_index + 0] = light_square ? 240 : 32;
			checker_pixels[pixel_index + 1] = light_square ? 240 : 96;
			checker_pixels[pixel_index + 2] = light_square ? 240 : 192;
			checker_pixels[pixel_index + 3] = 255;
		}
	}

	glGenTextures(1, &checker_texture);
	glBindTexture(GL_TEXTURE_2D, checker_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		4,
		4,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		checker_pixels
	);

	SDL_GL_SetSwapInterval(1);

	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = 0;
			}
		}

		SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
		glViewport(0, 0, drawable_width, drawable_height);
		aspect_ratio = (GLfloat)drawable_width / (GLfloat)drawable_height;

		glClearColor(0.06f, 0.08f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, checker_texture);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0, quad_vertices);
		glTexCoordPointer(2, GL_FLOAT, 0, quad_texture_coordinates);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_2D);

		SDL_GL_SwapWindow(window);
	}

	glDeleteTextures(1, &checker_texture);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
