#ifdef WITH_FILEMANAGER
#include <ESPFMfGK.h>
#endif

#include "storage.h"

bool Storage::init() {
    static bool initialized = false;

    if (initialized) return true;

    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        Serial.println(F("LittleFS mount failed"));
        return false;
    } else {
        Serial.println(F("LittleFS mounted OK"));
        initialized = true;

#ifdef WITH_FILEMANAGER
        FileManager::addFileSystems();
#endif
        return true;
    }
}

bool Storage::exists(const String& path) {
    return LittleFS.exists(path);
}

bool Storage::remove(const String& path) {
    return LittleFS.remove(path);
}

String Storage::read(const String& path) {
    File file = LittleFS.open(path);
    if (!file || file.isDirectory()) {
        Serial.println(F("Failed to open file for reading"));
        return "";
    }

    const String data = file.readString();
    file.close();

    return data;
}

String Storage::readLine(const String& path) {
    static File file;
    if (String(file.path()) != path) {
        file.close();
    }

    if (!file) {
        file = LittleFS.open(path);
        if (!file || file.isDirectory()) {
            Serial.println(F("Failed to open file for reading"));
            return "";
        }
    }

    if (file.available()) {
        return file.readStringUntil('\n');
    }

    file.close();

    return "";
}

bool Storage::write(const String& path, const String& data) {
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.println(F("Failed to open file for writing"));
        return false;
    }

    if (file.print(data)) {
        file.close();
        return true;
    } else {
        file.close();
        return false;
    }
}

bool Storage::append(const String& path, const String& data) {
    File file = LittleFS.open(path, FILE_APPEND);
    if (!file) {
        Serial.println(F("Failed to open file for appending"));
        return false;
    }

    if (file.print(data)) {
        file.close();
        return true;
    } else {
        file.close();
        return false;
    }
}

uint32_t Storage::countLines(const String& path) {
    File file = LittleFS.open(path);
    if (!file || file.isDirectory()) {
        Serial.println(F("Failed to open file for reading"));
        return 0;
    }

    uint32_t lines = 0;
    while (file.available()) {
        // Read a line
        lines++;
        file.readStringUntil('\n');
    }
    return lines;
}

#ifdef WITH_FILEMANAGER

const word filemanagerport = 8080;
ESPFMfGK filemgr(filemanagerport);

void FileManager::addFileSystems() {
    if (!filemgr.AddFS(LittleFS, "Little FS", false)) {
        Serial.println(F("Adding Little FS failed."));
    }
}

uint32_t FileManager::checkFileFlags(fs::FS& fs, String filename, uint32_t flags) {
    // this will hide system files (in my world, system files start with a dot)
    if (filename.startsWith("/.")) {
        // no other flags, file is invisible and nothing allowed
        return ESPFMfGK::flagIsNotVisible;
    }

    // Checks if target file name is valid for action. This will simply allow everything by returning the queried flag
    if (flags & ESPFMfGK::flagIsValidAction) {
        return flags & (~ESPFMfGK::flagIsValidAction);
    }

    // Checks if target file name is valid for action.
    if (flags & ESPFMfGK::flagIsValidTargetFilename) {
        return flags & (~ESPFMfGK::flagIsValidTargetFilename);
    }

    // Default actions
    uint32_t defaultflags = ESPFMfGK::flagCanDelete | ESPFMfGK::flagCanRename | ESPFMfGK::flagCanGZip |  // ^t
                            ESPFMfGK::flagCanDownload | ESPFMfGK::flagCanUpload;                         // ^t

    // editable files.
    const String extedit[] PROGMEM = {".html", ".css", ".js", ".txt", ".json", ".ino"};

    filename.toLowerCase();
    // I simply assume, that editable files are also allowed to be previewd
    for (int i = 0; i < sizeof(extedit) / sizeof(extedit[0]); i++) {
        if (filename.endsWith(String(extedit[i]))) {
            defaultflags |= ESPFMfGK::flagCanEdit | ESPFMfGK::flagAllowPreview;
            break;
        }
    }

    const String extpreview[] PROGMEM = {".jpg", ".png"};
    for (int i = 0; i < sizeof(extpreview) / sizeof(extpreview[0]); i++) {
        if (filename.endsWith(String(extpreview[i]))) {
            defaultflags |= ESPFMfGK::flagAllowPreview;
            break;
        }
    }

    return defaultflags;
}

void FileManager::setupFilemanager() {
    static bool initialized = false;

    if (initialized) return;

    filemgr.checkFileFlags = checkFileFlags;

    filemgr.WebPageTitle = "FileManager";
    filemgr.BackgroundColor = "white";
    filemgr.textareaCharset = "accept-charset=\"utf-8\"";

    if (/*(WiFi.status() == WL_CONNECTED) &&*/ (filemgr.begin())) {
        Serial.print(F("Open Filemanager with http://"));
        Serial.print(WiFi.localIP());
        Serial.print(F(":"));
        Serial.print(filemanagerport);
        Serial.print(F("/"));
        Serial.println();
        initialized = true;
    } else {
        //Serial.print(F("Filemanager: did not start"));
    }
}

void FileManager::loop() {
    // Check the file manager
    filemgr.handleClient();
}
#endif
