#pragma once
#ifdef _WIN32
#include "FileSystem.h"
#include <functional>
#include <optional>

namespace WindowsStartup {
enum class ImportChoice { Copy, ChooseAnother, Cancel };
// UI boundary also permits deterministic tests of the entire selection loop.
struct UserInterface {
    std::function<std::optional<std::filesystem::path>()> pick;
    std::function<void(const std::string&)> error;
    std::function<ImportChoice(const std::filesystem::path&)> offerImport;
    std::function<bool(const std::filesystem::path&)> confirmReplace;
};
std::filesystem::path metadataPath(const FileSystem::WindowsPaths& paths);
std::optional<std::filesystem::path> rememberedIpa(const FileSystem::WindowsPaths& paths);
void rememberIpa(const FileSystem::WindowsPaths& paths, const std::filesystem::path& selected);
std::optional<std::filesystem::path> discoverIpa(const FileSystem::WindowsPaths& paths);
void importIpa(const std::filesystem::path& source, const std::filesystem::path& target, bool replace);
std::optional<std::filesystem::path> selectIpa(const FileSystem::WindowsPaths& paths, const UserInterface& ui);
// Owns COM initialization and native dialogs. No SDL window is needed.
std::optional<std::filesystem::path> selectIpa(const FileSystem::WindowsPaths& paths);
}
#endif
