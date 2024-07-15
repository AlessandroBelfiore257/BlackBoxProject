#include "FileSystem.h"

#include <LittleFS.h>

#include "Config.h"

bool mountingFS(char* s) {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, s)) {
    return 0;
  } 
  return 1;
}

void openFS() {
  File root = LittleFS.open("/");
  if (!root) {
    #ifdef DEBUG
    Serial.println("- failed to open directory");
    #endif
    return;
  }
  if (!root.isDirectory()) {
    #ifdef DEBUG
    Serial.println(" - not a directory");
    #endif
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      #ifdef DEBUG
      Serial.print("  DIR : ");
      Serial.println(file.name());
      #endif
    } else {
      #ifdef DEBUG
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
      #endif
    }
    file = root.openNextFile();
  }
  LittleFS.remove(DB_FILE);
}