#include "FileSystem.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <vector>
#endif

const char* dir = "WolfensteinRPG.app";

namespace FileSystem {
namespace fs = std::filesystem;

std::string savePath(const char* name) {
    if (!name || !*name) throw std::runtime_error("Missing save filename.");
    const auto leaf = fs::u8path(name);
    if (leaf.has_parent_path() || leaf.is_absolute())
        throw std::runtime_error("Invalid save filename.");
    return (fs::u8path(dir) / leaf).u8string();
}

FILE* open(const char* path, const char* mode) {
#ifdef _WIN32
    const std::wstring wideMode(mode, mode + std::strlen(mode));
    FILE* file = _wfopen(fs::u8path(path).c_str(), wideMode.c_str());
#else
    FILE* file = std::fopen(path, mode);
#endif
    // Missing input files are normal on first run; other failures must be visible.
    if (!file && !(mode[0] == 'r' && errno == ENOENT))
        throw std::runtime_error(std::string("Cannot open ") + path + ": " +
            std::strerror(errno) + ". Check permissions and available disk space.");
    return file;
}

void remove(const char* path) {
#ifdef _WIN32
    const int result = _wremove(fs::u8path(path).c_str());
#else
    const int result = std::remove(path);
#endif
    if (result && errno != ENOENT)
        throw std::runtime_error(std::string("Cannot remove ") + path + ": " + std::strerror(errno));
}

void prepareDirectory(const fs::path& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec || !fs::is_directory(path))
        throw std::runtime_error("Cannot create storage directory " + path.u8string() +
            ". Check the path and directory permissions. " + ec.message());
#ifdef _WIN32
    // CREATE_NEW never truncates existing data. Delete-on-close leaves no probe file.
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        const auto probe = path / (L".wolf-storage-check-" + std::to_wstring(GetCurrentProcessId()) +
            L"-" + std::to_wstring(attempt));
        HANDLE handle = CreateFileW(probe.c_str(), GENERIC_WRITE | DELETE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); return; }
        const auto error = GetLastError();
        if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
            throw std::runtime_error("Storage directory is not writable: " + path.u8string() +
                " (Windows error " + std::to_string(error) +
                "). Check permissions, or remove portable.txt to use Local AppData.");
    }
    throw std::runtime_error("Cannot check storage permissions: " + path.u8string());
#endif
}

#ifdef _WIN32
fs::path executableDirectory() {
    std::vector<wchar_t> buffer(256);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (!length) throw std::runtime_error("Cannot determine executable directory.");
        if (length < buffer.size()) return fs::path(std::wstring(buffer.data(), length)).parent_path();
        if (buffer.size() >= 32768) throw std::runtime_error("Executable path is too long.");
        buffer.resize(buffer.size() * 2);
    }
}

fs::path localAppData() {
    PWSTR value = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &value);
    if (FAILED(result)) throw std::runtime_error("Cannot resolve Windows Local AppData.");
    const fs::path path(value);
    CoTaskMemFree(value);
    return path;
}

WindowsPaths resolveWindowsPaths(const fs::path& executableDir, const fs::path& appData) {
    if (!executableDir.is_absolute()) throw std::runtime_error("Executable directory must be absolute.");
    const auto marker = executableDir / L"portable.txt";
    const bool portable = fs::exists(marker);
    if (portable && !fs::is_regular_file(marker))
        throw std::runtime_error("portable.txt must be a file beside the executable.");
    const auto legacy = executableDir / L"WolfensteinRPG.app";
    // Fail closed even if AppData already has saves: choosing which set to use is explicit.
    if (!portable && fs::exists(legacy) && (!fs::is_directory(legacy) || !fs::is_empty(legacy)))
        throw std::runtime_error("Legacy save data detected in " + legacy.u8string() +
            ". No saves have been moved or changed. To keep using these saves, create an empty "
            "portable.txt beside the executable. To use standard storage, back up and manually copy "
            "the saves to Local AppData/PellPorts/WolfensteinRPG, resolve any duplicate files yourself, "
            "then rename the legacy folder. See README.md for migration instructions.");
    if (!portable && !appData.is_absolute()) throw std::runtime_error("Local AppData must be absolute.");
    return {executableDir, executableDir / L"Wolfenstein RPG.ipa",
        portable ? legacy : appData / L"PellPorts" / L"WolfensteinRPG", portable};
}
#endif
}
