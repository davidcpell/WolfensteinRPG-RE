package com.wolfensteinrpg;

import org.libsdl.app.SDLActivity;

import java.io.File;

public final class WolfensteinGameActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "wolfenstein_engine_audit" };
    }

    @Override
    protected String[] getArguments() {
        File privateIpa = new File(
                new File(getFilesDir(), "game"),
                "Wolfenstein RPG.ipa");
        File saveDirectory = new File(getFilesDir(), "save");
        if (!saveDirectory.isDirectory() && !saveDirectory.mkdirs()) {
            throw new IllegalStateException("Could not create the private save directory");
        }
        return new String[] {
                privateIpa.getAbsolutePath(),
                saveDirectory.getAbsolutePath()
        };
    }
}
