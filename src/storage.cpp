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
