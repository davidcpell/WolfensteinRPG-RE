#include <stdexcept>
#include "FileSystem.h"
#include "WindowsStartup.h"
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

void drawView(SDLGL* sdlGL);

int main(int argc, char* args[]) try {
#ifdef _WIN32
    const auto executableDir = FileSystem::executableDirectory();
    // Portable installations do not depend on a working user profile.
    const auto paths = FileSystem::resolveWindowsPaths(executableDir,
        std::filesystem::exists(executableDir / L"portable.txt") ? std::filesystem::path{} : FileSystem::localAppData());
    FileSystem::prepareDirectory(paths.storage);
    const std::string storageDirectory = paths.storage.u8string();
    dir = storageDirectory.c_str();
    const auto ipa = WindowsStartup::selectIpa(paths);
    if (!ipa) return 0; // Cancelling startup is a normal exit.
#endif
    int		UpTime = 0;

    if (UpTime == 0) {
        UpTime = CAppContainer::getInstance()->getTimeMS();
    }
    
    ZipFile zipFile;
#ifdef _WIN32
    zipFile.openZipFile(ipa->u8string().c_str());
#else
    zipFile.openZipFile("Wolfenstein RPG.ipa");
#endif

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


catch (const std::exception& error) {
    fprintf(stderr, "%s\n", error.what());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Wolfenstein RPG filesystem error", error.what(), nullptr);
    return 1;
}

static uint32_t lastTimems = 0;

void drawView(SDLGL *sdlGL) {

    int cx, cy;
    int w = sdlGL->vidWidth;
    int h = sdlGL->vidHeight;

    if (lastTimems == 0) {
        lastTimems = CAppContainer::getInstance()->getTimeMS();
    }

    SDL_GetWindowSize(sdlGL->window, &cx, &cy);
    if (w != cx || h != cy) {
        w = cx; h = cy;
    }

    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
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

    SDL_GL_SwapWindow(sdlGL->window);  // Swap the window/pBmp to display the result.
    
}