#include <SDL.h>

#include <cstdint>
#include <cstdlib>

#include "ZipFile.h"

extern "C" int wolfenstein_probe_ipa(const char* path)
{
    static const char* entries[] = {
        "Payload/Wolf...RPG.app/Packages/logo.bmp",
        "Payload/Wolf...RPG.app/Packages/strings.idx",
        "Payload/Wolf...RPG.app/Packages/tables.bin",
        "Payload/Wolf...RPG.app/Packages/sounds/Alarm.wav"
    };
    ZipFile archive;

    archive.openZipFile(path);
    for (const char* entryName : entries) {
        int size = 0;
        uint8_t* data = archive.readZipFileEntry(entryName, &size);
        uint32_t checksum = 2166136261u;

        for (int i = 0; i < size; ++i) {
            checksum = (checksum ^ data[i]) * 16777619u;
        }
        SDL_Log(
                "Native IPA entry read: %s (%d bytes, checksum %08x)",
                entryName,
                size,
                checksum);
        free(data);
    }
    archive.closeZipFile();
    return 1;
}
