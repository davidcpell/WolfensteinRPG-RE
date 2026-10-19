
#include <stdexcept>

#include "SDLGL.h"
#include "App.h"


SDLResVidModes sdlResVideoModes[18] = {
	{480, 320},
	{640, 480},
	{720, 400},
	{720, 480},
	{720, 576},
	{800, 600},
	{832, 624},
	{960, 640},
	{1024, 768},
	{1152, 864},
	{1152, 872},
	{1280, 720},
	{1280, 800},
	{1280, 1024},
	{1440, 900},
	{1600, 1000},
	{1680, 1050},
	{1920, 1080}
};

SDLGL::SDLGL() {
	this->initialized = false;
}

SDLGL::~SDLGL() {
	if (this) {
		if (this->initialized) {
			if (this->window) {
				SDL_SetWindowFullscreen(this->window, 0);
				if (this->glcontext) {
					SDL_GL_DeleteContext(this->glcontext);
					this->glcontext = nullptr;
				}
				SDL_DestroyWindow(this->window);
				this->window = nullptr;
			}

#ifdef __linux__
			if (this->gpSdlWindowIcon) {
				SDL_FreeSurface(this->gpSdlWindowIcon);
				this->gpSdlWindowIcon = nullptr;
			}
#endif
			SDL_Quit();
		}
	}
}

bool SDLGL::Initialize() {
	Uint32 flags;

	if (!this->initialized) {
		
		SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			printf("Could not initialize SDL: %s", SDL_GetError());
		}

#ifdef __ANDROID__
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

		flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /* | SDL_WINDOW_RESIZABLE*/;
#ifdef __ANDROID__
		flags |= SDL_WINDOW_FULLSCREEN;
#endif
		// Set the highdpi flags - this makes a big difference on Macs with
		// retina displays, especially when using small window sizes.
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;

		this->oldResolutionIndex = -1;
		this->resolutionIndex = 0;
		this->winVidWidth = sdlResVideoModes[this->resolutionIndex].width;//Applet::IOS_WIDTH*2;
		this->winVidHeight = sdlResVideoModes[this->resolutionIndex].height;//Applet::IOS_HEIGHT*2;

		//this->winVidWidth = 1440;
		//this->winVidHeight = 900;

		// Linux: create the icon for the window
#ifdef __linux__
		this->gpSdlWindowIcon = SDL_CreateRGBSurfaceWithFormatFrom((void*)Applet::gIcon_64_raw_rgb888, 64, 64, 24, 64 * 3, SDL_PIXELFORMAT_RGB24);
#endif

		this->window = SDL_CreateWindow("Wolfenstein RPG By [GEC] Version 0.1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, winVidWidth, winVidHeight, flags);
		if (!this->window) {
			printf("Could not set %dx%d video mode: %s", winVidWidth, winVidHeight, SDL_GetError());
		}

		this->vidWidth = Applet::IOS_WIDTH;
		this->vidHeight = Applet::IOS_HEIGHT;
		this->oldWindowMode = -1;
		this->windowMode = 0;
		this->oldVSync = false;
		this->vSync = true;

#ifdef __ANDROID__
		this->windowMode = 2;
		this->oldWindowMode = this->windowMode;
		this->oldResolutionIndex = this->resolutionIndex;
#endif

		//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
		//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");//sdlVideo.vSync ? "1" : "0");
		this->updateVideo();

		// Linux: set the icon for the window
#ifdef __linux__
		if (this->gpSdlWindowIcon) {
			SDL_SetWindowIcon(this->window, this->gpSdlWindowIcon);
		}
#endif

		this->glcontext = SDL_GL_CreateContext(window);

		// now you can make GL calls.
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(this->window);

#ifdef _WIN32
		// Check for joysticks
		SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
#endif
		this->initialized = true;
	}

	return this->initialized;
}


#include <stdarg.h> //va_list|va_start|va_end

