#include "Filesystem.h"

#include <LittleFS.h>

#include "Config.h"

// Gestione di montaggio del filesystem
bool mountingFS(char* s) {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, s)) {
    return 0;
  } else {
    return 1;
  }
}

void openFS() {
  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  LittleFS.remove(DB_FILE);
}