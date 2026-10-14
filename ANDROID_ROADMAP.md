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
- Controller: GameSir Cyclone 2; USB-C is the preferred connection.
- Controller-required gameplay is acceptable initially, subject to proving wired operation on the phone.
- Importer: support one known-good Wolfenstein RPG IPA layout first.
- Use an Android emulator for routine development, with physical-phone checks at specific milestones.
- Keep game logic unchanged where possible, reuse SDL2, and avoid a speculative renderer rewrite.

## Current state as of 2026-10-13

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

### Deferred or not started

- The harness has not run on the Pixel 9 Pro XL. Real Pixel GPU behavior, lifecycle behavior and the reported GLES implementation remain unverified.
- The GameSir Cyclone 2 has not been tested over USB-C.
- The existing game engine has not been compiled or linked into the Android APK.
- IPA selection/import, game-resource loading, saves, audio, game input and touch integration have not started.
- The current APK is an isolated graphics harness and does not contain proprietary game resources.

### Next recommended checkpoint

Install the current ARM64 harness on the Pixel, record its GLES vendor/renderer/version, and verify the checkerboard through pause/resume and orientation changes. After that phone checkpoint, continue the isolated GLES tests with alpha testing/blending, fog and the texture-combiner behavior used by the existing renderer before attempting to link the full game.

An IPA is an iPhone/iPad application archive, roughly equivalent to an Android APK. This port reads graphics, maps, sounds and other resources from the user's IPA; it runs the rebuilt C++ engine rather than the executable inside that archive.

## Evidence and confidence

Original source-review baseline: local `davidcpell/WolfensteinRPG-RE` commit `22a8e7e` (Add Windows build support). That initial review did not compile for Android or test an Android device. The Android harness work recorded above supersedes that limitation for the isolated GLES tests; the full game still has not been compiled for Android.

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
- Basic touch support may require an event adapter and fixes rather than a new virtual-control system.
- OpenAL Soft integration should mostly involve dependency/build setup plus lifecycle and failure handling.

### Requires experiments

- Whether alpha testing/blending, fog and texture-combiner behavior match expectations on the emulator, and whether all isolated rendering operations behave correctly on the Pixel.
- All Android Clang compilation errors and optimized ARM runtime issues.
- Cyclone 2 wired detection and complete button/axis mapping on the Pixel.
- Complete controller-only and touch-only coverage of gameplay and minigames.
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

First wire finger events into the existing touch interface. Preserve starting/current pointer coordinates because `Canvas` and `MenuSystem` currently consult mouse globals. Use actual release coordinates, handle cancelled gestures, and prevent duplicate synthetic mouse/finger actions.

Add controller defaults for all required actions and test every interactive game state. Controller detection alone does not establish playability. GameSir documents Android support and wired connectivity, but wired operation on this specific Pixel remains untested.

Use upstream OpenAL Soft, initially evaluating its OpenSL ES backend; Oboe is an alternative if needed. Add a deliberate silent mode, safe initialization failure, and background/audio-focus handling. Audio focus means responding when Android gives another application priority to play sound.

## Emulator and phone testing

An emulator runs a virtual Android device on the development computer. Use it for frequent development iterations, but do not treat it as a complete simulation of Pixel hardware.

| Test | Normal test environment |
| --- | --- |
| Build, install and launch | Emulator |
| Document picker, import errors and resource reads | Emulator |
| Save/load and restart | Emulator, then phone checkpoint |
| Menus and basic touch gestures | Emulator, then phone usability checks |
| Drawing and graphics effects | Emulator first, Pixel early |
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

- **Status:** Emulator portion complete through textured rendering. Pixel validation remains pending.
- **Objective:** Run native code and display a solid color.
- **Files/subsystems:** New minimal Android wrapper, SDL Android template and isolated graphics harness.
- **Expected changes:** Pin build tools/SDL, add emulator and phone build targets, request a GLES1 drawing context, create a landscape surface.
- **Unknowns/risks:** Tool installation, virtual-device configuration, library loading and context support.
- **Success:** App installs and repeatedly clears/presents a solid-color screen in the emulator; then the same experiment runs on the Pixel. Log the actual graphics implementation.
- **Architecture consequence of failure:** Fix setup first; a demonstrated graphics limitation informs the next experiment.
- **Learning focus:** How compiled C/C++ becomes part of an APK, and how a frame reaches the screen.

