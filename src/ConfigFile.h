#ifndef WOLFENSTEIN_CONFIG_FILE_H
#define WOLFENSTEIN_CONFIG_FILE_H

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <cerrno>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <direct.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

// Version 29 is fixed-size. Validate before the legacy reader mutates settings
// or reads beyond the end of an interrupted file.
inline bool isCompleteConfig(const uint8_t* data, size_t size,
    int savedIndexes, int mappings, int bindings) {
    const size_t expectedSize = 101 + 8 * savedIndexes + 4 * mappings * bindings;
    return data && size == expectedSize
        && data[0] == 29 && data[1] == 0 && data[2] == 0 && data[3] == 0
        && data[size - 4] == 0xef && data[size - 3] == 0xbe
        && data[size - 2] == 0xad && data[size - 1] == 0xde;
}

// Configuration is small: serialize it completely before touching the disk.
// Campaign saves continue to use their existing OutputStream path.
class ConfigFileOutput {
    std::vector<uint8_t> data;

public:
    void writeByte(uint8_t value) { data.push_back(value); }
    void writeBoolean(bool value) { writeByte(value ? 1 : 0); }
    void writeShort(int16_t value) {
        const uint16_t bits = static_cast<uint16_t>(value);
        writeByte(static_cast<uint8_t>(bits));
        writeByte(static_cast<uint8_t>(bits >> 8));
    }
    void writeInt(int value) {
        const uint32_t bits = static_cast<uint32_t>(value);
        for (int shift = 0; shift < 32; shift += 8)
            writeByte(static_cast<uint8_t>(bits >> shift));
    }

    bool save(const char* directory, const char* name) const {
#ifdef _WIN32
        if (_mkdir(directory) != 0 && errno != EEXIST) return false;
#else
        if (mkdir(directory, 0777) != 0 && errno != EEXIST) return false;
#endif
        const std::string target = std::string(directory) + "/" + name;
        const std::string temporary = target + ".tmp";
        FILE* file = fopen(temporary.c_str(), "wb");
        if (!file) return false;

        bool success = fwrite(data.data(), 1, data.size(), file) == data.size();
        if (success) success = fflush(file) == 0;
#ifdef _WIN32
        if (success) success = _commit(_fileno(file)) == 0;
#else
        if (success) success = fsync(fileno(file)) == 0;
#endif
        if (fclose(file) != 0) success = false;
        if (!success) {
            std::remove(temporary.c_str());
            return false;
        }

#ifdef _WIN32
        success = MoveFileExA(temporary.c_str(), target.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        success = std::rename(temporary.c_str(), target.c_str()) == 0;
#endif
        if (!success) {
            std::remove(temporary.c_str());
            return false;
        }
#ifndef _WIN32
        // Persist the rename as well as the contents. Failure here cannot undo
        // the replacement, but must still be reported to the caller.
        const int directoryFd = open(directory, O_RDONLY | O_DIRECTORY);
        if (directoryFd < 0) return false;
        success = fsync(directoryFd) == 0;
        if (close(directoryFd) != 0) success = false;
#endif
        return success;
    }
};

#endif
