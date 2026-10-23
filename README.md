# WolfensteinRPG-RE

![Imagen1](https://github.com/user-attachments/assets/cbfab4c2-5310-45cf-a694-7a74e0a299bd)<br>

https://www.doomworld.com/forum/topic/151475

Wolfenstein RPG Reverse Engineering By [GEC]<br />
[Created by Erick Vásquez García](https://github.com/Erick194/WolfensteinRPG-RE).

Current version 0.1

### **Requirements**
You need CMake to make the project.<br />
What you need for the project is:
  * SDL2
  * Zlib
  * OpenAL

### 🔹 **Ubuntu**
Before compiling, make sure to install the following dependencies:
```sh
sudo apt update && sudo apt install -y cmake g++ libsdl2-dev zlib1g-dev libopenal-dev libgl1-mesa-dev
```

#### ⚙️ **Compilation**
Clone the repository and compile the project using CMake:
```sh
git clone https://github.com/Erick194/WolfensteinRPG-RE.git
cd WolfensteinRPG-RE
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### 🚀 **Execution**
After compiling, run the game with:
```sh
./build/src/WolfensteinRPG
```

#### 📦 **Creating the AppImage**
🔹 1. Download required tools
```sh
wget -O appimagetool https://github.com/AppImage/AppImageKit/releases/latest/download/appimagetool-x86_64.AppImage
wget -O linuxdeploy https://github.com/linuxdeploy/linuxdeploy/releases/latest/download/linuxdeploy-x86_64.AppImage
chmod +x appimagetool linuxdeploy
```
🔹 2. Generate the AppImage using the script
```sh
chmod +x make-wolfenstein-rpg-linux-appimage.sh
./make-wolfenstein-rpg-linux-appimage.sh ./appimagetool ./linuxdeploy $(pwd) ./build/src/WolfensteinRPG
```

### 🔹 **Windows**

Windows prerequisites:
* Visual Studio with the **Desktop development with C++** workload
* CMake
* vcpkg

The included `vcpkg.json` manifest will install the required SDL2, zlib, and OpenAL dependencies.

Run the compilation commands below from **Developer PowerShell for Visual Studio**, which configures the environment so CMake can find the relevant build tools. To open it:

1. Open the Windows **Start** menu by clicking the Windows icon or pressing the Windows key.
1. Type **Developer PowerShell for VS** in the search bar.
1. Open **Developer PowerShell for VS**.

If you don't see `Developer PowerShell for VS`, open Visual Studio and select **Tools -> Get Tools and Features...** to open the Visual Studio Installer. In the installer, select the **Workloads** tab, enable **Desktop development with C++**, and click **Modify**.

Now search the Start menu again for **Developer PowerShell for VS**. When you have it open, verify that the C++ compiler, CMake, and vcpkg are available by running:

```powershell
cl
cmake --version
vcpkg version
```

If `cl`, `cmake`, or `vcpkg` is still unavailable, go back to the Visual Studio Installer, open the **Individual components** tab, and make sure **C++ CMake tools for Windows** and **vcpkg package manager** are installed.

#### ⚙️ **Compilation**

Clone the repository:

```powershell
git clone https://github.com/Erick194/WolfensteinRPG-RE.git
cd WolfensteinRPG-RE
```

Locate the vcpkg CMake toolchain file:

```powershell
$vcpkgRoot = Split-Path (Get-Command vcpkg).Source
$toolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"
```

Configure:

```powershell
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE="$toolchain"
```

Build:

```powershell
cmake --build build --config Release
```

The executable should now exist at:

```text
build\src\Release\WolfensteinRPG.exe
```

#### 🚀 **Execution**

From the repository root, run:

```powershell
.\build\src\Release\WolfensteinRPG.exe
```

#### Windows first launch and game data

Extract the Windows build into a folder and double-click `WolfensteinRPG.exe`.
You need your legally obtained **original Wolfenstein RPG iOS IPA**. No developer
tools, command-line arguments, or separate launcher are required to play.

If a valid `Wolfenstein RPG.ipa` is beside the executable, the game uses it
automatically. Otherwise Windows opens a native file picker. Select your IPA
wherever it is stored; you do not need to rename it. The picker offers both IPA
and **All files** filters for unusual filenames. Invalid or unsupported archives
show an explanation and return to the picker. **Cancel** exits normally.

Do not commit, package, upload, or redistribute the IPA, imported copies, or
extracted resources. They are local user data and are not included with this port.

| Mode | Saves and configuration | IPA discovery order |
| --- | --- | --- |
| Standard (no marker) | `%LOCALAPPDATA%\PellPorts\WolfensteinRPG\` | Executable-adjacent IPA, remembered external IPA, picker |
| Portable (marker present) | `<executable-directory>\WolfensteinRPG.app\` | Executable-adjacent IPA, picker with local import |

Windows resolves the executable directory independently of the working directory
with `GetModuleFileNameW`. Standard storage uses the Windows
`FOLDERID_LocalAppData` known-folder API. Explorer, shortcuts, PowerShell, and
launchers therefore select the same paths.

**Standard mode:** a selected external IPA is read directly in place. Its absolute
UTF-8 path is remembered in `startup\ipa-path.txt` inside the standard data
directory, separate from game saves and configuration. Subsequent launches reuse
it after validation. If it is moved, deleted, unreadable, or invalid, the picker
opens again. A valid executable-adjacent IPA always takes precedence. Moving or
replacing the executable does not move this user's saves.

**Portable mode:** create an empty file named exactly `portable.txt` beside the
executable (not `portable.txt.txt`). Quiver Launcher can create the same marker,
but no launcher is needed. If there is no valid adjacent IPA, choose your original
IPA and accept the offer to **import a copy** as `Wolfenstein RPG.ipa` beside the
executable. The dialog explains that copying lets the whole installation move to
another computer. Your original is preserved. **No** returns to file selection;
Cancel exits. If an IPA already exists, a separate replacement confirmation is
required, with No selected by default.

Portable mode never remembers an external absolute path. Move the complete folder
with the executable, required DLLs, `portable.txt`, `Wolfenstein RPG.ipa`, and
`WolfensteinRPG.app`. The installation must be writable. An unwritable destination
produces an actionable error; it does not silently switch to AppData.

**Replacing an IPA:** close the game first. In standard mode, either replace your
external file in place, or rename/remove only `startup\ipa-path.txt` to select
another file on next launch. If an adjacent IPA exists, rename it first to stop it
taking precedence. In portable mode, back up and rename the adjacent IPA before
launching to select/import another one. Saves do not need to be removed.

Imports use a unique same-directory staging file, flush it, validate the completed
copy, then publish it with a Windows rename. A failed copy or replacement preserves
the existing IPA and cleans up its own staging file. Normal exceptions also clean
up metadata staging files. A forced process termination or power loss can leave a
`.wolf-startup-*.tmp` file; it is never accepted as an adjacent IPA. With the game
closed, leftover files matching that name can be removed manually. Network shares
and filesystem/hardware failure may offer weaker crash guarantees than local NTFS.

Validation checks the Android baseline entry list, including `Info.plist` and
essential files under `Payload/Wolf...RPG.app/Packages/`. It checks ZIP bounds,
compression support, resource sizes, and streamed decompression/CRC for all package
files without extracting them. It checks archive integrity and expected layout,
not every proprietary resource's internal semantics. Encrypted, split, ZIP64, and
archives at or above 2 GiB are unsupported; individual resources are limited to
128 MiB and total validated resources to 1 GiB to bound validation work.

There is no automatic fallback to a different save directory. File-open and
startup errors identify the path or operation; recoverable selection, persistence,
and import failures let you select again. Late save write failures retain the
existing engine error dialog.

#### Existing Windows saves: explicit choice required

Older builds used `WolfensteinRPG.app` under the **process working directory**.
This might be the repository root, a shortcut's “Start in” directory, or a launcher
working directory. This version cannot discover every old working directory.

If a nonempty `WolfensteinRPG.app` exists **beside the executable** without
`portable.txt`, startup stops with instructions, even if AppData already contains
another set of saves. Detection never moves, deletes, copies, or overwrites saves.
An empty legacy directory does not block startup.

Close the game and back up the complete old save directory before choosing:

* **Keep saves beside the executable:** add `portable.txt` beside it. This explicitly
  selects that existing `WolfensteinRPG.app` directory. If your old saves are under
  another working directory, manually copy the complete backed-up directory beside
  the executable first; resolve any existing destination files yourself.
* **Use standard storage:** open `%LOCALAPPDATA%` in Explorer and create
  `PellPorts\WolfensteinRPG`. Manually copy the **contents** of your backed-up
  save directory into it. If destination files already exist, back up both sets and
  explicitly decide which set to retain; do not blindly overwrite or combine
  progress. Rename the old directory beside the executable (for example,
  `WolfensteinRPG.app.backup`) to acknowledge the choice, and leave the marker
  absent.

Removing a marker or changing modes does not migrate saves. The save format is
unchanged; ordinary gameplay still updates files in the explicitly selected
directory. Keep backups until you have verified your progress in the game.

When switching from standard to portable mode, select/import the IPA separately;
the standard-mode remembered path is intentionally ignored in portable mode.

Filesystem paths cross the engine boundary as UTF-8 and use Windows UTF-16 APIs
for executable/known-folder lookup, directory operations, IPA/save opens and
save deletion. Tests cover accented and Japanese directory names. Arbitrary
Windows long-path support is not guaranteed; normal Windows/CRT path limits
still apply.

#### Windows storage tests

After configuring the Windows build above (CTest tests are enabled by default):

```powershell
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Tests use newly created temporary directories, generated ZIP archives, and
disposable text only. They exercise real local-file streams, exact save bytes,
Unicode paths, legacy refusal, invalid/read-only destinations, directory ACL
denial, and copied/relocated executables. Startup tests cover archive integrity,
the engine ZIP handoff, IPA precedence, retry/cancel, UTF-8 persistence, missing
selections, portable import consent, failed replacement, and temporary-file
cleanup. They contain no original IPA or extracted resources.

Live validation on Windows with the MSVC Release build (2026-10-23):

| Check | Observed result |
| --- | --- |
| Clean standard launch | Native picker appeared before the game window |
| Invalid selection | Clear validation warning, then picker reopened in the same process |
| Valid external IPA with Unicode name and unusual extension | Game launched; original read in place |
| Standard restart | Remembered path reused without a picker |
| Remembered file moved | Picker reopened; cancellation returned exit code 0 |
| Portable selection | Copy explanation and consent dialog appeared; import hash matched source |
| Portable relocation to Unicode directories | Automatic local IPA discovery, with external selected path unavailable |
| Configuration after relocation/restart | SFX volume 48 loaded from the relocated portable configuration |
| Unrelated working directory | Missing remembered IPA still prompted; relocated portable game launched |
| Existing development saves | All six pre-existing file hashes remained unchanged |

The live installations and imported IPA were outside Git and were removed after
testing. The standard data directory was absent before the test and contained
only newly created test metadata. Existing development data was not used for
writes. Synthetic tests cover overwrite refusal, failed replacement and permission
errors; the live session did not simulate disk exhaustion, power failure, or a
full game playthrough. An unrelated device-update overlay was left untouched;
the game window was moved out of its way with computer control.

The Android branch can continue to assign the runtime `dir` at startup. When
merging, retain a single definition (now in `FileSystem.cpp`), compile that file
with C++17, and reconcile the platform startup blocks in `Main.cpp`. Windows
path selection must remain under `_WIN32`; Android's supplied IPA/save paths
remain authoritative there. `WindowsStartup.cpp` is Windows-only and
`IpaArchive.cpp` uses the existing zlib dependency; the Windows UI uses only
system COM/Shell APIs, with no .NET or additional GUI framework.

### 🎮 **Default key configuration:**

Move Forward: W, Up<br />
Move Backward: S, Down<br />
Move Left: A<br />
Move Right: D<br />
Turn Left: Left<br />
Turn Right: Right<br />
Atk/Talk/Use: Return<br />
Next Weapon: Z<br />
Prev Weapon: X<br />
Pass Turn: C<br />
Automap: Tab<br />
Menu Open/Back: Escape<br />
Items/Info: I<br />
Syringes: O<br />
Journal: P<br />

### 🔑 **Original game cheat codes:**

J2ME/BREW Version:<br />
3666 -> Opens debug menu.<br />
1666 -> Restarts level.<br />
4332 -> Gives all keys, items and weapons to the player.<br />
3366 -> Starts speed test "Benchmark".<br />

iOS Version:<br />
Tap the screen corners in the following order:<br />
Top left, top right, bottom left, bottom right, bottom left, bottom right -> Opens debug menu.<br />
Top left, top right, bottom left, bottom right, top left, bottom right -> Restarts level.<br />
Top left, top right, bottom left, bottom right, top right, bottom right -> Gives all keys, items and weapons to the player.<br />
Top left, top right, bottom left, bottom right, bottom left, top right -> Starts speed test "Benchmark".<br />
