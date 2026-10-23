#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include "WindowsStartup.h"
#include "IpaArchive.h"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace WindowsStartup {
namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;
namespace {
void winCheck(bool success, const std::string& action) {
    if (!success) throw std::runtime_error(action + " (Windows error " + std::to_string(GetLastError()) +
        "). Check permissions and available disk space, then try again.");
}
void comCheck(HRESULT result, const char* action) {
    if (FAILED(result)) throw std::runtime_error(std::string(action) + " (HRESULT " + std::to_string(result) + ").");
}
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    void close() { if (value != INVALID_HANDLE_VALUE) { CloseHandle(value); value = INVALID_HANDLE_VALUE; } }
};
// Same-directory unique staging file. Ordinary failures remove only our own file;
// interrupted-process leftovers retain .tmp and are never used as game data.
struct Temporary {
    fs::path path;
    Handle file;
    explicit Temporary(const fs::path& parent) {
        GUID id{}; comCheck(CoCreateGuid(&id), "Cannot name temporary file");
        wchar_t name[40]; StringFromGUID2(id, name, 40);
        path = parent / (std::wstring(L".wolf-startup-") + name + L".tmp");
        file.value = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        winCheck(file.value != INVALID_HANDLE_VALUE, "Cannot create temporary file in " + parent.u8string());
    }
    ~Temporary() { file.close(); DeleteFileW(path.c_str()); }
    void write(const void* data, DWORD size) {
        DWORD written = 0;
        winCheck(WriteFile(file.value, data, size, &written, nullptr) && written == size, "Cannot write temporary file");
    }
    void finish() { winCheck(FlushFileBuffers(file.value), "Cannot flush temporary file"); file.close(); }
    void publish(const fs::path& target, bool replace) {
        winCheck(MoveFileExW(path.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0)),
            "Cannot install " + target.u8string());
    }
};
constexpr const char* metadataHeader = "WolfensteinRPG IPA path v1\n";
void validateOrThrow(const fs::path& path) {
    const auto error = IpaArchive::validate(path);
    if (!error.empty()) throw std::runtime_error("Cannot use " + path.u8string() + "\n\n" + error +
        "\n\nSelect an original, unencrypted Wolfenstein RPG iOS IPA. Renaming another game's archive will not work.");
}
std::optional<fs::path> pick() {
    ComPtr<IFileOpenDialog> dialog;
    comCheck(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)), "Cannot create IPA picker");
    DWORD options = 0;
    comCheck(dialog->GetOptions(&options), "Cannot read picker options");
    comCheck(dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR | FOS_DONTADDTORECENT), "Cannot set picker options");
    const COMDLG_FILTERSPEC filters[] = {{L"iOS application archives (*.ipa)", L"*.ipa"}, {L"All files (*.*)", L"*.*"}};
    comCheck(dialog->SetFileTypes(2, filters), "Cannot set IPA filter");
    comCheck(dialog->SetTitle(L"Select your Wolfenstein RPG IPA"), "Cannot set picker title");
    comCheck(dialog->SetOkButtonLabel(L"Use IPA"), "Cannot set picker button");
    const auto result = dialog->Show(nullptr);
    if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return std::nullopt;
    comCheck(result, "Cannot open IPA picker");
    ComPtr<IShellItem> item;
    comCheck(dialog->GetResult(&item), "Cannot read IPA selection");
    PWSTR value = nullptr;
    comCheck(item->GetDisplayName(SIGDN_FILESYSPATH, &value), "Cannot read IPA path");
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> owned(value, CoTaskMemFree);
    return fs::path(value);
}
}

fs::path metadataPath(const FileSystem::WindowsPaths& paths) { return paths.storage / L"startup" / L"ipa-path.txt"; }

std::optional<fs::path> rememberedIpa(const FileSystem::WindowsPaths& paths) try {
    if (paths.portable) return std::nullopt;
    std::ifstream input(metadataPath(paths), std::ios::binary | std::ios::ate);
    if (!input || input.tellg() > 131072) return std::nullopt;
    input.seekg(0);
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const std::string header(metadataHeader);
    if (!input.eof() && input.fail()) return std::nullopt;
    if (text.rfind(header, 0) != 0) return std::nullopt;
    const auto value = text.substr(header.size());
    if (value.empty() || value.find_first_of("\r\n\0", 0, 3) != std::string::npos) return std::nullopt;
    const auto path = fs::u8path(value);
    return path.is_absolute() ? std::optional<fs::path>(path) : std::nullopt;
} catch (const std::exception&) { return std::nullopt; }

