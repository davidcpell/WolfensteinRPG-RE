# Native Android roadmap

## Purpose and working agreement

Port this reverse-engineered C++ game to a normal APK running on an unrooted Android phone. The APK must not bundle the original IPA or extracted proprietary game resources, and must not execute the original iPhone binary or use J2ME emulation.

This is a plan, not authorization to implement the whole port at once. Work in small chunks with a visible result and a learning checkpoint. Agents can drive implementation, but each chunk should:

1. Explain one goal and the concepts needed to understand it.
2. Make a small, reviewable change.
3. Demonstrate the result and explain what the test proves.
4. Walk through the important code and distinguish C concepts from C++ features.
5. Pause before starting the next chunk.

The developer is experienced in software development, is learning C, and is new to game and Android development. Explain unfamiliar terms when they become relevant rather than front-loading the entire graphics and Android stack. This repository is primarily C++.

## Agreed initial target

- Phone: Pixel 9 Pro XL, Android 17.
- Input: touchscreen is the primary target and the intended way to play on the phone.
- Optional controller experiment: check whether Android and SDL detect the GameSir Cyclone 2 over USB-C. Controller gameplay is not an MVP prerequisite.
- Importer: support one known-good Wolfenstein RPG IPA layout first.
- Use an Android emulator for routine development, with physical-phone checks at specific milestones.
- Keep game logic unchanged where possible, reuse SDL2, and avoid a speculative renderer rewrite.

## Current state as of 2026-10-18

### Completed and verified

