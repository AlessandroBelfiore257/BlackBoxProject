#ifndef SYNCH_H
#define SYNCH_H

#include <Arduino.h>
#include <WiFi.h>
#include <sqlite3.h>
#include <vector>

void getOrderedRecords(sqlite3* db, std::vector<std::vector<String>>& records);
void updateSyncStatus(sqlite3* db, const std::vector<String>& record);
long long int getTimeNowInSeconds();
bool shouldSync();
void SendToServer(String name_sensor, String value, String timestamp, String synchronized, String priority);
void syncData(sqlite3* db);

#endif