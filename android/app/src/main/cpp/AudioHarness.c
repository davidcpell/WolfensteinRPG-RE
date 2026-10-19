#include <AL/al.h>
#include <AL/alc.h>
#include <SDL.h>
#include <SDL_opengles.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define TONE_FREQUENCY 440.0
#define SAMPLE_RATE 48000
#define TONE_SECONDS 1
#define PI 3.14159265358979323846

static int check_al_error(const char *operation)
{
    ALenum error = alGetError();
    if (error == AL_NO_ERROR) {
        return 1;
    }

    SDL_Log("%s failed with OpenAL error 0x%04x", operation, error);
    return 0;
}

static void play_tone(ALuint source)
{
    alSourceStop(source);
    alSourcePlay(source);
    if (check_al_error("Playing generated tone")) {
        SDL_Log("Generated 440 Hz tone started");
    }
}

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_GLContext gl_context = NULL;
    ALCdevice *device = NULL;
    ALCcontext *al_context = NULL;
    ALuint buffer = 0;
    ALuint source = 0;
    int16_t *samples = NULL;
    const int sample_count = SAMPLE_RATE * TONE_SECONDS;
    int running = 1;
    int tone_was_playing = 0;
    int result = 1;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    window = SDL_CreateWindow(
        "Wolfenstein RPG audio test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        480,
        320,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
    );
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        goto cleanup;
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == NULL) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        goto cleanup;
    }

    device = alcOpenDevice(NULL);
    if (device == NULL) {
        SDL_Log("alcOpenDevice failed");
        goto cleanup;
    }
    SDL_Log("OpenAL device: %s", alcGetString(device, ALC_DEVICE_SPECIFIER));

    al_context = alcCreateContext(device, NULL);
    if (al_context == NULL || !alcMakeContextCurrent(al_context)) {
        SDL_Log("Could not create or activate an OpenAL context");
        goto cleanup;
    }
    SDL_Log("OpenAL vendor: %s", alGetString(AL_VENDOR));
    SDL_Log("OpenAL renderer: %s", alGetString(AL_RENDERER));
    SDL_Log("OpenAL version: %s", alGetString(AL_VERSION));

    samples = (int16_t *)malloc((size_t)sample_count * sizeof(int16_t));
    if (samples == NULL) {
        SDL_Log("Could not allocate generated tone samples");
        goto cleanup;
    }
    for (int index = 0; index < sample_count; ++index) {
        double phase = 2.0 * PI * TONE_FREQUENCY * (double)index / SAMPLE_RATE;
        samples[index] = (int16_t)(sin(phase) * 6553.0);
    }

    alGenBuffers(1, &buffer);
    alBufferData(
        buffer,
        AL_FORMAT_MONO16,
        samples,
        sample_count * (ALsizei)sizeof(int16_t),
        SAMPLE_RATE
    );
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, (ALint)buffer);
    if (!check_al_error("OpenAL tone setup")) {
        goto cleanup;
    }

    result = 0;
    play_tone(source);
    while (running) {
        SDL_Event event;
        ALint source_state = AL_STOPPED;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                play_tone(source);
            }
        }

        alGetSourcei(source, AL_SOURCE_STATE, &source_state);
        if (source_state == AL_PLAYING) {
            tone_was_playing = 1;
            glClearColor(0.05f, 0.45f, 0.12f, 1.0f);
        } else {
            if (tone_was_playing) {
                SDL_Log("Generated tone completed");
                tone_was_playing = 0;
            }
            glClearColor(0.05f, 0.12f, 0.45f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

cleanup:
    if (source != 0) {
        alDeleteSources(1, &source);
    }
    if (buffer != 0) {
        alDeleteBuffers(1, &buffer);
    }
    free(samples);
    alcMakeContextCurrent(NULL);
    if (al_context != NULL) {
        alcDestroyContext(al_context);
    }
    if (device != NULL) {
        alcCloseDevice(device);
    }
    if (gl_context != NULL) {
        SDL_GL_DeleteContext(gl_context);
    }
    if (window != NULL) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return result;
}
