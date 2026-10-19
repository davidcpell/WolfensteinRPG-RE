#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include "SDLGL.h"
#include "ZipFile.h"

#include "CAppContainer.h"
#include "App.h"
#include "Image.h"
#include "Resource.h"
#include "Render.h"
#include "GLES.h"

#include "Canvas.h"
#include "Graphics.h"
#include "Player.h"
#include "Game.h"
#include "Graphics.h"
#include "Utils.h"
#include "TinyGL.h"
#include "Input.h"
#include "JavaStream.h"

void drawView(SDLGL* sdlGL);

int main(int argc, char* args[]) {
    int		UpTime = 0;

#ifdef __ANDROID__
    if (argc < 3) {
        SDL_Log("Game startup requires an IPA path and save directory");
        return 1;
    }
    const char* ipaPath = args[1];
    dir = args[2];
    SDL_Log("Starting engine with IPA: %s", ipaPath);
    SDL_Log("Engine save directory: %s", dir);
#else
    const char* ipaPath = "Wolfenstein RPG.ipa";
#endif

    if (UpTime == 0) {
        UpTime = CAppContainer::getInstance()->getTimeMS();
    }
    
    ZipFile zipFile;
    zipFile.openZipFile(ipaPath);

	SDLGL sdlGL;
	sdlGL.Initialize();

    Input input;
    input.init(); // [GEC] Port: set default Binds

    CAppContainer::getInstance()->Construct(&sdlGL, &zipFile);
    sdlGL.updateVideo(); // [GEC]

    while (CAppContainer::getInstance()->app->closeApplet != true) {
        int currentTimeMillis = CAppContainer::getInstance()->getTimeMS();
        if (currentTimeMillis > UpTime) {
            input.handleEvents();
            UpTime = currentTimeMillis + 15;
            drawView(&sdlGL);
            input.consumeEvents();
        }
    }

    printf("APP_QUIT\n");
    CAppContainer::getInstance()->~CAppContainer();
    zipFile.closeZipFile();
    sdlGL.~SDLGL();
    input.~Input();
	return 0;
}


static uint32_t lastTimems = 0;

void drawView(SDLGL *sdlGL) {

    int viewportX, viewportY, viewportWidth, viewportHeight;

    if (lastTimems == 0) {
        lastTimems = CAppContainer::getInstance()->getTimeMS();
    }

    sdlGL->getDrawableViewport(
        &viewportX, &viewportY, &viewportWidth, &viewportHeight);
    glViewport(
        viewportX, viewportY,
        (GLsizei)viewportWidth, (GLsizei)viewportHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#ifdef __ANDROID__
    glOrthof(0.0f, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0f, -1.0f, 1.0f);
#else
    glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
#endif
    //glRotatef(90.0, 0.0, 0.0, 1.0);
    //glTranslatef(0.0, -320.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    

    uint32_t startTime = CAppContainer::getInstance()->getTimeMS();
    uint32_t passedTime = startTime - lastTimems;
    lastTimems = startTime;

    if (passedTime >= 125) {
        passedTime = 125;
    }
    //printf("passedTime %d\n", passedTime);

    CAppContainer::getInstance()->DoLoop(passedTime);

#ifdef __ANDROID__
    static bool loggedViewport = false;
    if (!loggedViewport) {
        GLint actualViewport[4];
        glGetIntegerv(GL_VIEWPORT, actualViewport);
        SDL_Log(
            "Engine viewport after draw: %d,%d %dx%d (requested %d,%d %dx%d)",
            actualViewport[0], actualViewport[1],
            actualViewport[2], actualViewport[3],
            viewportX, viewportY, viewportWidth, viewportHeight);
        loggedViewport = true;
    }
#endif

    SDL_GL_SwapWindow(sdlGL->window);  // Swap the window/pBmp to display the result.
    
}
