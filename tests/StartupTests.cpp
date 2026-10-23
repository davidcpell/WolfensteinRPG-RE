#define NOMINMAX
#include <windows.h>
#include "WindowsStartup.h"
#include "IpaArchive.h"
#include "ZipFile.h"
#include <memory>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <zlib.h>

namespace fs = std::filesystem;
namespace startup = WindowsStartup;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void fails(F action, const char* message) {
    bool failed = false; try { action(); } catch (const std::exception&) { failed = true; }
    require(failed, message);
}
void put(const fs::path& path, const std::string& text) { std::ofstream(path, std::ios::binary) << text; }
std::string get(const fs::path& path) {
    std::ifstream in(path, std::ios::binary); return {std::istreambuf_iterator<char>(in), {}};
}
void n16(std::string& out, uint32_t value) { out += char(value); out += char(value >> 8); }
void n32(std::string& out, uint32_t value) { n16(out, value); n16(out, value >> 16); }
// Generated entry bodies contain only disposable text, never game resources.
std::string archive(bool deflated, bool missing = false, const std::vector<std::string>& customNames = {}) {
    std::string out, central;
    auto names = customNames.empty() ? IpaArchive::requiredEntries() : customNames;
    if (missing) names.pop_back();
    for (const auto& name : names) {
        const std::string content = "Synthetic test data for " + name;
        std::string compressed = content;
        if (deflated) {
            z_stream z{};
            require(deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) == Z_OK, "deflate init");
            compressed.resize(1024);
            z.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(content.data())); z.avail_in = static_cast<uInt>(content.size());
            z.next_out = reinterpret_cast<Bytef*>(&compressed[0]); z.avail_out = static_cast<uInt>(compressed.size());
            require(deflate(&z, Z_FINISH) == Z_STREAM_END, "deflate test archive");
            compressed.resize(z.total_out); deflateEnd(&z);
        }
        const auto crc = crc32(0, reinterpret_cast<const Bytef*>(content.data()), static_cast<uInt>(content.size()));
        const auto offset = static_cast<uint32_t>(out.size());
        n32(out, 0x04034b50); n16(out, 20); n16(out, 0); n16(out, deflated ? 8 : 0);
        n32(out, 0); n32(out, crc); n32(out, static_cast<uint32_t>(compressed.size())); n32(out, static_cast<uint32_t>(content.size()));
        n16(out, static_cast<uint32_t>(name.size())); n16(out, 0); out += name; out += compressed;
        n32(central, 0x02014b50); n16(central, 20); n16(central, 20); n16(central, 0); n16(central, deflated ? 8 : 0);
        n32(central, 0); n32(central, crc); n32(central, static_cast<uint32_t>(compressed.size())); n32(central, static_cast<uint32_t>(content.size()));
        n16(central, static_cast<uint32_t>(name.size())); n16(central, 0); n16(central, 0); n16(central, 0); n16(central, 0); n32(central, 0); n32(central, offset); central += name;
    }
    const auto offset = static_cast<uint32_t>(out.size()); out += central;
    n32(out, 0x06054b50); n16(out, 0); n16(out, 0); n16(out, static_cast<uint32_t>(names.size())); n16(out, static_cast<uint32_t>(names.size()));
    n32(out, static_cast<uint32_t>(central.size())); n32(out, offset); n16(out, 0);
    return out;
}
struct FakeUI {
    std::vector<std::optional<fs::path>> choices;
    size_t picked = 0;
    int errors = 0, offers = 0, confirmations = 0;
    bool replace = false;
    startup::ImportChoice import = startup::ImportChoice::Copy;
    startup::UserInterface ui() {
        return {[this]() { require(picked < choices.size(), "Unexpected picker"); return choices[picked++]; },
            [this](const std::string& message) { require(!message.empty(), "Empty error"); ++errors; },
            [this](const fs::path&) { ++offers; return import; },
            [this](const fs::path&) { ++confirmations; return replace; }};
    }
};
void noTemps(const fs::path& path) {
    for (const auto& entry : fs::recursive_directory_iterator(path))
        require(entry.path().extension() != L".tmp", "Temporary file leaked");
}
int wmain(int argc, wchar_t** argv) try {
    if (argc == 3 && std::wstring(argv[1]) == L"--validate") {
        const auto error = IpaArchive::validate(argv[2]); std::cout << (error.empty() ? "VALID" : error) << '\n'; return error.empty() ? 0 : 1;
    }
    const auto root = fs::temp_directory_path() / (L"wolf-startup-tests-" + std::to_wstring(GetCurrentProcessId()));
    require(fs::create_directory(root), "Temporary directory must be new");
    const auto previous = fs::current_path();
    struct Cleanup { fs::path root, previous; ~Cleanup() { fs::current_path(previous); std::error_code ec; fs::remove_all(root, ec); } } cleanup{root, previous};
    const auto install = root / L"installation 日本語", external = root / L"outside ü";
    fs::create_directory(install); fs::create_directory(external);
    const auto valid = external / L"original 日本語.ipa", stored = external / L"stored.ipa", invalid = external / L"bad.ipa";
    put(valid, archive(true)); put(stored, archive(false)); put(invalid, "not a ZIP");
    require(IpaArchive::validate(valid).empty() && IpaArchive::validate(stored).empty(), "Valid archives rejected");
    require(!IpaArchive::validate(invalid).empty() && !IpaArchive::validate(external / L"missing").empty(), "Invalid input accepted");
    const auto broken = external / L"broken.ipa";
    put(broken, archive(true, true)); require(IpaArchive::validate(broken).find("Missing required") != std::string::npos, "Missing resources accepted");
    auto corrupt = archive(false); corrupt[30 + IpaArchive::requiredEntries()[0].size()] ^= 1;
    put(broken, corrupt); require(!IpaArchive::validate(broken).empty(), "CRC corruption accepted");
    auto truncated = archive(true); truncated.resize(truncated.size() - 10); put(broken, truncated);
    require(!IpaArchive::validate(broken).empty(), "Truncation accepted");
    for (const auto& path : {valid, stored}) {
        ZipFile engine; engine.openZipFile(path.u8string().c_str());
        int size = 0;
        std::unique_ptr<uint8_t, decltype(&free)> data(engine.readZipFileEntry(IpaArchive::requiredEntries()[0].c_str(), &size), free);
        require(std::string(reinterpret_cast<char*>(data.get()), size) == "Synthetic test data for " + IpaArchive::requiredEntries()[0], "Engine handoff failed");
        engine.closeZipFile();
    }
    const auto tiny = external / L"tiny.zip";
    auto tinyZip = archive(false, false, {"tiny.txt"});
    require(tinyZip.size() < 512, "Small ZIP fixture too large");
    put(tiny, tinyZip);
    { ZipFile engine; engine.openZipFile(tiny.u8string().c_str()); engine.closeZipFile(); }
    require(!IpaArchive::validate(tiny).empty(), "Unrelated ZIP accepted as game data");
    // Long ZIP comments, including signature bytes, must not confuse the engine.
    auto commented = archive(true);
    const std::string comment = std::string(4090, 'x') + std::string("PK\5\6", 4);
    commented[commented.size()-2] = char(comment.size());
    commented[commented.size()-1] = char(comment.size() >> 8);
    commented += comment;
    put(broken, commented);
    require(IpaArchive::validate(broken).empty(), "ZIP comment rejected");
    { ZipFile engine; engine.openZipFile(broken.u8string().c_str()); engine.closeZipFile(); }
    std::cout << "PASS stored/deflated archives, required resources, CRC, truncation, small/commented ZIPs and engine handoff\n";

    auto paths = FileSystem::resolveWindowsPaths(install, root / L"appdata");
    FileSystem::prepareDirectory(paths.storage);
    put(paths.storage / L"CONFIGFILENAME", "save sentinel");
    fs::current_path(external);
    require(!startup::discoverIpa(paths), "Unexpected IPA discovery");
    FakeUI first{{invalid, valid}};
    require(startup::selectIpa(paths, first.ui()) == valid && first.errors == 1 && first.picked == 2, "Invalid selection did not retry");
    require(startup::rememberedIpa(paths) == valid, "Unicode selection did not persist");
    FakeUI restart;
    require(startup::selectIpa(paths, restart.ui()) == valid && restart.picked == 0, "Restart prompted");
    fs::copy_file(stored, paths.ipa);
    require(startup::discoverIpa(paths) == paths.ipa, "Adjacent IPA must take precedence");
    put(paths.ipa, "invalid adjacent IPA");
    require(startup::discoverIpa(paths) == valid, "Invalid adjacent IPA blocked remembered selection");
    const auto moved = external / L"moved.ipa"; fs::rename(valid, moved);
    FakeUI cancel{{std::nullopt}};
    require(!startup::selectIpa(paths, cancel.ui()) && cancel.errors == 0, "Missing remembered IPA did not allow clean cancel");
    put(startup::metadataPath(paths), "malformed metadata"); require(!startup::rememberedIpa(paths), "Bad metadata accepted");
    startup::rememberIpa(paths, moved); put(moved, "now invalid");
    require(!startup::discoverIpa(paths), "Invalid remembered IPA accepted");
    put(moved, archive(true)); startup::rememberIpa(paths, moved);
    require(get(paths.storage / L"CONFIGFILENAME") == "save sentinel", "Startup modified saves");
    const auto metadata = startup::metadataPath(paths);
    const auto priorMetadata = get(metadata);
    HANDLE metadataLock = CreateFileW(metadata.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    require(metadataLock != INVALID_HANDLE_VALUE, "Lock metadata target");
    fails([&] { startup::rememberIpa(paths, stored); }, "Locked metadata replacement succeeded");
    CloseHandle(metadataLock);
    require(get(metadata) == priorMetadata, "Failed metadata update changed prior selection");
    noTemps(root);
    std::cout << "PASS retry/cancel, Unicode persistence, restart reuse, precedence, moved/invalid selections and unrelated CWD\n";

    put(install / L"portable.txt", "");
    paths = FileSystem::resolveWindowsPaths(install, {}); FileSystem::prepareDirectory(paths.storage);
    put(paths.storage / L"CONFIGFILENAME", "portable sentinel");
    require(!startup::rememberedIpa(paths), "Portable used external metadata");
    const auto original = get(moved), existing = get(paths.ipa);
    FakeUI declined{{moved, std::nullopt}};
    require(!startup::selectIpa(paths, declined.ui()) && declined.confirmations == 1 && get(paths.ipa) == existing, "Declined overwrite changed existing IPA");
    FakeUI imported{{moved}}; imported.replace = true;
    require(startup::selectIpa(paths, imported.ui()) == paths.ipa && imported.offers == 1 && imported.confirmations == 1, "Portable import failed");
    require(get(paths.ipa) == original && get(moved) == original, "Import modified source or data");
    require(!fs::exists(startup::metadataPath(paths)), "Portable persisted absolute path");
    fs::remove(moved); // disposable source; portable must now be self-contained
    const auto relocated = root / L"relocated 日本語";
    fs::rename(install, relocated); paths = FileSystem::resolveWindowsPaths(relocated, {});
    FakeUI portableRestart;
    require(startup::selectIpa(paths, portableRestart.ui()) == paths.ipa, "Relocation required external source");
    require(get(paths.storage / L"CONFIGFILENAME") == "portable sentinel", "Relocation changed saves");
    noTemps(root);
    std::cout << "PASS portable consent, import, source preservation, local reuse and relocation\n";

    const auto destination = root / L"destination.ipa"; put(destination, "existing data");
    fails([&] { startup::importIpa(stored, destination, false); }, "Overwrite without consent");
    fails([&] { startup::importIpa(invalid, destination, true); }, "Invalid import accepted");
    HANDLE locked = CreateFileW(destination.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    require(locked != INVALID_HANDLE_VALUE, "Cannot lock disposable target");
    fails([&] { startup::importIpa(stored, destination, true); }, "Failed replacement accepted");
    CloseHandle(locked);
    require(get(destination) == "existing data", "Failed import damaged target");
    noTemps(root);
    FakeUI noImport{{stored, std::nullopt}}; noImport.import = startup::ImportChoice::ChooseAnother;
    fs::remove(paths.ipa);
    require(!startup::selectIpa(paths, noImport.ui()) && !fs::exists(paths.ipa), "Declined import created data");
    FakeUI cancelImport{{stored}}; cancelImport.import = startup::ImportChoice::Cancel;
    require(!startup::selectIpa(paths, cancelImport.ui()), "Import cancellation did not exit");
    std::cout << "PASS import decline/cancel, failed atomic replacement, cleanup and existing data protection\n";
    return 0;
} catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
