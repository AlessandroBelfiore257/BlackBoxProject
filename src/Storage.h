#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>
#include "LCtime.h"

extern sqlite3 *db;
extern struct tm timeinfo;

static int callback(void *data, int argc, char **argv, char **azColName);
int db_open(const char *filename, sqlite3 **db);
int db_exec(sqlite3 *db, const char *sql);
void initializeStorage();
struct tm getDateAndTime();

#endif