- Installed and verified Android Studio Rabbit 1, Android SDK/API 37, Build Tools 36.0.0, NDK 30.0.16248370, CMake 4.1.2 and the Android emulator.
- Created and booted the x86-64 `Medium_Phone_API_37.0` Android 17 virtual device.
- Added a minimal Android application using Android Gradle Plugin 9.4.0 and Gradle 9.6.0.
- Added SDL 2.32.10 as a pinned Git submodule and compiled its matching native and Java Android integration.
- Built the native harness for both `x86_64` (emulator) and `arm64-v8a` (Pixel).
- Built, installed and repeatedly launched a debug APK on the Android 17 emulator.
- Created an OpenGL ES 1.1 Common Profile context. The emulator reported `OpenGL ES-CM 1.1`, using Android's OpenGL ES translator backed by the host RTX 5080.
- Confirmed a 2400 by 1080 landscape drawable. The emulator's desktop frame can remain portrait while rotating the landscape framebuffer for presentation.
- Rendered a solid-color frame, a smoothly interpolated vertex-colored triangle, and a generated 4 by 4 RGBA checkerboard texture.
- Verified GLES 1.1 matrix operations, `glOrthof`, vertex/color/texture-coordinate client arrays, texture upload, nearest filtering and `glDrawArrays`.
- Corrected the activity configuration to handle SDL display changes without starting a second native window. The final checkerboard build remained alive after the display-change interval that previously exposed the issue.
- Installed and launched the ARM64 debug APK on the Pixel 9 Pro XL running Android 17. The checkerboard rendered correctly in a 2244 by 1008 landscape drawable.
- The Pixel reported OpenGL vendor `ARM`, renderer `Mali-G715` and `OpenGL ES-CM 1.1` from the production phone driver.
- Verified a Home/resume cycle: Android destroyed and recreated the SDL surface, the same process survived, and rendering recovered without a crash or ANR.
- Rotated between both landscape orientations. SDL received the surface changes and the checkerboard remained correct.
- Verified the existing renderer's alpha-test pattern on the Pixel: `glEnable(GL_ALPHA_TEST)` with `glAlphaFunc(GL_GREATER, 0.0f)` discarded zero-alpha texels, retained opaque texels and produced no GL errors.
- Verified the existing renderer's normal alpha-blending factors on the Pixel: `GL_SRC_ALPHA` and `GL_ONE_MINUS_SRC_ALPHA` mixed half-alpha white texels with the dark background while fully opaque blue texels remained unchanged. Blend setup and drawing produced no GL errors.
- Verified the renderer's linear fixed-function fog calls on the Pixel. A textured quad spanning the configured eye-space depth range retained its original checker colors at the near edge and converged smoothly to the fog color at the far edge, with no GL errors.
- Verified the `TexCombineShift` operation on the Pixel. The GLES headers require `GL_SRC0_*`/`GL_SRC1_*` parameter names instead of the desktop `GL_SOURCE0_*`/`GL_SOURCE1_*` names. Both `GL_TEXTURE0` and the portable GLES 1.1 `GL_TEXTURE` source worked on the Mali driver; the harness retains `GL_TEXTURE`. Adding a constant red tint produced the expected clamped RGB values while preserving alpha, with no GL errors.
- Compiled every engine source file and the bundled hash library for Android ARM64 with Android Clang. The audit identified and corrected Android GLES header selection, direct GLES function declarations, `glOrtho` calls, texture-combiner names, missing standard `<algorithm>` includes and signed color-case constants.
- Linked the compiled engine into a separate `libwolfenstein_engine_audit.so` alongside the unchanged graphics harness. The temporary no-op OpenAL shim used for the original link audit has now been replaced by OpenAL Soft.
- Installed the linked APK on the Pixel and verified that Android loaded the engine-library dependency while the known-good texture-combiner harness continued to render with no GL or Java runtime errors.
- Rebuilt the normal Visual Studio Debug target successfully after the shared-source portability changes. Its existing warnings remain, with no build errors.
- Added a Storage Access Framework document-picker screen and imported the known-good IPA into persistent app-private storage. The 34,454,149-byte private copy matched the source and remained present after a forced stop and restart.
- Added validation for the known-good `Payload/Wolf...RPG.app/Packages/` layout before atomic replacement. A deliberately incomplete ZIP was rejected for its missing required entry, while the existing valid private IPA remained unchanged.
- Passed the private IPA pathname through the custom SDL activity into native `SDL_main`; the native harness opened the exact app-private path successfully.
- Exercised the project's existing native `ZipFile` reader on the Pixel. It decompressed and checksummed `logo.bmp`, `strings.idx`, `tables.bin` and `sounds/Alarm.wav`, then continued into the GLES harness without errors.
- Added a separate game activity while retaining the graphics harness as a diagnostic fallback. The engine receives the app-private IPA and save-directory paths explicitly; normal game startup now uses the real Android audio implementation.
- Started the real engine on the Pixel from the imported IPA. It rendered the intro, advanced to the title/main menu through touch, and opened the Options and New Game difficulty menus without a native crash or missing-resource failure.
- Corrected Android window setup to retain the full 2244 by 1008 drawable and render the original 480 by 320 view in a centered 1512 by 1008 viewport. Input coordinates now invert that letterbox transform before reaching the existing game touch handlers.
- Added Android 17 immersive-mode handling to the game activity. The Pixel now hides the status and navigation bars when the game gains focus, and the briefing's top `Skip` and bottom `More` controls are fully visible.
- Entered the first level on the Pixel and verified textured 3D world geometry, sprites, the weapon view, HUD, transparency and the previously tested fixed-function effects in the combined game renderer.
- Verified the latent touchscreen path in the real game. Touch advanced the intro, menus, dialogue and multi-page help; an upward swipe moved the player forward one grid square using the existing gameplay controls.
- Verified the first touchscreen combat encounter on the Pixel: the player fought and killed the first enemy successfully.
- Fixed intermittent first-tap failures in the main and in-game menus. SDL's Android mouse emulation queued a cursor-motion event immediately before finger-down; the input adapter read the later global button state and misclassified that earlier hover as a drag, so the following release was consumed by menu scrolling. Motion now uses the button state captured on its own SDL event. Five repeated main-menu Options transitions plus in-game Save Game and Load Game transitions all responded to single taps on the Pixel.
- Verified private persistence across an APK update and process restart. The main menu offered `Continue`, and the saved Tower game restored into its level intro and playable scene.
- Verified the real engine's basic Android lifecycle on the Pixel. Home/reopen and lock/unlock returned to a correctly rendered scene with working touch movement. After a force-stop, relaunching through `Continue` restored the Tower level; all five configuration/player/world save files retained their exact sizes and SHA-256 contents across the restart.
- Fixed an ARM signed-`char` portability crash in queued help dialogs. The `-1` no-script sentinel had become 255 on Android and indexed beyond the script-thread array. The queue now uses an explicit signed 8-bit index; the pistol and journal dialogs close successfully on the Pixel with no recurrence of the native crash.
- Added OpenAL Soft 1.25.2 as a pinned submodule and static Android dependency using its OpenSL ES backend. An isolated harness played a generated 440 Hz PCM tone through the Pixel speaker and replayed it on touch without OpenAL errors.
- Replaced the engine's silent OpenAL shim with OpenAL Soft. A controlled engine probe loaded `Music_Title.wav` and `Alarm.wav` through the existing IPA/WAV path, looped the title music and played the alarm over it; both sources completed or continued independently with no logged errors.
- Verified normal game audio on the Pixel across intro effects, doors, weapons, enemies, fire ambience, destructible objects and switches. Music and sound-effect volume controls mute their categories independently. Audio stops and resumes cleanly across both Home/reopen and lock/unlock without doubling, distortion or disappearing.
- The API 37 x86-64 emulator's GLES1 encoder is not reliable for alpha-function validation: both `glAlphaFunc` and `glAlphaFuncx` returned `GL_INVALID_OPERATION`, and querying `glIsEnabled(GL_ALPHA_TEST)` crashed inside the emulator's GLES1 encoder. Treat the Pixel as authoritative for the remaining fixed-function state experiments.

