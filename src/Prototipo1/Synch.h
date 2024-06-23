#ifndef SYNCH_H
#define SYNCH_H

#include <Arduino.h>
#include <WiFi.h>
#include <sqlite3.h>
#include <vector>

void syncData(sqlite3* db);

#endif