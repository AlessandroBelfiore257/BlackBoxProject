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
  Descrizione: creazione della tabella t2 usata per la politica di storage relativa ai dati di monitoraggio
  Input: -
  Return: void
*/
void createPolicyStorage() {
  int rc = db_exec(db, "CREATE TABLE t2 (nome_sensore TEXT, tempo_storage INTEGER, variabile INTEGER, fattore_incr_decr INTEGER, soglia INTEGER, tMinStorage INTEGER, tMaxStorage INTEGER);");
  char buffer1[BUFFER_SIZE / 2];
  char buffer2[BUFFER_SIZE / 2];
  char buffer3[BUFFER_SIZE / 2];
  char buffer4[BUFFER_SIZE / 2];
  char buffer5[BUFFER_SIZE / 2];
  char buffer6[BUFFER_SIZE / 2];
  char buffer7[BUFFER_SIZE / 2];
  char buffer8[BUFFER_SIZE / 2];
  char buffer9[BUFFER_SIZE / 2];
  sprintf(buffer1, "INSERT INTO t2 VALUES ('Accelerometria', 15, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer1);
  sprintf(buffer2, "INSERT INTO t2 VALUES ('Alcool', 20, 1, 10, 0.25, 20, 120);");
  rc = db_exec(db, buffer2);
  sprintf(buffer3, "INSERT INTO t2 VALUES ('Qualità aria', 20, 1, 10, 0.25, 20, 120);");
  rc = db_exec(db, buffer3);
  sprintf(buffer4, "INSERT INTO t2 VALUES ('Coordinate GPS', 30, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer4);
  sprintf(buffer5, "INSERT INTO t2 VALUES ('Liquido raffreddamento', 45, 1, '15', '0.25', '45', '600');");
  rc = db_exec(db, buffer5);
  sprintf(buffer6, "INSERT INTO t2 VALUES ('Olio motore temp', 45, 1, '15', '0.25', '45', '600');");
  rc = db_exec(db, buffer6);
  sprintf(buffer7, "INSERT INTO t2 VALUES ('Coppia motore', 45, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer7);
  sprintf(buffer8, "INSERT INTO t2 VALUES ('Voltaggio batteria', 45, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer8);
  sprintf(buffer9, "INSERT INTO t2 VALUES ('Livello carburante', 45, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer9);
  rc = db_exec(db, "SELECT * FROM t2");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}

/*
  Descrizione: creazione della tabella t3 relativa alla profilazione guida del conducente
  Input: -
  Return: void
*/
void createProfilazioneStorage() {
  int rc = db_exec(db, "CREATE TABLE t3 (data TEXT, voto TEXT, lastStorage INTEGER, synchronised INTEGER);");
  char buffer1[BUFFER_SIZE / 2];
  sprintf(buffer1, "INSERT INTO t3 VALUES ('2024-03-10 12:00:00', '0.99', 0, 0);");
  rc = db_exec(db, buffer1);
  rc = db_exec(db, "SELECT * FROM t3");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}

/*
  Descrizione: creazione della tabella t4 relativa al rilevamento degli allarmi più critici
  Input: -
  Return: void
*/
void createSegnalazioneAllarmiRedStorage() {
  int rc = db_exec(db, "CREATE TABLE t4 (problema TEXT, data TEXT, synchronised INTEGER);");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}

/*
  Descrizione: creazione della tabella t5 relativa al rilevamento degli allarmi meno critici
  Input: -
  Return: void
*/
void createSegnalazioneAllarmiYellowStorage() {
  int rc = db_exec(db, "CREATE TABLE t5 (problema TEXT, rilevamenti INTEGER, soglia INTEGER, daMandare INTEGER, synchronised INTEGER);");
  char buffer1[BUFFER_SIZE / 2];
  char buffer2[BUFFER_SIZE / 2];
  char buffer3[BUFFER_SIZE / 2];
  char buffer4[BUFFER_SIZE / 2];
  sprintf(buffer1, "INSERT INTO t5 VALUES ('Fumo', 0, 3, 0, 0);");
  rc = db_exec(db, buffer1);
  sprintf(buffer2, "INSERT INTO t5 VALUES ('Fuori strada', 0, 2, 0, 0);");
  rc = db_exec(db, buffer2);
  sprintf(buffer3, "INSERT INTO t5 VALUES ('Fuori range GPS', 0, 3, 0, 0);");
  rc = db_exec(db, buffer3);
  sprintf(buffer4, "INSERT INTO t5 VALUES ('Trasporto/rimorchio', 0, 2, 0, 0);");
  rc = db_exec(db, buffer4);
  rc = db_exec(db, "SELECT * FROM t5");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}

void initializeStorage(char *path) {
  sqlite3_initialize();
  if (db_open(path, &db))
    return;
  int rc = db_exec(db, "CREATE TABLE t1 (nome_sensore TEXT, valore TEXT, tipo_dato TEXT, unita_misura TEXT, timestamp INTEGER, synchronised INTEGER, priority INTEGER);");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  createPolicyStorage();
  createProfilazioneStorage();
  createSegnalazioneAllarmiRedStorage();
  createSegnalazioneAllarmiYellowStorage();
  sqlite3_close(db);
}

void createRecordToInsertIntoT1(char *name_sensor, const char *value, char *data_type, char *unit_of_measure, long long int ts, int synch, int pr) {
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
  Serial.println("DB/t1:");
  rc = db_exec(db, "SELECT * FROM t1");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  sqlite3_close(db);
}

void createRecordToInsertIntoT3(const char *date, const char *normalizedNumber, long long int lastStorage, int synchronised) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  char buffer[BUFFER_SIZE];
  sprintf(buffer, "INSERT INTO t3 VALUES ('%s', '%s', %lld, %d);", date, normalizedNumber, lastStorage, synchronised);
  int rc = db_exec(db, buffer);
  Serial.println("DB/t3:");
  rc = db_exec(db, "SELECT * FROM t3");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  sqlite3_close(db);
}

long long int getTempoStorageFromT2(sqlite3 *db, const char *nome_sensore) {
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

bool getSensorParametersFromT2(const char *sensorName, int &tempoStorage, int &fattoreIncrDecr, float &soglia, int &tMinStorage, int &tMaxStorage) {
  sqlite3_stmt *stmt;
  char sql[128];
  snprintf(sql, sizeof(sql), "SELECT tempo_storage, fattore_incr_decr, soglia, tMinStorage, tMaxStorage FROM t2 WHERE nome_sensore = '%s';", sensorName);
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return false;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      tempoStorage = sqlite3_column_int(stmt, 0);
      fattoreIncrDecr = sqlite3_column_int(stmt, 1);
      soglia = sqlite3_column_double(stmt, 2);
      tMinStorage = sqlite3_column_int(stmt, 3);
      tMaxStorage = sqlite3_column_int(stmt, 4);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return true;
    }
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return false;
}

void updateTempoStorageIntoT2(const char *sensorName, int newTempoStorage) {
  char sql[128];
  Serial.println("Sto cambiando il tempo di storage -------------------------------");
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  snprintf(sql, sizeof(sql), "UPDATE t2 SET tempo_storage = %d WHERE nome_sensore = '%s';", newTempoStorage, sensorName);
  int rc = db_exec(db, sql);
  sqlite3_close(db);
}

long long int getLastTimestampFromT3(sqlite3 *db) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT MAX(lastStorage) FROM t3";
  if (sqlite3_open(PATH_STORAGE_DATA_DB, &db) != SQLITE_OK) {
    Serial.printf("Failed to open database: %s\n", sqlite3_errmsg(db));
    return 0;
  }
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 0;
  }
  long long int lastTimestamp = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    lastTimestamp = sqlite3_column_int64(stmt, 0);
  } else {
    Serial.printf("Failed to step statement: %s\n", sqlite3_errmsg(db));
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return lastTimestamp;
}

void createRecordToInsertIntoT4(const char *problem, const char *date, int synchronised) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  char buffer[BUFFER_SIZE];
  sprintf(buffer, "INSERT INTO t4 VALUES ('%s', '%s', %d);", problem, date, synchronised);
  int rc = db_exec(db, buffer);
  Serial.println("DB/t4:");
  rc = db_exec(db, "SELECT * FROM t4");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  sqlite3_close(db);
}

bool alreadyExistsIntoT4(sqlite3 *db, const char *problem) {
  sqlite3_stmt *stmt = nullptr;
  bool exists = false;
  if (sqlite3_open(PATH_STORAGE_DATA_DB, &db) != SQLITE_OK) {
    fprintf(stderr, "Impossibile aprire il database: %s\n", sqlite3_errmsg(db));
    return false;
  }
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM t4 WHERE problema = '%s'", problem);
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    fprintf(stderr, "Errore durante la preparazione dello statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return false;
  }
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    exists = (count > 0);
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return exists;
}

int getValueFromT5(const char *problem, const char *column) {
  if (db_open(PATH_STORAGE_DATA_DB, &db) != SQLITE_OK) {
    return -1;
  }
  char sql[256];
  sqlite3_stmt *stmt;
  snprintf(sql, sizeof(sql), "SELECT %s FROM t5 WHERE problema = ?;", column);
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return -1;
  }
  if (sqlite3_bind_text(stmt, 1, problem, -1, SQLITE_STATIC) != SQLITE_OK) {
    Serial.printf("Failed to bind value: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return -1;
  }
  int rc = sqlite3_step(stmt);
  int value = -1;
  if (rc == SQLITE_ROW) {
    value = sqlite3_column_int(stmt, 0);
  } else if (rc == SQLITE_DONE) {
    Serial.println("No record found.");
  } else {
    Serial.printf("Failed to execute query: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return value;
}

void updateColumnValueIntoT5(const char *problem, const char *column, int value) {
  int rc = db_open(PATH_STORAGE_DATA_DB, &db);
  if (rc != SQLITE_OK) {
    return;
  }
  char sql[256];
  sqlite3_stmt *stmt;
  snprintf(sql, sizeof(sql), "UPDATE t5 SET %s = ? WHERE problema = ?;", column);
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }
  sqlite3_bind_int(stmt, 1, value);
  sqlite3_bind_text(stmt, 2, problem, -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("Failed to update record: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Record updated successfully.");
  }
  rc = db_exec(db, "SELECT * FROM t5");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
}