### Deferred or not started

- The GameSir Cyclone 2 has not been tested over USB-C.
- Inventory and complicated minigame touch interactions have not yet been verified on Android; broader combat coverage remains for later playtesting.
- A basic saved game survives an APK update and reloads through `Continue`, but save integrity during interruption and longer save/load testing remain unfinished.
- External audio-focus interruptions and output-route changes have not yet been tested. Repeated lifecycle stress, graphics-context loss and interruption during an active save also remain unfinished.
- The APK contains the importer, diagnostic harness and engine, but does not contain proprietary game resources.

### Next recommended checkpoint

Smoke-test the inventory and item interaction screens with touch as the next bounded input checkpoint. Retain the interrupted `CONFIGFILENAME` write as a durability research note for later hardening; it is a reproduced edge case, but it does not block the next functional checkpoint.

An IPA is an iPhone/iPad application archive, roughly equivalent to an Android APK. This port reads graphics, maps, sounds and other resources from the user's IPA; it runs the rebuilt C++ engine rather than the executable inside that archive.

## Evidence and confidence

Original source-review baseline: local `davidcpell/WolfensteinRPG-RE` commit `22a8e7e` (Add Windows build support). That initial review did not compile for Android or test an Android device. The Android work recorded above supersedes that limitation through real silent first-level rendering, saved-game restoration and basic touch movement on the Pixel.

### Confirmed from source

- CMake currently builds a desktop executable using SDL2, desktop OpenGL, OpenAL, zlib and the bundled hash library.
- The local Windows SDL headers at review time identify version 2.32.10. The desktop build does not impose a minimum SDL version in `find_package`; the Android harness now pins SDL 2.32.10 as a submodule.
- `Main.cpp` opens `Wolfenstein RPG.ipa` by relative filename before initializing SDL.
- `ZipFile` uses seekable `FILE*` access through `fopen`, `fseek` and `ftell`.
- `JavaStream.cpp` already reads individual resources from the archive into memory. It assumes `Payload/Wolf...RPG.app/Packages/` and a `sounds/` subdirectory.
- Saves use the relative directory `WolfensteinRPG.app`; directory creation shells out to `mkdir`.
- The `gles` class is the main accelerated 3D rendering backend. `Render` and `TinyGL` call it, and TinyGL also contains software rendering paths.
- `GLES.cpp` retains disabled iPhone viewport/projection code and a commented `glOrthof` call. These are useful historical clues, not a ready-made Android switch.
- The active renderer uses fixed-function graphics: matrix operations, vertex arrays, fog, alpha testing, blending and texture combiners. No particular GL version/profile is explicitly requested.
- Direct GL calls also exist in `Graphics.cpp`, `Image.cpp`, `Main.cpp` and the software-buffer display path in `Render.cpp`.
- `gles::SwapBuffers()` is empty; `Main.cpp` actually presents the frame with `SDL_GL_SwapWindow()`.
- There are no explicit SDL finger-event handlers, but mouse events already flow through `CAppContainer::userPressed/userMoved/userReleased` into `Canvas` touch handling.
- Touch UI includes menus, dialogs, swipes, combat, driving, syringe mixing and card games. The logical display is 480 by 320.
- SDL controller handling exists, but the default action bindings are keyboard-only.
- Audio uses ordinary OpenAL sources/buffers and a local PCM WAV reader. Apple-like audio structures are locally defined rather than dependencies on Apple frameworks.
- `Sound` has suspend/resume methods, but the corresponding `CAppContainer` methods are empty.

### Strong expectations, not yet demonstrated

- Adapting the existing renderer to OpenGL ES 1.1 should be smaller than converting it to shaders.
- Copying the IPA privately should preserve most existing resource-loading code.
- The existing mouse-to-touch path is sufficient for basic gameplay gestures; complete interaction coverage may still expose targeted fixes.
- OpenAL Soft provides working generated, imported and simultaneous game audio on the Pixel; external focus and route-change behavior remain to be demonstrated.

