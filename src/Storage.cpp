#include "Storage.h"

#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <LittleFS.h>

#include "Config.h"

extern sqlite3 *db;

static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("%s: ", (const char *)data);
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

void createPolicyStorage() {
  /*
  int rc = db_exec(db, "CREATE TABLE t2 (nome_sensore TEXT, frequency INTEGER);");
  char buffer1[BUFFER_SIZE];
  char buffer2[BUFFER_SIZE];
  sprintf(buffer1, "INSERT INTO t2 VALUES ('Button pressure', 60);");
  rc = db_exec(db, buffer1);
  sprintf(buffer2, "INSERT INTO t2 VALUES ('Casual number', 120);");
  rc = db_exec(db, buffer2);
  rc = db_exec(db, "SELECT * FROM t2");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  } */
  Serial.println("Si vedrà se memorizzare su un file di config o su una tabella");
}

void initializeStorage(char* path) {
  sqlite3_initialize();
  if (db_open(path, &db))
    return;
  // Creazione tabella principale di storage
  int rc = db_exec(db, "CREATE TABLE t1 (nome_sensore TEXT, valore TEXT, tipo_dato TEXT, unita_misura TEXT, timestamp INTEGER, synchronised INTEGER, priority INTEGER);");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  createPolicyStorage();
  sqlite3_close(db);
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
  return rc;
}

/*
  Creazione record e inserimento nella tabella t1 
*/
void createRecordToInsertIntot1(char* name_sensor, const char* value, char* data_type, char* unit_of_measure, long long int ts, int synch, int pr) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, sizeof(buffer));
  Serial.print("Not buffered: ");
  Serial.println(buffer);
  sprintf(buffer, "INSERT INTO t1 VALUES ('%s', '%s', '%s', '%s', %lld, %d, %d);", name_sensor, value, data_type, unit_of_measure, ts, synch, pr);
  Serial.print("Buffered: ");
  Serial.println(buffer);
  int rc = db_exec(db, buffer);
  // Debugging
  Serial.println("DB/t1:");
    rc = db_exec(db, "SELECT * FROM t1");
    if (rc != SQLITE_OK) {
      sqlite3_close(db);
      return;
    }
  sqlite3_close(db);
}