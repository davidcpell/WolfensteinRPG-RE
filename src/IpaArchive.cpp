#include "IpaArchive.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <set>
#include <stdexcept>
#include <zlib.h>

namespace IpaArchive {
const std::vector<std::string>& requiredEntries() {
    static const std::vector<std::string> entries = {
        "Payload/Wolf...RPG.app/Info.plist",
        "Payload/Wolf...RPG.app/Packages/logo.bmp",
        "Payload/Wolf...RPG.app/Packages/strings.idx",
        "Payload/Wolf...RPG.app/Packages/strings00.bin",
        "Payload/Wolf...RPG.app/Packages/tables.bin",
        "Payload/Wolf...RPG.app/Packages/entities.bin",
        "Payload/Wolf...RPG.app/Packages/menus.bin",
        "Payload/Wolf...RPG.app/Packages/newMappings.bin",
        "Payload/Wolf...RPG.app/Packages/newPalettes.bin",
        "Payload/Wolf...RPG.app/Packages/model_0000.bin",
        "Payload/Wolf...RPG.app/Packages/newTexels000.bin",
        "Payload/Wolf...RPG.app/Packages/sounds/Alarm.wav"
    };
    return entries;
}
namespace {
uint32_t u16(const unsigned char* p) { return p[0] | (uint32_t(p[1]) << 8); }
uint32_t u32(const unsigned char* p) { return u16(p) | (u16(p + 2) << 16); }
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void read(std::ifstream& in, uint64_t offset, void* data, size_t size, uint64_t limit) {
    check(offset <= limit && size <= limit - offset, "Truncated ZIP archive.");
    in.seekg(static_cast<std::streamoff>(offset));
    in.read(static_cast<char*>(data), static_cast<std::streamsize>(size));
    check(bool(in), "Cannot read ZIP archive.");
}
// Stream decompression into fixed-size buffers, bounding both memory and work.
void verifyData(std::ifstream& in, uint64_t start, uint32_t compressed, uint32_t size,
                uint32_t expectedCrc, uint32_t method, uint64_t limit) {
    std::array<unsigned char, 65536> input{}, output{};
    z_stream stream{};
    struct End { z_stream* stream; bool active; ~End() { if (active) inflateEnd(stream); } } end{&stream, method == 8};
    if (method == 8) check(inflateInit2(&stream, -MAX_WBITS) == Z_OK, "Cannot initialize ZIP validation.");
    uint64_t produced = 0;
    uLong crc = crc32(0, Z_NULL, 0);
    uint32_t remaining = compressed;
    int code = Z_OK;
    while (remaining) {
        const auto chunk = static_cast<uint32_t>((std::min)(size_t(remaining), input.size()));
        read(in, start, input.data(), chunk, limit);
        start += chunk; remaining -= chunk;
        if (method == 0) {
            produced += chunk; crc = crc32(crc, input.data(), chunk);
        } else {
            stream.next_in = input.data(); stream.avail_in = chunk;
            do {
                const auto before = stream.avail_in;
                stream.next_out = output.data(); stream.avail_out = static_cast<uInt>(output.size());
                code = inflate(&stream, Z_NO_FLUSH);
                check(code == Z_OK || code == Z_STREAM_END, "Corrupt compressed resource.");
                const auto count = static_cast<uInt>(output.size()) - stream.avail_out;
                produced += count; crc = crc32(crc, output.data(), count);
                check(produced <= size, "Resource exceeds declared size.");
                check(code == Z_STREAM_END || count || stream.avail_in < before, "Invalid compressed resource.");
                if (code == Z_STREAM_END) {
                    check(!remaining && !stream.avail_in, "Trailing compressed resource data.");
                    break;
                }
            } while (stream.avail_in || stream.avail_out == 0);
        }
    }
    check(method == 0 || code == Z_STREAM_END, "Incomplete compressed resource.");
    check(produced == size && crc == expectedCrc, "Resource size or CRC check failed.");
}
}

std::string validate(const std::filesystem::path& path) try {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    check(bool(in), "Cannot open this file. Check that it exists and is readable.");
    const auto end = in.tellg();
    check(end >= 22 && end < INT32_MAX, "Not a supported IPA/ZIP archive (size must be below 2 GiB).");
    const auto length = static_cast<uint64_t>(end);
    const size_t tailSize = static_cast<size_t>((std::min)(length, uint64_t(65557)));
    std::vector<unsigned char> tail(tailSize);
    read(in, length - tailSize, tail.data(), tail.size(), length);
    size_t eocd = tail.size() - 22;
    for (;;) {
        if (u32(tail.data() + eocd) == 0x06054b50 && eocd + 22 + u16(tail.data() + eocd + 20) == tail.size()) break;
        check(eocd != 0, "Not a readable IPA/ZIP archive: ZIP directory not found.");
        --eocd;
    }
    const auto* e = tail.data() + eocd;
    const auto count = u16(e + 10);
    check(!u16(e + 4) && !u16(e + 6) && u16(e + 8) == count && count && count < 32768,
          "Split, ZIP64, or oversized ZIP directories are not supported by this engine.");
    uint64_t position = u32(e + 16), directoryEnd = position + u32(e + 12);
    check(directoryEnd == length - tailSize + eocd, "Invalid ZIP directory bounds.");
    std::set<std::string> found, foldedNames;
    uint64_t total = 0;
    for (uint32_t index = 0; index < count; ++index) {
        std::array<unsigned char, 46> header{};
        read(in, position, header.data(), header.size(), directoryEnd);
        const auto* h = header.data();
        check(u32(h) == 0x02014b50, "Invalid ZIP directory entry.");
        const auto nameLength = u16(h + 28), extraLength = u16(h + 30), commentLength = u16(h + 32);
        check(nameLength && nameLength < 32768 && extraLength < 32768 && commentLength < 32768 && !u16(h + 34),
              "Unsupported ZIP entry metadata.");
        std::string name(nameLength, '\0');
        read(in, position + 46, &name[0], name.size(), directoryEnd);
        check(name.find('\0') == std::string::npos && found.insert(name).second, "Ambiguous ZIP entry names.");
        std::string folded = name;
        for (char& c : folded) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        check(foldedNames.insert(folded).second, "Ambiguous case-insensitive ZIP entry names.");
        position += 46 + nameLength + extraLength + commentLength;
        check(position <= directoryEnd, "Truncated ZIP entry metadata.");
        const uint32_t compressed = u32(h + 20), size = u32(h + 24), offset = u32(h + 42);
        check(compressed < INT32_MAX && size < INT32_MAX && offset < INT32_MAX, "ZIP64 entries are not supported.");
        // Validate every engine resource, not only the baseline presence checks.
        const bool resource = name == requiredEntries()[0];
        const bool package = name.rfind("Payload/Wolf...RPG.app/Packages/", 0) == 0;
        if ((!resource && !package) || name.back() == '/') continue;
        check(size && size <= 128 * 1024 * 1024, "Empty or excessively large game resource.");
        total += size;
        check(total <= 1024ULL * 1024 * 1024, "Game resources exceed the validation size limit.");
        const auto flags = u16(h + 8), method = u16(h + 10);
        check(!(flags & (1 | 0x40)) && (method == 0 || method == 8), "Encrypted or unsupported resource compression.");
        std::array<unsigned char, 30> local{};
        read(in, offset, local.data(), local.size(), u32(e + 16));
        check(u32(local.data()) == 0x04034b50 && u16(local.data() + 6) == flags && u16(local.data() + 8) == method,
              "Invalid local resource header.");
        const auto localNameLength = u16(local.data() + 26), localExtra = u16(local.data() + 28);
        check(localNameLength == nameLength && localExtra < 32768, "Invalid local resource filename.");
        std::string localName(localNameLength, '\0');
        read(in, uint64_t(offset) + 30, &localName[0], localName.size(), u32(e + 16));
        check(localName == name, "Resource filename mismatch.");
        verifyData(in, uint64_t(offset) + 30 + localNameLength + localExtra, compressed, size, u32(h + 16), method, u32(e + 16));
    }
    check(position == directoryEnd, "ZIP directory size mismatch.");
    for (const auto& required : requiredEntries())
        if (!found.count(required)) return "Missing required Wolfenstein RPG resource: " + required;
    return {};
} catch (const std::exception& error) {
    return error.what();
}
}