### Requires experiments

- Whether remaining combined rendering states match the desktop during longer levels and effects-heavy scenes.
- Additional ARM runtime issues during longer play, combat and minigames.
- Cyclone 2 wired detection and basic button/axis reporting on the Pixel.
- Complete touch coverage of gameplay and minigames; optional controller mappings can be evaluated afterward where they are useful.
- Audio interruptions, output changes, background/resume and graphics-resource recovery.

## Proposed architecture

```text
Android APK
  small Android bootstrap: document picker, import UI, audio focus
  SDLActivity: native startup, window, input and lifecycle integration
  NDK-built C++ shared library
    existing game logic and UI
    existing gles backend adapted to GLES 1.1, if tests pass
    OpenAL Soft
    ZIP/zlib resource reader
    explicit private paths for imported data, saves and settings
```

The NDK is Android's toolchain for compiling C and C++. A shared library is the compiled native code loaded by the Android application. SDLActivity is SDL's Android application component, which supplies much of the platform glue.

Use the Android template and native SDL libraries from the same pinned SDL2 release. Keep the desktop executable target and add an Android shared-library target. An SDL3 migration is outside the initial scope.

### Resource import

Use Android's system document picker, called the Storage Access Framework. It gives access to a selected document without requesting access to all user files.

1. Show an import screen that requires no game resources.
2. Let the user choose their IPA.
3. Stream it to a temporary file in persistent app-private storage.
4. Validate archive structure, supported layout and required resources.
5. Atomically install the validated copy, preserving any previous valid copy on failure.
6. Pass the actual private path into native startup.

Prefer copying over retaining access to the original document. The current ZIP reader needs random access, while a document provider may supply a stream and may later become unavailable. Copying costs extra storage but keeps provider behavior outside gameplay.

Do not extract the entire archive for the MVP. Read and decompress individual resources using the existing interface. The original executable remains unused. Keep imported game data separate from saves/settings, and avoid backing up the IPA automatically.

Use persistent storage, not cache. App updates should preserve data; uninstalling normally removes private data. Save export/import would be a separate feature for surviving uninstall/reinstall.

### Rendering

The useful existing boundary is:

```text
game logic -> Render/TinyGL -> gles backend -> graphics API
                   plus some direct 2D/UI graphics calls
```

OpenGL ES, or GLES, is the graphics API used here on Android. Fixed-function means the API supplies built-in transformations and effects. A shader is a small program that supplies those operations explicitly on newer graphics APIs.

First test GLES 1.1, which resembles the existing fixed-function code. Likely adaptations include GLES1 headers, an explicit ES 1.1 context request, linking `GLESv1_CM`, replacing `glOrtho` with `glOrthof`, and correcting desktop-specific declarations and texture-combiner calls.

In particular, `TexCombineShift` uses `GL_TEXTURE0` as a combiner source. GLES 1.1 omits desktop texture crossbar functionality; the active texture source should be adapted appropriately to `GL_TEXTURE`. Check texture sizes, wrapping and upload alignment too.

TinyGL is not a general OpenGL compatibility library. Its software fallback does not remove direct GL dependencies in UI and screen presentation.

If real tests demonstrate fundamental GLES1 problems, implement a bounded GLES2 backend covering the existing operations. That changes rendering, not gameplay. Do not plan a shader rewrite as a prerequisite.

### Input and audio

Touch is the primary input method. First wire SDL finger events into the existing touch interface. Preserve starting/current pointer coordinates because `Canvas` and `MenuSystem` currently consult mouse globals. Use actual release coordinates, handle cancelled gestures, and prevent duplicate synthetic mouse/finger actions.

Test whether Android and SDL detect the GameSir over USB-C and record its buttons and axes. Treat controller mappings as an optional enhancement after the touch path works, unless touch integration proves substantially harder than the source suggests. Controller detection alone does not establish controller playability.

Use upstream OpenAL Soft, initially evaluating its OpenSL ES backend; Oboe is an alternative if needed. Add a deliberate silent mode, safe initialization failure, and background/audio-focus handling. Audio focus means responding when Android gives another application priority to play sound.

## Emulator and phone testing

An emulator runs a virtual Android device on the development computer. Use it for frequent development iterations, but do not treat it as a complete simulation of Pixel hardware.