void rememberIpa(const FileSystem::WindowsPaths& paths, const fs::path& selected) {
    if (paths.portable) throw std::runtime_error("Portable mode cannot remember an external IPA.");
    const auto path = fs::absolute(selected).u8string();
    if (path.find_first_of("\r\n\0", 0, 3) != std::string::npos) throw std::runtime_error("Unsupported IPA filename.");
    const auto target = metadataPath(paths);
    FileSystem::prepareDirectory(target.parent_path());
    Temporary temp(target.parent_path());
    const std::string text = std::string(metadataHeader) + path;
    temp.write(text.data(), static_cast<DWORD>(text.size()));
    temp.finish(); temp.publish(target, true);
}

std::optional<fs::path> discoverIpa(const FileSystem::WindowsPaths& paths) {
    if (IpaArchive::validate(paths.ipa).empty()) return paths.ipa;
    if (const auto remembered = rememberedIpa(paths))
        if (IpaArchive::validate(*remembered).empty()) return remembered;
    return std::nullopt;
}

void importIpa(const fs::path& source, const fs::path& target, bool replace) {
    validateOrThrow(source);
    if (!replace && fs::exists(target)) throw std::runtime_error("An IPA already exists. Replacement requires your explicit consent.");
    Handle input;
    // Deny writes/deletion while taking the copy; never modify the source.
    input.value = CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    winCheck(input.value != INVALID_HANDLE_VALUE, "Cannot read " + source.u8string());
    Temporary temp(target.parent_path());
    std::vector<unsigned char> buffer(65536);
    for (;;) {
        DWORD count = 0;
        winCheck(ReadFile(input.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr), "Cannot copy IPA");
        if (!count) break;
        temp.write(buffer.data(), count);
    }
    temp.finish();
    validateOrThrow(temp.path); // Never publish a partial or invalid copy.
    input.close();
    temp.publish(target, replace);
}

std::optional<fs::path> selectIpa(const FileSystem::WindowsPaths& paths, const UserInterface& ui) {
    if (const auto automatic = discoverIpa(paths)) return automatic;
    for (;;) {
        const auto selected = ui.pick();
        if (!selected) return std::nullopt;
        try {
            validateOrThrow(*selected);
            if (!paths.portable) {
                rememberIpa(paths, *selected);
                return selected;
            }
            const auto choice = ui.offerImport(paths.ipa);
            if (choice == ImportChoice::Cancel) return std::nullopt;
            if (choice == ImportChoice::ChooseAnother) continue;
            const bool exists = fs::exists(paths.ipa);
            if (exists && !ui.confirmReplace(paths.ipa)) continue;
            importIpa(*selected, paths.ipa, exists);
            return paths.ipa;
        } catch (const std::exception& error) { ui.error(error.what()); }
    }
}

std::optional<fs::path> selectIpa(const FileSystem::WindowsPaths& paths) {
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    comCheck(initialized, "Cannot initialize Windows file selection");
    struct ComLifetime { ~ComLifetime() { CoUninitialize(); } } lifetime;
    UserInterface ui;
    ui.pick = pick;
    ui.error = [](const std::string& error) {
        const auto wide = fs::u8path(error).native();
        MessageBoxW(nullptr, wide.c_str(), L"Unable to use this IPA", MB_OK | MB_ICONWARNING);
    };
    ui.offerImport = [](const fs::path& target) {
        const auto text = L"Copy this IPA into the portable installation?\n\nDestination: " + target.wstring() +
            L"\n\nCopying lets you move the entire game directory to another computer. Your original IPA will be preserved."
            L"\n\nYes: import a copy\nNo: choose a different file\nCancel: exit without starting";
        const auto result = MessageBoxW(nullptr, text.c_str(), L"Make this installation portable", MB_YESNOCANCEL | MB_ICONQUESTION);
        return result == IDYES ? ImportChoice::Copy : result == IDNO ? ImportChoice::ChooseAnother : ImportChoice::Cancel;
    };
    ui.confirmReplace = [](const fs::path& target) {
        const auto text = L"An IPA already exists at:\n" + target.wstring() +
            L"\n\nReplace it with the validated selected IPA? Saves will not be changed.";
        return MessageBoxW(nullptr, text.c_str(), L"Replace existing IPA?", MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) == IDYES;
    };
    return selectIpa(paths, ui);
}
}
#endif
