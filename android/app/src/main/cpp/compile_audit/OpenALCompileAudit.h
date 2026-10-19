#ifndef WOLFENSTEIN_OPENAL_COMPILE_AUDIT_H
#define WOLFENSTEIN_OPENAL_COMPILE_AUDIT_H

// Declarations used only to let the Android compile audit proceed before an
// actual OpenAL implementation is added. This header is never linked into the
// runnable graphics harness.

typedef char ALchar;
typedef int ALenum;
typedef int ALint;
typedef int ALsizei;
typedef unsigned int ALuint;
typedef void ALvoid;

struct ALCcontext;
struct ALCdevice;

#define AL_FALSE 0
#define AL_TRUE 1
#define AL_NO_ERROR 0
#define AL_POSITION 0x1004
#define AL_VELOCITY 0x1006
#define AL_GAIN 0x100A
#define AL_MAX_GAIN 0x100E
#define AL_LOOPING 0x1007
#define AL_BUFFER 0x1009
#define AL_SOURCE_STATE 0x1010
#define AL_PLAYING 0x1012
#define AL_PAUSED 0x1013
#define AL_REFERENCE_DISTANCE 0x1020
#define AL_FORMAT_MONO8 0x1100
#define AL_FORMAT_MONO16 0x1101
#define AL_FORMAT_STEREO8 0x1102
#define AL_FORMAT_STEREO16 0x1103

#ifdef __cplusplus
extern "C" {
#endif

ALenum alGetError(void);
const ALchar* alGetString(ALenum error);
void alListener3i(ALenum parameter, ALint x, ALint y, ALint z);
void alListenerf(ALenum parameter, float value);
void alDeleteBuffers(ALsizei count, const ALuint* buffers);
void alDeleteSources(ALsizei count, const ALuint* sources);
void alSourcef(ALuint source, ALenum parameter, float value);
void alGetSourcei(ALuint source, ALenum parameter, ALint* value);
void alSourcei(ALuint source, ALenum parameter, ALint value);
void alSource3i(ALuint source, ALenum parameter, ALint x, ALint y, ALint z);
void alSourcePlay(ALuint source);
void alSourceStop(ALuint source);
void alBufferData(ALuint buffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency);
void alGenBuffers(ALsizei count, ALuint* buffers);
void alGenSources(ALsizei count, ALuint* sources);

ALCcontext* alcGetCurrentContext(void);
ALCdevice* alcOpenDevice(const ALchar* deviceName);
ALCcontext* alcCreateContext(ALCdevice* device, const ALint* attributes);
ALint alcMakeContextCurrent(ALCcontext* context);
void alcDestroyContext(ALCcontext* context);
ALint alcCloseDevice(ALCdevice* device);
void alcSuspendContext(ALCcontext* context);
void alcProcessContext(ALCcontext* context);

#ifdef __cplusplus
}
#endif

#endif