| Test | Normal test environment |
| --- | --- |
| Build, install and launch | Emulator |
| Document picker, import errors and resource reads | Emulator |
| Save/load and restart | Emulator, then phone checkpoint |
| Menus and basic touch gestures | Emulator, then phone usability checks |
| Drawing and graphics effects | Emulator where supported; Pixel is authoritative for GLES1 driver behavior |
| Background/resume and process restart | Both |
| Cyclone 2 USB-C connection | Pixel |
| Performance, heat, battery and touch comfort | Pixel |
| Audio routing and interruptions | Pixel, after basic emulator playback |

For an Intel/AMD development computer, normally build x86-64 native code for the emulator and ARM64 code for the Pixel. These are processor families: the same source is compiled twice, not maintained as separate ports. Confirm the development machine architecture before choosing the emulator image. The release APK can remain ARM64-only.

The emulator uses different graphics hardware or software rendering. Both success and failure need interpretation before making renderer decisions. Android also documents USB/Bluetooth hardware limitations; use the phone to establish controller connectivity.

## Roadmap with validation gates

Each milestone may contain several small learning chunks. Stop at each agreed chunk boundary rather than treating this document as one large implementation task.

### 0. Establish a desktop baseline

- **Objective:** Record correct behavior before changing platform code.
- **Files/subsystems:** CMake, `Main`, rendering, input, streams, audio, configuration.
- **Expected changes:** Initially evidence only: dependency versions, screenshots and a repeatable test route.
- **Unknowns/risks:** Existing desktop defects may otherwise look like Android regressions.
- **Success:** Known-good title/menu, level entry, combat and save/reload at a recorded revision.
- **Architecture consequence of failure:** Usually none; document or fix baseline defects separately.

### 1. Launch a minimal Android app

- **Status:** Complete on the emulator and Pixel, including textured rendering, Home/resume and landscape-orientation changes.
- **Objective:** Run native code and display a solid color.
- **Files/subsystems:** New minimal Android wrapper, SDL Android template and isolated graphics harness.
- **Expected changes:** Pin build tools/SDL, add emulator and phone build targets, request a GLES1 drawing context, create a landscape surface.
- **Unknowns/risks:** Tool installation, virtual-device configuration, library loading and context support.
- **Success:** App installs and repeatedly clears/presents a solid-color screen in the emulator; then the same experiment runs on the Pixel. Log the actual graphics implementation.
- **Architecture consequence of failure:** Fix setup first; a demonstrated graphics limitation informs the next experiment.
- **Learning focus:** How compiled C/C++ becomes part of an APK, and how a frame reaches the screen.

### 2. Test the game's graphics features in isolation

- **Status:** Complete for the isolated operations currently known to be required. Basic matrices, client arrays, texture upload/filtering and textured drawing work on both the emulator and Pixel. Alpha testing, normal source-alpha blending, linear fog and constant-color texture combining work on the Pixel. The required GLES combiner-name adaptations and emulator limitation are documented above.
- **Objective:** Test GLES1 viability before substantial port work.
- **Files/subsystems:** Small graphics harness informed by `GLES`, `Image` and `Graphics`.
- **Expected changes:** Draw synthetic textured shapes using the game's texture formats, alpha testing, fog and corrected tint combiner.
- **Unknowns/risks:** Unsupported operations, driver differences, texture restrictions.
- **Success:** Expected images and no graphics API errors on emulator and Pixel; compare the effects individually.
- **Architecture consequence of failure:** Correct isolated differences first; fundamental limitations justify a small GLES2 backend.
- **Learning focus:** Vertices, textures and graphics state, introduced one effect at a time.

### 3. Compile and link the existing game for Android

- **Status:** Complete for the compile/link/load audit. Every engine and hash-library translation unit compiles for Android ARM64; the resulting library links and loads on the Pixel, and the temporary OpenAL audit shim has been replaced by OpenAL Soft.
- **Objective:** Produce a loadable native game library before full game startup.
- **Files/subsystems:** Root/src CMake files, SDL finder, hash-library target, platform headers and affected translation units.
- **Expected changes:** Add an Android shared target while retaining desktop builds; use Android-built dependencies; exclude Windows resources; fix demonstrated portability defects.
- **Unknowns/risks:** Missing includes, signed/unsigned conversions, dependency discovery and ARM assumptions.
- **Success:** Compile every game and bundled-library source file to object files; then link/load the library without unresolved symbols. Desktop still builds.
- **Architecture consequence of failure:** Normally local fixes; widespread binary-layout dependencies could require broader portability work.
- **Learning focus:** Compilation versus linking, headers and target dependencies.

