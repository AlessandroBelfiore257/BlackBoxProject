#ifndef SYNCH_H
#define SYNCH_H

#include <Arduino.h>
#include <WiFi.h>
#include <sqlite3.h>
#include <vector>

void syncData(sqlite3* db);
void syncT3Data(sqlite3* db);
void syncT4Data(sqlite3* db);
void syncT5Data(sqlite3* db);

#endif