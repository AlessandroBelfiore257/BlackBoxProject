#include "Garbage.h"

extern sqlite3 *db;

int countRecordsInTableT(sqlite3 *db) {
  int count = 0;
  char *sql = "SELECT COUNT(*) FROM t;";
  char **result;
  int rows, columns;
  char *errMsg = 0;

  int rc = sqlite3_get_table(db, sql, &result, &rows, &columns, &errMsg);
  if (rc == SQLITE_OK) {
    // Il risultato della query è memorizzato in result[1]
    // Converto il valore restituito da stringa a intero
    count = atoi(result[1]);
    sqlite3_free_table(result);
  } else {
    Serial.print("SQL error: ");
    Serial.println(errMsg);
    sqlite3_free(errMsg);
  }

  return count;
}

void deleteFirstXRecords(sqlite3 *db, int x) {
  char selectSql[BUFFER_SIZE];
  char deleteSql[BUFFER_SIZE];
  snprintf(selectSql, sizeof(selectSql), "SELECT rowid FROM t ORDER BY timestamp ASC LIMIT %d;", x);

  // Iniziamo una transazione
  int rc = db_exec(db, "BEGIN TRANSACTION;");
  if (rc != SQLITE_OK) {
    Serial.println("Error starting transaction.");
    return;
  }

  // Creiamo un buffer per memorizzare i rowid da eliminare
  std::vector<int> rowids;
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, selectSql, -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int rowid = sqlite3_column_int(stmt, 0);
      rowids.push_back(rowid);
    }
    sqlite3_finalize(stmt);
  } else {
    Serial.println("Error selecting rowids.");
    db_exec(db, "ROLLBACK;");
    return;
  }

  // Costruiamo la query di eliminazione
  if (!rowids.empty()) {
    String ids;
    for (int id : rowids) {
      ids += String(id) + ",";
    }
    ids.remove(ids.length() - 1);  // Rimuove l'ultima virgola

    snprintf(deleteSql, sizeof(deleteSql), "DELETE FROM t WHERE rowid IN (%s);", ids.c_str());

    // Eseguiamo la query di eliminazione
    rc = db_exec(db, deleteSql);
    if (rc != SQLITE_OK) {
      Serial.println("Error deleting records.");
      db_exec(db, "ROLLBACK;");
    } else {
      rc = db_exec(db, "COMMIT;");
      if (rc != SQLITE_OK) {
        Serial.println("Error committing changes.");
        db_exec(db, "ROLLBACK;");
      } else {
        Serial.println("Records deleted successfully.");
      }
    }
  } else {
    Serial.println("No records to delete.");
    db_exec(db, "ROLLBACK;");
  }
}

void garbageCollectorRoutine() {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;

  int rc = db_exec(db, "BEGIN TRANSACTION;");
  if (rc != SQLITE_OK) {
    Serial.println("Error starting transaction.");
    sqlite3_close(db);
    return;
  }

  char *sql = "DELETE FROM t1 WHERE synchronised = 1;";
  rc = db_exec(db, sql);
  if (rc != SQLITE_OK) {
    Serial.println("Error cleaning synced data.");
    db_exec(db, "ROLLBACK;");
  } else {
    rc = db_exec(db, "COMMIT;");
    if (rc != SQLITE_OK) {
      Serial.println("Error committing changes.");
      db_exec(db, "ROLLBACK;");
    } else {
      Serial.println("Synced data cleaned successfully.");

      // Utilizza la query VACUUM per compattare il database e ottimizzare lo spazio su disco
      rc = db_exec(db, "VACUUM;");
      if (rc != SQLITE_OK) {
        Serial.println("Error vacuuming database.");
      } else {
        Serial.println("Database vacuumed successfully.");
      }

      rc = db_exec(db, "PRAGMA wal_checkpoint(FULL);");
      if (rc != SQLITE_OK) {
        Serial.println("Error checkpointing WAL.");
      }

      rc = db_exec(db, "PRAGMA shrink_memory;");
      if (rc != SQLITE_OK) {
        Serial.println("Error shrinking memory.");
      }
    }
  }

  sqlite3_close(db);
}

void garbageCollectorMemoryFull() {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;

  int numRecordPre = countRecordsInTableT(db);
  int recordToDelete = numRecordPre / 2;
  Serial.print("Record totali: ");
  Serial.println(numRecordPre);
  Serial.print("Record da eliminare: ");
  Serial.println(recordToDelete);
  // In primis elimino i record già sincronizzati
  garbageCollectorRoutine();
  if (db_open("/littlefs/dati.db", &db))
    return;
  int numRecordPost = countRecordsInTableT(db);
  // Sono i record rimanti da eliminare
  int res = numRecordPost - recordToDelete;
  Serial.print("Record rimasti: ");
  Serial.println(numRecordPost);
  Serial.print("Record rimasti da eliminare: ");
  Serial.println(res);
  deleteFirstXRecords(db, res);
  sqlite3_close(db);
}

// Profilazione
int countSyncedRecordsInT3(sqlite3* db) {
  const char* sql = "SELECT COUNT(*) FROM t3 WHERE synchronised = 1";
  sqlite3_stmt* stmt;
  int count = 0;
  if (db_open(PATH_STORAGE_DATA_DB, &db)) {
    Serial.println("Failed to open the database.");
    return 0;
  }
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 0;
  }
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  Serial.printf("Number of synced records: %d\n", count);
  return count;
}

void deleteSyncedRecordsFromT3(sqlite3* db) {
  const char* sql = "DELETE FROM t3 WHERE synchronised = 1";
  
  if (db_open(PATH_STORAGE_DATA_DB, &db)) {
    Serial.println("Failed to open the database.");
    return;
  }

  char* errMsg = 0;
  int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
  
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to delete synced records: %s\n", errMsg);
    sqlite3_free(errMsg);
  } else {
    Serial.println("Successfully deleted synced records.");
  }

  sqlite3_close(db);
}

void eliminazioneVotiProfilazione(sqlite3* db) {
  if(countSyncedRecordsInT3(db) > 2) { // significa che sono passati 2 giorni dato che il voto sarà registrato al max una volta al giorno
    deleteSyncedRecordsFromT3(db);
  }
}

void cleanAllarmiRed(sqlite3* db) {
  const char* sql = "DELETE FROM t4 WHERE synchronised = 1";
  
  if (db_open(PATH_STORAGE_DATA_DB, &db)) {
    Serial.println("Failed to open the database.");
    return;
  }

  char* errMsg = 0;
  int rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
  
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to delete synced records: %s\n", errMsg);
    sqlite3_free(errMsg);
  } else {
    Serial.println("Successfully deleted synced records.");
  }

  sqlite3_close(db);
}

// La frequeza di pulizia per questo particolare compito sarà molto lunga --> 7 giorni ad esempio
void cleanAllarmiYellow(sqlite3* db) {
    int rc = db_open(PATH_STORAGE_DATA_DB, &db);
    if (rc != SQLITE_OK) {
        Serial.println("Failed to open the database.");
        return;
    }

    // Query SQL per aggiornare i campi "daMandare", "rilevamenti" e synchronised a 0 per tutti i record
    const char* sql = "UPDATE t5 SET daMandare = 0, rilevamenti = 0, synchronised = 0";
    char* errMsg = 0;

    rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("Failed to reset fields: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        Serial.println("Successfully reset daMandare and rilevamenti for all records.");
    }

    // Chiude il database
    sqlite3_close(db);
}