### 4. Import the user's IPA and establish writable paths

- **Status:** IPA selection, validation, atomic private copying, restart persistence, native path handoff and representative native resource reads are complete on the Pixel. The engine uses its app-private save path, and a basic saved game survived an APK update and loaded through `Continue`; interrupted-write hardening remains.
- **Objective:** Implement the real first-launch data flow.
- **Files/subsystems:** Android picker bridge, `Main`, `ZipFile`, `JavaStream`, configuration paths.
- **Expected changes:** Temporary private copy, validation, atomic installation, explicit resource/save paths and recoverable errors.
- **Unknowns/risks:** Invalid archives, short reads, excessive sizes, cancellation, insufficient space and provider behavior.
- **Success:** Read representative image/map/string/sound entries, restart offline and read them again. Cancelled or invalid imports leave a usable app and preserve an existing valid installation.
- **Architecture consequence of failure:** May justify replacing ZIP parser internals while retaining the resource interface. Supporting additional IPA layouts is deferred.
- **Learning focus:** File versus stream access, resource ownership and validating external input.

### 5. Display actual game content, initially silent

- **Status:** Complete for the first-level checkpoint. The Pixel reaches the intro, title/main menu, Options and difficulty menus, then renders the Tower level with geometry, sprites, text, HUD, transparency and fog. The centered 3:2 viewport remains fully visible under Android 17 immersive mode.
- **Objective:** Show the title/menu and a rendered level.
- **Files/subsystems:** `SDLGL`, `Main`, `GLES`, `Image`, `Graphics`, `Render`, `TinyGL`.
- **Expected changes:** Apply proven graphics adaptations, correct viewport conversions, scale the 480x320 logical display without distortion, and allow safe silent startup.
- **Unknowns/risks:** Combined graphics state, texture handling, HUD placement and software-buffer presentation.
- **Success:** Title/menu plus a level with geometry, sprites, text, HUD, transparency and fog match the desktop baseline. Test transitions and scrolling.
- **Architecture consequence of failure:** Main renderer decision gate; change the rendering backend only where evidence requires it.
- **Learning focus:** Following one frame through the existing rendering code.

### 6. Establish complete touch input

- **Status:** Partial. Touch drives the intro, menus, dialogue, paged help, basic forward movement and the first successful combat encounter on the Pixel. An SDL event-state fix also makes the main-menu Options and in-game Save/Load transitions respond reliably to one tap. Inventory, minigames, broader combat and complete-campaign coverage remain.
- **Objective:** Make the game playable using the phone's touchscreen.
- **Files/subsystems:** `Input`, `CAppContainer`, `Canvas`, `MenuSystem`, `Button`, `CardGames` and other touch-oriented minigames.
- **Expected changes:** Connect SDL finger events to the existing touch handlers; preserve the 480 by 320 logical coordinate system; correct scaling, gestures, hit detection and Android system-edge behavior where testing exposes problems.
- **Unknowns/risks:** Single-pointer assumptions, small targets, drag thresholds, coordinate conversion, duplicate synthetic events and system edge gestures.
- **Success:** The title screen, menus, movement, combat, dialogs, inventory, mixing, cards, driving, save/load and other required interactions can be completed on the Pixel using touch. No duplicate actions or stuck highlights occur.
- **Architecture consequence of failure:** Add targeted virtual controls or multi-pointer state rather than rewrite gameplay input.
- **Learning focus:** Mapping physical screen coordinates into the game's logical display and existing touch model.

### 7. Enable audio

- **Status:** Basic Android audio is complete on the Pixel. Generated PCM, imported effects, looping music, simultaneous music/effects, independent volume controls and Home/lock resume behavior pass. External audio-focus interruptions and output-route changes remain.
- **Objective:** Play music and effects reliably.
- **Files/subsystems:** `Sound`, OpenAL Soft build, `CAppContainer`, Android audio-focus integration.
- **Expected changes:** Package the dependency, handle initialization failure, connect pause/resume and focus.
- **Unknowns/risks:** Backend behavior, WAV assumptions and output changes.
- **Success:** Generated PCM WAV first, then an imported game sound, then simultaneous music/effects. Test mute, interruptions and output-route changes.
- **Architecture consequence of failure:** Try the alternative OpenAL Soft Android backend before replacing the audio layer.
- **Learning focus:** Samples, buffers and playback sources.

### 8. Evaluate optional controller support