void SDLGL::Error(const char* fmt, ...)
{
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
	va_end(ap);

	printf("%s", errMsg);

	const SDL_MessageBoxButtonData buttons[] = {
		{ /* .flags, .buttonid, .text */        0, 0, "Ok" },
	};
	const SDL_MessageBoxColorScheme colorScheme = {
		{ /* .colors (.r, .g, .b) */
			/* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
			{ 255,   0,   0 },
			/* [SDL_MESSAGEBOX_COLOR_TEXT] */
			{   0, 255,   0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
			{ 255, 255,   0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
			{   0,   0, 255 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
			{ 255,   0, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR, /* .flags */
		NULL, /* .window */
		"Doom II RPG Error", /* .title */
		errMsg, /* .message */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		&colorScheme /* .colorScheme */
	};

	SDL_ShowMessageBox(&messageboxdata, NULL);
	//closeZipFile(&zipFile);
	//DoomRPG_FreeAppData(doomRpg);
	//SDL_CloseAudio();
	//SDL_Close();
	this->~SDLGL();
	exit(0);
}


void SDLGL::transformCoord2f(float* x, float* y) {
	int w, h;
	SDL_GetWindowSize(this->window, &w, &h);
	*x *= (float)w / (float)Applet::IOS_WIDTH;
	*y *= (float)h / (float)Applet::IOS_HEIGHT;
}

void SDLGL::getDrawableViewport(int* x, int* y, int* width, int* height) {
	int drawableWidth;
	int drawableHeight;
	SDL_GL_GetDrawableSize(this->window, &drawableWidth, &drawableHeight);

	*x = 0;
	*y = 0;
	*width = drawableWidth;
	*height = drawableHeight;

#ifdef __ANDROID__
	if (drawableWidth * Applet::IOS_HEIGHT > drawableHeight * Applet::IOS_WIDTH) {
		*width = drawableHeight * Applet::IOS_WIDTH / Applet::IOS_HEIGHT;
		*x = (drawableWidth - *width) / 2;
	}
	else {
		*height = drawableWidth * Applet::IOS_HEIGHT / Applet::IOS_WIDTH;
		*y = (drawableHeight - *height) / 2;
	}
#endif
}

void SDLGL::windowToGameCoordinates(
	int windowX, int windowY, float* gameX, float* gameY) {
	int windowWidth;
	int windowHeight;
	int drawableWidth;
	int drawableHeight;
	int viewportX;
	int viewportY;
	int viewportWidth;
	int viewportHeight;

	SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);
	SDL_GL_GetDrawableSize(this->window, &drawableWidth, &drawableHeight);
	getDrawableViewport(&viewportX, &viewportY, &viewportWidth, &viewportHeight);

	float viewportWindowX = (float)viewportX * windowWidth / drawableWidth;
	float viewportWindowY = (float)viewportY * windowHeight / drawableHeight;
	float viewportWindowWidth = (float)viewportWidth * windowWidth / drawableWidth;
	float viewportWindowHeight = (float)viewportHeight * windowHeight / drawableHeight;

	*gameX = (windowX - viewportWindowX) / viewportWindowWidth;
	*gameY = (windowY - viewportWindowY) / viewportWindowHeight;
	if (*gameX < 0.0f) *gameX = 0.0f;
	if (*gameX > 1.0f) *gameX = 1.0f;
	if (*gameY < 0.0f) *gameY = 0.0f;
	if (*gameY > 1.0f) *gameY = 1.0f;
}

void SDLGL::centerMouse(int x, int y) {
	float X = (float)((Applet::IOS_WIDTH / 2) + x);
	float Y = (float)((Applet::IOS_HEIGHT / 2) + y);
	this->transformCoord2f(&X, &Y);
	SDL_WarpMouseInWindow(this->window, (int)X, (int)Y);
}

void SDLGL::updateWinVid(int w, int h) {
	this->winVidWidth = w;
	this->winVidHeight = h;
}

void SDLGL::updateVideo() {
	if (this->vSync != this->oldVSync) {
		SDL_GL_SetSwapInterval(this->vSync ? 1 : 0);
		this->oldVSync = this->vSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		SDL_SetWindowFullscreen(this->window, 0);
		SDL_SetWindowBordered(this->window, SDL_TRUE);

		if (this->windowMode == 1) {
			SDL_SetWindowBordered(this->window, SDL_FALSE);
		}
		else if (this->windowMode == 2) {
			SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
		}
		this->oldWindowMode = this->windowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		SDL_SetWindowSize(this->window, sdlResVideoModes[this->resolutionIndex].width, sdlResVideoModes[this->resolutionIndex].height);
		SDL_SetWindowPosition(this->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		this->updateWinVid(sdlResVideoModes[this->resolutionIndex].width, sdlResVideoModes[this->resolutionIndex].height);
		this->oldResolutionIndex = this->resolutionIndex;
	}

	int winVidWidth, winVidHeight;
	SDL_GetWindowSize(this->window, &winVidWidth, &winVidHeight);
	this->updateWinVid(winVidWidth, winVidHeight);
}

void SDLGL::restore() {
	if (this->vSync != this->oldVSync) {
		this->vSync = this->oldVSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		this->windowMode = this->oldWindowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		this->resolutionIndex = this->oldResolutionIndex;
	}
}