### 2. Test the game's graphics features in isolation

- **Status:** In progress. Basic matrices, client arrays, texture upload/filtering and textured drawing work on the emulator; alpha testing/blending, fog, texture combiners and Pixel behavior remain to be tested.
- **Objective:** Test GLES1 viability before substantial port work.
- **Files/subsystems:** Small graphics harness informed by `GLES`, `Image` and `Graphics`.
- **Expected changes:** Draw synthetic textured shapes using the game's texture formats, alpha testing, fog and corrected tint combiner.
- **Unknowns/risks:** Unsupported operations, driver differences, texture restrictions.
- **Success:** Expected images and no graphics API errors on emulator and Pixel; compare the effects individually.
- **Architecture consequence of failure:** Correct isolated differences first; fundamental limitations justify a small GLES2 backend.
- **Learning focus:** Vertices, textures and graphics state, introduced one effect at a time.

### 3. Compile and link the existing game for Android

- **Objective:** Produce a loadable native game library before full game startup.
- **Files/subsystems:** Root/src CMake files, SDL finder, hash-library target, platform headers and affected translation units.
- **Expected changes:** Add an Android shared target while retaining desktop builds; use Android-built dependencies; exclude Windows resources; fix demonstrated portability defects.
- **Unknowns/risks:** Missing includes, signed/unsigned conversions, dependency discovery and ARM assumptions.
- **Success:** Compile every game and bundled-library source file to object files; then link/load the library without unresolved symbols. Desktop still builds.
- **Architecture consequence of failure:** Normally local fixes; widespread binary-layout dependencies could require broader portability work.
- **Learning focus:** Compilation versus linking, headers and target dependencies.

### 4. Import the user's IPA and establish writable paths

- **Objective:** Implement the real first-launch data flow.
- **Files/subsystems:** Android picker bridge, `Main`, `ZipFile`, `JavaStream`, configuration paths.
- **Expected changes:** Temporary private copy, validation, atomic installation, explicit resource/save paths and recoverable errors.
- **Unknowns/risks:** Invalid archives, short reads, excessive sizes, cancellation, insufficient space and provider behavior.
- **Success:** Read representative image/map/string/sound entries, restart offline and read them again. Cancelled or invalid imports leave a usable app and preserve an existing valid installation.
- **Architecture consequence of failure:** May justify replacing ZIP parser internals while retaining the resource interface. Supporting additional IPA layouts is deferred.
- **Learning focus:** File versus stream access, resource ownership and validating external input.

### 5. Display actual game content, initially silent

- **Objective:** Show the title/menu and a rendered level.
- **Files/subsystems:** `SDLGL`, `Main`, `GLES`, `Image`, `Graphics`, `Render`, `TinyGL`.
- **Expected changes:** Apply proven graphics adaptations, correct viewport conversions, scale the 480x320 logical display without distortion, and allow safe silent startup.
- **Unknowns/risks:** Combined graphics state, texture handling, HUD placement and software-buffer presentation.
- **Success:** Title/menu plus a level with geometry, sprites, text, HUD, transparency and fog match the desktop baseline. Test transitions and scrolling.
- **Architecture consequence of failure:** Main renderer decision gate; change the rendering backend only where evidence requires it.
- **Learning focus:** Following one frame through the existing rendering code.

### 6. Establish a complete controller play path

- **Objective:** Play using the Cyclone 2 over USB-C.
- **Files/subsystems:** `Input`, `Canvas`, `MenuSystem`, configuration and minigames.
- **Expected changes:** Usable default bindings, Android Back handling, reconnect behavior and held-input reset.
- **Unknowns/risks:** Wired compatibility, mappings, dead zones, repeat behavior and pointer-oriented screens.
- **Success:** From fresh settings, navigate menus, move/turn/strafe, fight, use inventory, change weapons, save/load and complete representative minigames. Disconnect/reconnect without stuck input.
- **Architecture consequence of failure:** Usually input/UI fixes. Move basic touch earlier if that is cheaper than making every screen controller-only. Do not assume Bluetooth meets the user's wired preference without discussion.
- **Learning focus:** Translating physical input into existing game actions.

### 7. Enable audio

