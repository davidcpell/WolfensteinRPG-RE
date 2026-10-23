#include "../src/ConfigFile.h"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>

static void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

static std::vector<uint8_t> read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}

static ConfigFileOutput config(uint8_t difficulty) {
    ConfigFileOutput output;
    output.writeInt(29);
    output.writeByte(difficulty);
    // Remaining version-29 fields, excluding the final marker.
    for (int i = 5; i < 785; ++i) output.writeByte(0);
    output.writeInt(static_cast<int>(0xdeadbeefu));
    return output;
}

int main(int argc, char** argv) {
    const char* directory = "config-file-test-data";
    const std::string target = std::string(directory) + "/CONFIG";
    const std::string temporary = target + ".tmp";
    if (argc == 2 && std::string(argv[1]) == "--writer") {
        for (;;) {
            if (!config(1).save(directory, "CONFIG")) return 2;
            if (!config(2).save(directory, "CONFIG")) return 2;
        }
    }
    check(config(1).save(directory, "CONFIG"), "Initial save failed");
    const auto original = read(target);
    check(original.size() == 789 && original[4] == 1, "Serialization mismatch");
    check(isCompleteConfig(original.data(), original.size(), 6, 16, 10), "Valid config rejected");
    for (size_t size = 0; size < original.size(); ++size)
        check(!isCompleteConfig(original.data(), size, 6, 16, 10), "Truncation accepted");
    auto invalid = original;
    invalid[0] = 28;
    check(!isCompleteConfig(invalid.data(), invalid.size(), 6, 16, 10), "Unknown version accepted");
    invalid = original;
    invalid.back() = 0;
    check(!isCompleteConfig(invalid.data(), invalid.size(), 6, 16, 10), "Bad marker accepted");
    invalid = original;
    invalid.push_back(0);
    check(!isCompleteConfig(invalid.data(), invalid.size(), 6, 16, 10), "Unexpected length accepted");

    // A killed writer can leave an incomplete temporary file. The next load
    // still sees the previous config, and the next save replaces the leftover.
    {
        std::ofstream partial(temporary, std::ios::binary);
        partial.write(reinterpret_cast<const char*>(original.data()), 31);
    }
    check(read(target) == original, "Abandoned write changed existing config");
    check(config(2).save(directory, "CONFIG"), "Replacement save failed");
    const auto replacement = read(target);
    check(replacement.size() == original.size() && replacement[4] == 2,
        "Replacement has wrong contents");
    check(isCompleteConfig(replacement.data(), replacement.size(), 6, 16, 10),
        "Replacement is incomplete");

#ifdef _WIN32
    // Deny replacement after the temp file has been written and synced.
    HANDLE locked = CreateFileA(target.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(locked != INVALID_HANDLE_VALUE, "Could not lock destination");
    check(!config(3).save(directory, "CONFIG"), "Failed replacement reported success");
    check(read(target) == replacement, "Failed replacement damaged previous config");
    CloseHandle(locked);

    // Kill a separate writer at varying times during repeated replacements.
    // The destination must always be one complete generation, never a prefix.
    char executable[MAX_PATH];
    check(GetModuleFileNameA(nullptr, executable, MAX_PATH) != 0,
        "Could not find test executable");
    for (int iteration = 0; iteration < 20; ++iteration) {
        std::string command = std::string("\"") + executable + "\" --writer";
        std::vector<char> commandLine(command.begin(), command.end());
        commandLine.push_back(0);
        STARTUPINFOA startup = {};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process = {};
        check(CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr,
            FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != 0,
            "Could not start interruption test");
        Sleep(20 + iteration * 3);
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, INFINITE);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        const auto surviving = read(target);
        check(isCompleteConfig(surviving.data(), surviving.size(), 6, 16, 10),
            "Process interruption damaged config");
        check(surviving[4] == 1 || surviving[4] == 2,
            "Process interruption mixed generations");
    }
#endif

    // Verify signed values and every primitive's byte order independently.
    ConfigFileOutput primitives;
    primitives.writeInt(0x12345678);
    primitives.writeShort(-2);
    primitives.writeBoolean(true);
    primitives.writeBoolean(false);
    check(primitives.save(directory, "PRIMITIVES"), "Primitive save failed");
    check(read(std::string(directory) + "/PRIMITIVES") ==
        std::vector<uint8_t>({0x78, 0x56, 0x34, 0x12, 0xfe, 0xff, 1, 0}),
        "Primitive encoding changed");

    std::remove(target.c_str());
    std::remove(temporary.c_str());
    std::remove((std::string(directory) + "/PRIMITIVES").c_str());
#ifdef _WIN32
    _rmdir(directory);
#else
    rmdir(directory);
#endif
    std::cout << "Configuration durability tests passed\n";
}
