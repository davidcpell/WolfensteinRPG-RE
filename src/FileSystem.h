#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

// Runtime UTF-8 directory, also usable by platform startup code (e.g. Android).
extern const char* dir;

namespace FileSystem {
std::string savePath(const char* name);
FILE* open(const char* utf8Path, const char* mode);
void remove(const char* utf8Path);
void prepareDirectory(const std::filesystem::path& path);

#ifdef _WIN32
struct WindowsPaths {
    std::filesystem::path executableDirectory;
    std::filesystem::path ipa;
    std::filesystem::path storage;
    bool portable;
};
std::filesystem::path executableDirectory();
std::filesystem::path localAppData();
// Explicit inputs permit isolated tests; production passes the Windows API results.
WindowsPaths resolveWindowsPaths(const std::filesystem::path& executableDir,
                                 const std::filesystem::path& appData);
#endif
}
