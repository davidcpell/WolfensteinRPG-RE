package com.wolfensteinrpg;

import org.libsdl.app.SDLActivity;

import java.io.File;

public final class WolfensteinSdlActivity extends SDLActivity {
    @Override
    protected String[] getArguments() {
        File privateIpa = new File(
                new File(getFilesDir(), "game"),
                "Wolfenstein RPG.ipa");
        return new String[] { privateIpa.getAbsolutePath() };
    }
}
