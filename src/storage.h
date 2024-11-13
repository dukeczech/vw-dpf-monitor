#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

class Storage {
   public:
    static bool init();

    static bool exists(const String& path);
    static bool remove(const String& path);
    static String read(const String& path);
    static String readLine(const String& path);
    static bool write(const String& path, const String& data);
    static bool append(const String& path, const String& data);

    static uint32_t countLines(const String& path);

   protected:
};

#ifdef WITH_FILEMANAGER
class FileManager {
   public:
    static void addFileSystems();
    static uint32_t checkFileFlags(fs::FS& fs, String filename, uint32_t flags);
    static void setupFilemanager();
    static void loop();
};
#endif