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
  int rc = db_exec(db, "CREATE TABLE t2 (nome_sensore TEXT, tempo_storage INTEGER, variabile INTEGER, fattore_incr_decr INTEGER, soglia INTEGER, tMinStorage INTEGER, tMaxStorage INTEGER);");
  char buffer1[BUFFER_SIZE/2];
  char buffer2[BUFFER_SIZE/2];
  char buffer3[BUFFER_SIZE/2];
  char buffer4[BUFFER_SIZE/2];
  char buffer5[BUFFER_SIZE/2];
  char buffer6[BUFFER_SIZE/2];
  char buffer7[BUFFER_SIZE/2];
  sprintf(buffer1, "INSERT INTO t2 VALUES ('Button pressure', 10, 1, 10, 0.25, 10, 30);");
  rc = db_exec(db, buffer1);
  sprintf(buffer2, "INSERT INTO t2 VALUES ('Coordinate GPS', 30, 0, 'NULL', 'NULL', 'NULL', 'NULL');"); 
  rc = db_exec(db, buffer2);
  sprintf(buffer3, "INSERT INTO t2 VALUES ('Accelerometria', 20, 0, 'NULL', 'NULL', 'NULL', 'NULL');"); 
  rc = db_exec(db, buffer3);
  sprintf(buffer4, "INSERT INTO t2 VALUES ('Alcool', 25, 1, 10, 0.25, 25, 125);");
  rc = db_exec(db, buffer4);
  sprintf(buffer5, "INSERT INTO t2 VALUES ('Air quality', 25, 1, 10, 0.25, 25, 150);");
  rc = db_exec(db, buffer5);
  sprintf(buffer6, "INSERT INTO t2 VALUES ('rpm', 15, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer6);
  sprintf(buffer7, "INSERT INTO t2 VALUES ('liquido', 15, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer7);
  rc = db_exec(db, "SELECT * FROM t2");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  Serial.println("Si vedrà se memorizzare su un file di config o su una tabella");
}

void initializeStorage(char *path) {
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
void createRecordToInsertIntot1(char *name_sensor, const char *value, char *data_type, char *unit_of_measure, long long int ts, int synch, int pr) {
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

long long int getTempoStorage(sqlite3 *db, const char *nome_sensore) {
  sqlite3_stmt *stmt;
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT tempo_storage, variabile FROM t2 WHERE nome_sensore = '%s'", nome_sensore);

  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return 0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return 0;
  }

  long long int tempo_storage = -1;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int variabile = sqlite3_column_int(stmt, 1);
    if (variabile == 1) {
      tempo_storage = sqlite3_column_int64(stmt, 0);
    } else {
      tempo_storage = -1;
    }
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return tempo_storage;
}

bool getSensorParameters(const char* sensorName, int& tempoStorage, int& fattoreIncrDecr, float& soglia, int& tMinStorage, int& tMaxStorage) {
  sqlite3_stmt* stmt;  // Dichiarazione della variabile stmt
  char sql[128];
  snprintf(sql, sizeof(sql), "SELECT tempo_storage, fattore_incr_decr, soglia, tMinStorage, tMaxStorage FROM t2 WHERE nome_sensore = '%s';", sensorName);

  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return false;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);  // Ottenere il puntatore stmt tramite sqlite3_prepare_v2

  if (rc == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      tempoStorage = sqlite3_column_int(stmt, 0);
      fattoreIncrDecr = sqlite3_column_int(stmt, 1);
      soglia = sqlite3_column_double(stmt, 2);  // Modifica qui per ottenere un valore float
      tMinStorage = sqlite3_column_int(stmt, 3);
      tMaxStorage = sqlite3_column_int(stmt, 4);
      sqlite3_finalize(stmt);  // Rilascio delle risorse allocate
      sqlite3_close(db);
      return true;
    }
  }
  sqlite3_finalize(stmt);  // Rilascio delle risorse allocate
  sqlite3_close(db);
  return false;
}

void updateTempoStorage(const char* sensorName, int newTempoStorage) {
  char sql[128];
  Serial.println("Sto cambiando il tempo di storage -------------------------------");
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  snprintf(sql, sizeof(sql), "UPDATE t2 SET tempo_storage = %d WHERE nome_sensore = '%s';", newTempoStorage, sensorName);
  int rc = db_exec(db, sql);
  sqlite3_close(db);
}