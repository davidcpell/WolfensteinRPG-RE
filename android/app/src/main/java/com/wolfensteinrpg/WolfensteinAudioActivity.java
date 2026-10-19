package com.wolfensteinrpg;

import org.libsdl.app.SDLActivity;

public final class WolfensteinAudioActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "audio_harness" };
    }
}
