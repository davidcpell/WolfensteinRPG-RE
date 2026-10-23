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

#### Windows files and portable mode

Place your legally obtained original `Wolfenstein RPG.ipa` beside
`WolfensteinRPG.exe` (for a local Release build, in `build\src\Release\`).
The IPA is read in place. Do not commit or redistribute the IPA or extracted resources.
There is no IPA picker in this version.

Windows resolves the executable directory with `GetModuleFileNameW`, independently
of the working directory. Explorer, shortcuts, PowerShell, and Quiver Launcher use
the same paths:

| Mode | Saves and configuration |
| --- | --- |
| Standard (no marker) | `%LOCALAPPDATA%\PellPorts\WolfensteinRPG\` |
| Portable (marker present) | `<executable-directory>\WolfensteinRPG.app\` |

Standard mode uses Windows' `FOLDERID_LocalAppData` known-folder API, rather than
assuming an environment variable exists. Replacing or moving the executable does
not change this user-specific save location.

To enable portable mode, create an **empty file named exactly `portable.txt`**
beside the executable. Check that Explorer has not named it `portable.txt.txt`.
Quiver Launcher may create this same marker; no launcher is required. Marker
contents are ignored. Move the entire installation, including the marker and
`WolfensteinRPG.app`, to keep portable saves with the game. The installation must
be writable; remove the marker to select standard storage, after deciding how to
preserve any portable saves.

The selected directory is created and checked for write access before the engine
starts. Invalid paths, inaccessible storage, and missing IPAs produce an error
dialog and a nonzero startup exit code. File-open errors include the attempted
path; late save write/close failures use the existing engine error dialog.
There is no automatic fallback to another save directory.

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

Tests use newly created temporary directories and disposable data only. They
exercise the real local-file stream implementation, exact save bytes, Unicode
paths, legacy refusal, missing files, invalid destinations, read-only files,
directory ACL denial, and copied/relocated test executables launched from an
unrelated directory. They contain no IPA or extracted resources.

The Android branch can continue to assign the runtime `dir` at startup. When
merging, retain a single definition (now in `FileSystem.cpp`), compile that file
with C++17, and reconcile the platform startup blocks in `Main.cpp`. Windows
path selection must remain under `_WIN32`; Android's supplied IPA/save paths
remain authoritative there.

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
