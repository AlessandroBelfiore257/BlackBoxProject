#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>
#include "LCtime.h"

extern sqlite3 *db;

void initializeStorage();
int db_open(const char *filename, sqlite3 **db);
int db_exec(sqlite3 *db, const char *sql);
void createRecordToInsertIntot1(char* name_sensor, int value, long long int ts, int synch, int pr);

#endif
