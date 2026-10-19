#include "OpenALCompileAudit.h"

ALenum alGetError(void) { return AL_NO_ERROR; }
const ALchar* alGetString(ALenum) { return "OpenAL compile-audit stub"; }
void alListener3i(ALenum, ALint, ALint, ALint) {}
void alListenerf(ALenum, float) {}
void alDeleteBuffers(ALsizei, const ALuint*) {}
void alDeleteSources(ALsizei, const ALuint*) {}
void alSourcef(ALuint, ALenum, float) {}
void alGetSourcei(ALuint, ALenum, ALint* value) { *value = 0; }
void alSourcei(ALuint, ALenum, ALint) {}
void alSource3i(ALuint, ALenum, ALint, ALint, ALint) {}
void alSourcePlay(ALuint) {}
void alSourceStop(ALuint) {}
void alBufferData(ALuint, ALenum, const ALvoid*, ALsizei, ALsizei) {}
void alGenBuffers(ALsizei count, ALuint* buffers) {
    for (ALsizei i = 0; i < count; ++i) buffers[i] = 0;
}
void alGenSources(ALsizei count, ALuint* sources) {
    for (ALsizei i = 0; i < count; ++i) sources[i] = 0;
}

ALCcontext* alcGetCurrentContext(void) { return nullptr; }
ALCdevice* alcOpenDevice(const ALchar*) { return nullptr; }
ALCcontext* alcCreateContext(ALCdevice*, const ALint*) { return nullptr; }
ALint alcMakeContextCurrent(ALCcontext*) { return 1; }
void alcDestroyContext(ALCcontext*) {}
ALint alcCloseDevice(ALCdevice*) { return 1; }
void alcSuspendContext(ALCcontext*) {}
void alcProcessContext(ALCcontext*) {}