- **Objective:** Play music and effects reliably.
- **Files/subsystems:** `Sound`, OpenAL Soft build, `CAppContainer`, Android audio-focus integration.
- **Expected changes:** Package the dependency, handle initialization failure, connect pause/resume and focus.
- **Unknowns/risks:** Backend behavior, WAV assumptions and output changes.
- **Success:** Generated PCM WAV first, then an imported game sound, then simultaneous music/effects. Test mute, interruptions and output-route changes.
- **Architecture consequence of failure:** Try the alternative OpenAL Soft Android backend before replacing the audio layer.
- **Learning focus:** Samples, buffers and playback sources.

### 8. Connect and validate existing touch controls

- **Objective:** Play without a controller.
- **Files/subsystems:** `Input`, `CAppContainer`, `Canvas`, `MenuSystem`, `Button`, `CardGames`.
- **Expected changes:** Finger-event adapter, pointer state, coordinate conversion, cancellation and duplicate-event suppression.
- **Unknowns/risks:** Single-pointer assumptions, small targets, drag thresholds and system edge gestures.
- **Success:** Touch-only menus, movement/combat, dialogs, inventory, mixing, cards, driving and save/load. No duplicate actions or stuck highlights.
- **Architecture consequence of failure:** Add targeted virtual controls or multi-pointer state rather than rewrite gameplay input.
- **Learning focus:** Mapping physical screen coordinates into the game's logical display.

### 9. Harden lifecycle and persistence

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
| `int*` cast to `float*` during viewport scaling in `GLES.cpp` | Replace type-punning with actual numeric conversion |
| Relative save directory and shell-based creation | Explicit writable root and ordinary directory creation |
| Repeated explicit destructor calls | Repair ownership at affected startup/shutdown and stream boundaries |
| Missing mobile lifecycle handling | Pause/resume, input reset, persistence and graphics recovery |
| Desktop window/renderer options persisted in config | Adapt or ignore inapplicable settings on Android |

The exact Android compiler diagnostic list is still unknown. These findings do not claim that an Android compile has been attempted.

## Risk ranking

| Rank | Risk | Why |
| --- | --- | --- |
| High | Rendering correctness across devices | Synthetic/title tests do not prove all world effects work |
| High | Lifecycle and ownership | Missing integration and explicit destruction patterns threaten resume/restart |
| High | Archive robustness | Reader trusts sizes/offsets and uses fatal errors |
| Medium | Clang/ARM portability | Concrete defects exist; optimized behavior needs testing |
| Medium | Complete wired-controller play | Connection and coverage remain unproven |
| Medium | Touch integration | Existing UI is substantial, but pointer/coordinate assumptions remain |
| Medium | Audio interruptions and routing | Core API is portable; mobile behavior needs validation |
| Medium | Data-free packaging | Local files and bundled images require an explicit audit |
| Low | Basic SDL Android structure | Established workflow fits the application |
| Low | Copy-based private import | Matches the existing seekable reader |

## MVP and later polish

The smallest engineering proof is an ARM64 landscape build that imports the IPA and runs silent gameplay with a supported controller.

A usable MVP should include normal installation on the Pixel, one supported IPA layout, aspect-correct display, complete controller bindings, save/load, basic background/resume, audio with safe failure handling, and no packaged proprietary resources. GLES1 is conditional on experiments. Basic touch may be included early if it is easier than closing controller-only UI gaps.

Do not call the game fully playable based on reaching the first level. A complete campaign validation is needed for that claim.

Phone-native polish adds validated touch-only play, comfortable hit targets, screen-cutout/gesture-area handling, import progress and recovery, save export/import, robust audio routing, context-loss recovery and broader device testing.

## Immediate next chunk

**Validate the current textured GLES 1.1 harness on the Pixel 9 Pro XL.**

Install the existing ARM64 debug build on the phone, launch it, and confirm that the generated blue-and-white checkerboard renders correctly in landscape. Record the reported GLES vendor, renderer, version and drawable size. Then send the app to the background and resume it, and exercise an orientation change, to expose basic Android lifecycle or surface-recreation problems. This is a validation chunk; it should not add game loading, audio, controls or new rendering features.

After that: add isolated tests for alpha testing/blending, fog and the texture-combiner behavior used by the existing renderer. Validate each effect on the emulator and Pixel before performing the full-game NDK compile/link audit. The document-picker-to-resource path follows that audit.

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
