#include "storage.h"
#include <SPI.h>
#include <FS.h>
#include <LittleFS.h>
#include <Arduino.h>

#include "Config.h"

extern char buffer[BUFFER_SIZE];
sqlite3 *db;
struct tm timeinfo;

static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("%s: ", (const char *)data);
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

int db_open(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

int db_exec(sqlite3 *db, const char *sql) {
  Serial.println(sql);
  long start = micros();
  char *zErrMsg = 0;
  int rc = sqlite3_exec(db, sql, callback, (void *)"Callback function called", &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros() - start);
  return rc;
}

void initializeStorage() {
  sqlite3_initialize();
  if (db_open("/littlefs/dati.db", &db))
    return;

  int rc = db_exec(db, "CREATE TABLE t1 (nome_sensore TEXT, valore REAL, timestamp TEXT, synchronised INTEGER, priority INTEGER);");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  createPolicyStorage();

  sqlite3_close(db);
}

void createPolicyStorage() {
  int rc = db_exec(db, "CREATE TABLE t2 (nome_sensore TEXT, frequency INTEGER);");
  sprintf(buffer, "INSERT INTO t2 VALUES ('Button pressure', 60000);");
  rc = db_exec(db, buffer);
  sprintf(buffer, "INSERT INTO t2 VALUES ('Casual number', 120000);");
  rc = db_exec(db, buffer);
  rc = db_exec(db, "SELECT * FROM t2");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}