- **Objective:** Determine whether the Cyclone 2 is a useful optional input method, without blocking touch-first play.
- **Files/subsystems:** SDL controller events, `Input` and a small diagnostic or input log.
- **Expected changes:** Detect the wired controller and record its reported buttons, axes, connection and reconnection behavior. Add gameplay mappings later only where they provide a clear benefit.
- **Unknowns/risks:** Wired compatibility, device mappings, dead zones and Android USB behavior.
- **Success:** Establish whether Android and SDL recognize the controller over USB-C and document its input report. Full controller-only campaign completion is not required.
- **Architecture consequence of failure:** None for the MVP; continue with touch as the supported input path.
- **Learning focus:** How Android and SDL represent an external game controller.

### 9. Harden lifecycle and persistence

- **Status:** Partial. Home/reopen, lock/unlock and force-stop/restart pass with the real engine on the Pixel, and the saved-game checksums remain stable. Configuration writes are still direct and have produced a partial file when interrupted by an APK installation.
- **Objective:** Survive ordinary phone use. Start basic handling earlier; this milestone validates it systematically.
- **Files/subsystems:** Main loop, SDL events, rendering resources, audio, input, saves and streams.
- **Expected changes:** Pause safely, reset timing/input, recover graphics resources, persist atomically and repair affected cleanup ownership.
- **Unknowns/risks:** Process death, lost graphics context, interrupted writes and busy-loop battery use.
- **Success:** Repeated Home/lock/resume cycles; process termination followed by loading the last committed save; same-signature updates preserve imports/settings. No corrupted saves or stuck input.
- **Architecture consequence of failure:** May require clearer resource ownership and reinitialization boundaries, not a gameplay rewrite.
- **Learning focus:** Process lifetime versus saved state; why cleanup callbacks are not guaranteed.

Uninstall/reinstall normally removes private data. Surviving that requires a separately designed export/restore or backup feature.

### 10. Package reproducible releases

- **Objective:** Build a signed, data-free APK from a clean checkout.
- **Files/subsystems:** Android project, CI, dependency pins, release documentation and packaging rules.
- **Expected changes:** Automated builds, signing, saved debugging symbols, native-library alignment checks and explicit packaged-resource allowlists.
- **Unknowns/risks:** Accidental data inclusion, dependency compatibility and signing continuity.
- **Success:** Clean CI produces an ARM64 APK that opens the importer on a fresh device; archive inspection finds no IPA or extracted proprietary resources; update testing passes. Verify native libraries on supported page-size configurations.
- **Architecture consequence of failure:** Usually packaging/toolchain changes only.
- **Learning focus:** Build artifacts, reproducibility and release identity.

Audit icons and the embedded image array in `App.h` too: excluding the IPA alone does not establish that all shipped art is independently distributable. Include appropriate source and license materials for the engine and dependencies.

## Portability issues already identified

| Finding | Treatment |
| --- | --- |
| Desktop GL headers and unconditional OpenGL linking | Platform-specific graphics headers and dependencies |
| OpenAL linkage only in Windows/Linux branches | Explicit Android dependency linkage |
| Local SDL finder creates imported targets and searches desktop paths | Use Android source-built SDL targets without host-library discovery or target collisions |
| Fixed-width types without direct standard includes | Make headers such as `ZipFile.h`, `JavaStream.h` and `Sound.h` self-contained |
| ARGB constants such as `0xFFCC0000` in signed `int` switches | Correct comparisons/types while preserving stored bit patterns; examples in `Canvas.cpp` around line 5092 |
| Plain `char` used for the `-1` help-dialog thread sentinel | Use an explicit signed 8-bit type so ARM cannot reinterpret the sentinel as index 255 |
| `int*` cast to `float*` during viewport scaling in `GLES.cpp` | Replace type-punning with actual numeric conversion |
| Relative save directory and shell-based creation | Explicit writable root and ordinary directory creation |
| Repeated explicit destructor calls | Repair ownership at affected startup/shutdown and stream boundaries |
| Missing mobile lifecycle handling | Pause/resume, input reset, persistence and graphics recovery |
| Desktop window/renderer options persisted in config | Adapt or ignore inapplicable settings on Android |

Android Clang now compiles every engine source file for ARM64. The table records both fixed defects and remaining portability work found through compilation and phone testing.

## Risk ranking

