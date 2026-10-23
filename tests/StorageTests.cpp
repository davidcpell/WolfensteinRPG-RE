#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#define NOMINMAX
#include <windows.h>
#include <sddl.h>
#include "FileSystem.h"
#include "JavaStream.h"
#include "CAppContainer.h"
#include "ZipFile.h"

// Resource access and the app error dialog are outside these filesystem tests.
// All local-file InputStream/OutputStream methods are the production implementation.
CAppContainer::CAppContainer() : app(nullptr), zipFile(nullptr) {}
CAppContainer::~CAppContainer() {}
CAppContainer* CAppContainer::getInstance() { static CAppContainer instance; return &instance; }
void Applet::Error(const char*, ...) { throw std::runtime_error("Unexpected engine I/O error"); }
uint8_t* ZipFile::readZipFileEntry(const char*, int*) { throw std::runtime_error("Unexpected resource read"); }

namespace fs = std::filesystem;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template<class F> void fails(F action, const char* message) {
    bool failed = false;
    try { action(); } catch (const std::exception&) { failed = true; }
    require(failed, message);
}
void put(const fs::path& path, const char* value = "") { std::ofstream(path, std::ios::binary) << value; }
std::string get(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void roundTrip(const fs::path& storage) {
    const std::string selected = storage.u8string();
    dir = selected.c_str();
    for (const char* name : {"Config", "FullPlayer"}) {
        OutputStream output;
        require(output.openFile(name, 1) == 1, "OutputStream open failed");
        output.writeInt(0x12345678);
        output.writeBoolean(true);
        output.close();
        InputStream input;
        require(input.loadFile(name, LT_FILE), "InputStream open failed");
        require(input.readInt() == 0x12345678 && input.readBoolean(), "Stream round trip failed");
        require(get(storage / name) == std::string("\x78\x56\x34\x12\x01", 5), "Save bytes changed");
    }
    require(!FileSystem::open(FileSystem::savePath("Missing").c_str(), "rb"), "Missing save must be normal");
    const auto disposable = FileSystem::savePath("Disposable");
    FILE* file = FileSystem::open(disposable.c_str(), "wb");
    require(file != nullptr && fclose(file) == 0, "Create disposable file failed");
    FileSystem::remove(disposable.c_str());
    FileSystem::remove(disposable.c_str()); // Missing deletes are harmless.
    require(!fs::exists(fs::u8path(disposable)), "Resolved delete failed");
    fails([] { FileSystem::savePath("../escape"); }, "Save traversal allowed");
    dir = "WolfensteinRPG.app";
}

int wmain(int argc, wchar_t** argv) try {
    // A copied probe executable verifies the real GetModuleFileNameW path after relocation.
    if (argc == 3 && std::wstring(argv[1]) == L"--probe") {
        const auto executable = FileSystem::executableDirectory();
        const auto paths = FileSystem::resolveWindowsPaths(executable, {});
        require(paths.portable && paths.storage == executable / L"WolfensteinRPG.app", "Probe portable path");
        require(paths.ipa == executable / L"Wolfenstein RPG.ipa", "Probe IPA path");
        require(executable == fs::path(argv[2]), "Probe executable directory");
        FileSystem::prepareDirectory(paths.storage);
        roundTrip(paths.storage);
        return 0;
    }
    wchar_t temp[MAX_PATH];
    require(GetTempPathW(MAX_PATH, temp) != 0, "GetTempPathW failed");
    const auto root = fs::path(temp) / (L"wolf-storage-tests-" + std::to_wstring(GetCurrentProcessId()));
    require(fs::create_directory(root), "Test directory must be new");
    const auto previous = fs::current_path();
    // Cleanup only this newly created, known test directory, including on failure.
    struct Cleanup {
        fs::path root, previous;
        ~Cleanup() { fs::current_path(previous); std::error_code ec; fs::remove_all(root, ec); }
    } cleanup{root, previous};
    const auto install = root / L"installation 日本語";
    const auto appData = root / L"appdata ü";
    const auto unrelated = root / L"unrelated";
    fs::create_directories(install);
    fs::create_directories(unrelated);
    fs::current_path(unrelated);
    auto paths = FileSystem::resolveWindowsPaths(install, appData);
    require(!paths.portable && paths.storage == appData / L"PellPorts" / L"WolfensteinRPG", "Standard path");
    require(paths.ipa == install / L"Wolfenstein RPG.ipa", "IPA is not executable relative");
    FileSystem::prepareDirectory(paths.storage);
    roundTrip(paths.storage);
    require(!fs::exists(unrelated / L"WolfensteinRPG.app"), "CWD affected saves");
    require(FileSystem::localAppData().is_absolute(), "Known folder API failed");
    require(FileSystem::executableDirectory().is_absolute(), "Executable API failed");
    std::cout << "PASS standard, unrelated CWD, known folder, stream round trip and byte compatibility\n";

    const auto legacy = install / L"WolfensteinRPG.app";
    fs::create_directory(legacy);
    put(legacy / L"legacy-sentinel", "original legacy progress");
    fails([&] { FileSystem::resolveWindowsPaths(install, appData); }, "Legacy data silently ignored");
    require(get(legacy / L"legacy-sentinel") == "original legacy progress", "Legacy data changed");
    put(install / L"portable.txt");
    paths = FileSystem::resolveWindowsPaths(install, {});
    require(paths.portable && paths.storage == legacy, "Portable path");
    roundTrip(paths.storage);
    require(get(legacy / L"legacy-sentinel") == "original legacy progress", "Portable test changed legacy data");
    std::cout << "PASS legacy refusal, explicit portable opt-in, legacy data preserved\n";

    // No IPA fixture: test just the path and the same wide-character file-open boundary.
    require(!FileSystem::open(paths.ipa.u8string().c_str(), "rb"), "Unexpected IPA fixture");
    const auto probe = install / L"StorageTests.exe";
    fs::copy_file(FileSystem::executableDirectory() / L"StorageTests.exe", probe);
    auto runProbe = [&](const fs::path& folder) {
        std::wstring command = L"\"" + (folder / L"StorageTests.exe").wstring() + L"\" --probe \"" + folder.wstring() + L"\"";
        STARTUPINFOW startup{}; startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        require(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, unrelated.c_str(), &startup, &process), "Cannot start copied probe");
        const DWORD wait = WaitForSingleObject(process.hProcess, 30000);
        DWORD code = 1;
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hThread); CloseHandle(process.hProcess);
        require(wait == WAIT_OBJECT_0 && code == 0, "Copied probe failed");
    };
    // The test executable uses only system DLLs and the installed MSVC runtime.
    runProbe(install);
    const auto relocated = root / L"relocated 日本語";
    fs::rename(install, relocated);
    runProbe(relocated);
    require(get(relocated / L"WolfensteinRPG.app" / L"legacy-sentinel") == "original legacy progress", "Relocation changed legacy");
    std::cout << "PASS copied executable and relocation with unrelated CWD, Unicode paths and executable-relative IPA\n";

    put(root / L"blocked", "not a directory");
    fails([&] { FileSystem::prepareDirectory(root / L"blocked" / L"child"); }, "Invalid destination accepted");
    fs::create_directory(root / L"portable.txt");
    fails([&] { FileSystem::resolveWindowsPaths(root, appData); }, "Directory marker accepted");
    const auto readonly = root / L"readonly-save";
    put(readonly, "untouched");
    require(SetFileAttributesW(readonly.c_str(), FILE_ATTRIBUTE_READONLY), "Set readonly failed");
    fails([&] { FileSystem::open(readonly.u8string().c_str(), "wb"); }, "Readonly save accepted");
    SetFileAttributesW(readonly.c_str(), FILE_ATTRIBUTE_NORMAL);
    require(get(readonly) == "untouched", "Readonly save overwritten");

    // Deny file creation to Everyone in a disposable directory. The Windows directory
    // readonly attribute alone does not prohibit writes, so use a real DACL here.
    const auto denied = root / L"denied";
    fs::create_directory(denied);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    require(ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:(D;;0x00000002;;;WD)(A;;FA;;;WD)",
        SDDL_REVISION_1, &descriptor, nullptr), "Create test DACL failed");
    const BOOL secured = SetFileSecurityW(denied.c_str(), DACL_SECURITY_INFORMATION, descriptor);
    LocalFree(descriptor);
    require(secured, "Apply test DACL failed");
    fails([&] { FileSystem::prepareDirectory(denied); }, "Unwritable directory accepted");
    std::cout << "PASS invalid paths, invalid marker, readonly file and denied directory\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
}
