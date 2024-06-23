#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>
#include "LCtime.h"

void initializeStorage(char* path);
int db_open(const char *filename, sqlite3 **db);
int db_exec(sqlite3 *db, const char *sql);
void createRecordToInsertIntot1(char* name_sensor, const char* value, char* data_type, char* unit_of_measure, long long int ts, int synch, int pr);
long long int getTempoStorage(sqlite3* db, const char* nome_sensore);
bool getSensorParameters(const char* sensorName, int& tempoStorage, int& fattoreIncrDecr, float& soglia, int& tMinStorage, int& tMaxStorage);
void updateTempoStorage(const char* sensorName, int newTempoStorage);

#endif