| Rank | Risk | Why |
| --- | --- | --- |
| High | Rendering correctness across devices | Synthetic/title tests do not prove all world effects work |
| High | Lifecycle and ownership | Missing integration and explicit destruction patterns threaten resume/restart |
| High | Archive robustness | Reader trusts sizes/offsets and uses fatal errors |
| Medium | Clang/ARM portability | Concrete defects exist; optimized behavior needs testing |
| Medium | Touch integration | Existing touch-oriented UI is substantial, but the SDL adapter and pointer/coordinate assumptions remain unproven |
| Low | Optional wired-controller support | Detection is unproven, but it does not block the touch-first MVP |
| Medium | Audio interruptions and routing | Core API is portable; mobile behavior needs validation |
| Medium | Data-free packaging | Local files and bundled images require an explicit audit |
| Low | Basic SDL Android structure | Established workflow fits the application |
| Low | Copy-based private import | Matches the existing seekable reader |

## MVP and later polish

The smallest engineering proof is an ARM64 landscape build that imports the IPA and runs silent gameplay with working touchscreen input.

A usable MVP should include normal installation on the Pixel, one supported IPA layout, aspect-correct display, complete touchscreen play, save/load, basic background/resume, audio with safe failure handling, and no packaged proprietary resources. GLES1 is conditional on experiments. Controller support is optional for the MVP and can be added where it provides a clear benefit.

Do not call the game fully playable based on reaching the first level. A complete campaign validation using the touchscreen is needed for that claim.

Phone-native polish adds more comfortable hit targets, screen-cutout and gesture-area refinements, optional controller mappings, import progress and recovery, save export/import, robust audio routing, context-loss recovery and broader device testing.

## Immediate next chunk

**Smoke-test inventory and item interaction with touch.**

Use the existing saved game to open the inventory and exercise the item screens that are currently reachable: switch categories, select an item, view its details and return to play. Record any screen that needs repeated taps, has an unreachable control or confuses a swipe with a tap.

The success gate is one complete inventory round trip using single taps and ordinary swipes, with the selected item and game state preserved on return. If a failure appears, instrument only that interaction before changing behavior.

## Source files informing the plan

- Build: `CMakeLists.txt`, `src/CMakeLists.txt`, `cmake/Modules/FindSDL2.cmake`, `vcpkg.json`, `third_party_libs/hash-library/CMakeLists.txt`.
- Startup/platform: `src/Main.cpp`, `src/SDLGL.cpp`, `src/SDLGL.h`, `src/CAppContainer.cpp`.
- Rendering: `src/GLES.cpp`, `src/GLES.h`, `src/TinyGL.cpp`, `src/Render.cpp`, `src/Image.cpp`, `src/Graphics.cpp`, `src/IDIB.cpp`.
- Input/UI: `src/Input.cpp`, `src/Canvas.cpp`, `src/Canvas.h`, `src/MenuSystem.cpp`, `src/MenuSystem.h`, `src/Button.cpp`, `src/CardGames.cpp`.
- Resources/storage: `src/ZipFile.cpp`, `src/ZipFile.h`, `src/JavaStream.cpp`, `src/JavaStream.h`, `src/Game.cpp`, `src/Resource.cpp`.
- Audio/application: `src/Sound.cpp`, `src/Sound.h`, `src/App.cpp`, `src/App.h`, `src/Utils.h`.

## References

- [SDL2 Android integration](https://wiki.libsdl.org/SDL2/README-android)
- [SDL2 graphics-context attributes](https://raw.githubusercontent.com/libsdl-org/SDL/SDL2/include/SDL_video.h)
- [Android document picker and Storage Access Framework](https://developer.android.com/training/data-storage/shared/documents-files)
- [Android app-private storage](https://developer.android.com/training/data-storage/app-specific)
- [Android OpenGL ES support](https://developer.android.com/develop/ui/views/graphics/opengl/about-opengl)
- [Khronos GLES 1.1 specification](https://registry.khronos.org/OpenGL/specs/es/1.1/es_full_spec_1.1.pdf)
- [OpenAL Soft build configuration](https://raw.githubusercontent.com/kcat/openal-soft/master/CMakeLists.txt)
- [Android emulator acceleration and processor matching](https://developer.android.com/studio/run/emulator-acceleration)
- [Android emulator hardware limitations](https://developer.android.com/studio/run/advanced-emulator-usage)
- [Android 16 KB page-size support](https://developer.android.com/guide/practices/page-sizes)
- [GameSir Cyclone 2 connection guide](https://gamesir.com/pages/tutorial-how-to-connect-cyclone-2)

Platform documentation changes over time. Pin concrete dependency/tool versions when implementation begins and verify requirements against those versions.
