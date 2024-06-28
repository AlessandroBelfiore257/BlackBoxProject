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
  char buffer1[BUFFER_SIZE / 2];
  char buffer2[BUFFER_SIZE / 2];
  char buffer3[BUFFER_SIZE / 2];
  char buffer4[BUFFER_SIZE / 2];
  sprintf(buffer1, "INSERT INTO t2 VALUES ('Accelerometria', 15, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer1);
  sprintf(buffer2, "INSERT INTO t2 VALUES ('Alcool', 20, 1, 10, 0.25, 20, 120);");
  rc = db_exec(db, buffer2);
  sprintf(buffer3, "INSERT INTO t2 VALUES ('Qualità aria', 20, 1, 10, 0.25, 20, 120);");
  rc = db_exec(db, buffer3);
  sprintf(buffer4, "INSERT INTO t2 VALUES ('Coordinate GPS', 30, 0, 'NULL', 'NULL', 'NULL', 'NULL');");
  rc = db_exec(db, buffer4);
  rc = db_exec(db, "SELECT * FROM t2");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}

// Creata tabella relativa alla profilazione
void createProfilazioneStorage() {
  int rc = db_exec(db, "CREATE TABLE t3 (data TEXT, voto TEXT, lastStorage INTEGER, synchronised INTEGER);");
  char buffer1[BUFFER_SIZE / 2];
  sprintf(buffer1, "INSERT INTO t3 VALUES ('2024-03-10 12:00:00', '0.27', 0, 0);");
  rc = db_exec(db, buffer1);
  rc = db_exec(db, "SELECT * FROM t3");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
}

// Creata tabella relativa alla segnalazione di allarmi poco rilevanti
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

// Creata tabella relativa alla segnalazione di allarmi critici
void createSegnalazioneAllarmiRedStorage() {
  int rc = db_exec(db, "CREATE TABLE t4 (problema TEXT, data TEXT, synchronised INTEGER);");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
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
  createProfilazioneStorage();
  createSegnalazioneAllarmiYellowStorage();
  createSegnalazioneAllarmiRedStorage();
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

void createRecordToInsertIntot3(const char *date, const char *normalizedNumber, long long int lastStorage, int synchronised) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  char buffer[BUFFER_SIZE];
  // Assumiamo che db_exec sia una funzione che esegue query SQL
  sprintf(buffer, "INSERT INTO t3 VALUES ('%s', '%s', %lld, %d);", date, normalizedNumber, lastStorage, synchronised);
  int rc = db_exec(db, buffer);
  // Debugging
  Serial.println("DB/t3:");
  rc = db_exec(db, "SELECT * FROM t3");
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

bool getSensorParameters(const char *sensorName, int &tempoStorage, int &fattoreIncrDecr, float &soglia, int &tMinStorage, int &tMaxStorage) {
  sqlite3_stmt *stmt;  // Dichiarazione della variabile stmt
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

void updateTempoStorage(const char *sensorName, int newTempoStorage) {
  char sql[128];
  Serial.println("Sto cambiando il tempo di storage -------------------------------");
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  snprintf(sql, sizeof(sql), "UPDATE t2 SET tempo_storage = %d WHERE nome_sensore = '%s';", newTempoStorage, sensorName);
  int rc = db_exec(db, sql);
  sqlite3_close(db);
}

long long int getLastTimestampFromt3(sqlite3 *db) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT MAX(lastStorage) FROM t3";
  // Apri il database, assicurandoti che 'db' sia un puntatore a una connessione valida
  if (sqlite3_open(PATH_STORAGE_DATA_DB, &db) != SQLITE_OK) {
    Serial.printf("Failed to open database: %s\n", sqlite3_errmsg(db));
    return 0;
  }
  // Prepara la query SQL
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 0;
  }
  long long int lastTimestamp = 0;
  // Esegui la query e ottieni il risultato
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    lastTimestamp = sqlite3_column_int64(stmt, 0);
  } else {
    Serial.printf("Failed to step statement: %s\n", sqlite3_errmsg(db));
  }
  // Finalizza la dichiarazione e chiudi il database
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return lastTimestamp;
}

void createRecordToInsertIntot4(const char *problema, const char *date, int synchronised) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  char buffer[BUFFER_SIZE];
  // Assumiamo che db_exec sia una funzione che esegue query SQL
  sprintf(buffer, "INSERT INTO t4 VALUES ('%s', '%s', %d);", problema, date, synchronised);
  int rc = db_exec(db, buffer);
  // Debugging
  Serial.println("DB/t4:");
  rc = db_exec(db, "SELECT * FROM t4");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  sqlite3_close(db);
}

bool alreadyExists(sqlite3 *db, const char *problema) {
  sqlite3_stmt *stmt = nullptr;
  bool exists = false;

  if (sqlite3_open(PATH_STORAGE_DATA_DB, &db) != SQLITE_OK) {
    fprintf(stderr, "Impossibile aprire il database: %s\n", sqlite3_errmsg(db));
    return false;
  }
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM t4 WHERE problema = '%s'", problema);
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

// trattazione allarmi led giallo
// Funzione per ottenere un valore dalla tabella t5
// Funzione per ottenere il valore da una colonna specifica nella tabella t5
int getValueFromT5(const char *problema, const char *column) {
  if (db_open(PATH_STORAGE_DATA_DB, &db) != SQLITE_OK) {
    return -1;  // Indica errore nell'apertura del database
  }
  char sql[256];
  sqlite3_stmt *stmt;
  // Costruisce la query SQL
  snprintf(sql, sizeof(sql), "SELECT %s FROM t5 WHERE problema = ?;", column);
  // Prepara la query SQL
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return -1;  // Indica errore nella preparazione della query
  }
  // Bind del valore del problema al primo placeholder
  if (sqlite3_bind_text(stmt, 1, problema, -1, SQLITE_STATIC) != SQLITE_OK) {
    Serial.printf("Failed to bind value: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return -1;  // Indica errore nel binding del valore
  }
  // Esegue la query
  int rc = sqlite3_step(stmt);
  int value = -1;  // Valore di default in caso di errore o assenza di record

  if (rc == SQLITE_ROW) {
    // Estrai il valore dalla colonna specificata
    value = sqlite3_column_int(stmt, 0);
  } else if (rc == SQLITE_DONE) {
    // Nessun record trovato
    Serial.println("No record found.");
  } else {
    // Errore durante l'esecuzione della query
    Serial.printf("Failed to execute query: %s\n", sqlite3_errmsg(db));
  }

  // Finalizza la query e chiude il database
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  return value;  // Ritorna il valore della colonna o -1 in caso di errore
}

// Funzione per aggiornare un valore intero in una colonna specificata nella tabella t5 per un dato problema
void updateColumnValue(const char *problema, const char *column, int value) {
  int rc = db_open(PATH_STORAGE_DATA_DB, &db);
  if (rc != SQLITE_OK) {
    return;  // Indica errore nell'apertura del database
  }
  char sql[256];
  sqlite3_stmt *stmt;

  // Costruisce la query SQL per l'aggiornamento
  snprintf(sql, sizeof(sql), "UPDATE t5 SET %s = ? WHERE problema = ?;", column);

  // Prepara la query SQL
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;  // Indica errore nella preparazione della query
  }

  // Bind del valore intero al primo placeholder
  sqlite3_bind_int(stmt, 1, value);

  // Bind del problema al secondo placeholder
  sqlite3_bind_text(stmt, 2, problema, -1, SQLITE_STATIC);

  // Esegue la query
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

  // Finalizza la query e chiude il database
  sqlite3_finalize(stmt);
  sqlite3_close(db);
}