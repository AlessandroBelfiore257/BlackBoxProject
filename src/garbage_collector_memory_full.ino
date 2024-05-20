#include <LittleFS.h>
#include <sqlite3.h>
#include <vector>

#define FORMAT_LITTLEFS_IF_FAILED true

sqlite3 *db;
int i = 0;

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, "/littlefs")) {
    Serial.println("Failed to mount file system");
    return;
  } else {
    Serial.println("Little FS Mounted Successfully");
  }

  stampaMemorySituation();

  LittleFS.remove("/dati.db");
  stampaMemorySituation();
  initializeStorage();
  stampaMemorySituation();
}

void loop() {
  int mydata = random(0, 1000);
  if (i < 50) {
    createRecordToInsertIntot("Casual", mydata, i+1, 1, 3);
  } else if (i >= 50 && i < 200) {
    createRecordToInsertIntot("Casual", mydata, i+1, 0, 3);
  }

  stampaMemorySituation();  // Stampa la situazione della memoria dopo l'inserimento
  if (i == 200) {
    garbageCollector2();
    if (db_open("/littlefs/dati.db", &db))
      return;
    Serial.println("DB/t:");
    int rc = db_exec(db, "SELECT * FROM t");
    if (rc != SQLITE_OK) {
      sqlite3_close(db);
      return;
    }
    sqlite3_close(db);
  }
  i++;
  Serial.println(i);
  delay(1000);
}

void initializeStorage() {
  sqlite3_initialize();
  if (db_open("/littlefs/dati.db", &db))
    return;
  int rc = db_exec(db, "CREATE TABLE t (nome_sensore TEXT, valore REAL, timestamp INTEGER, synchronised INTEGER, priority INTEGER);");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }
  sqlite3_close(db);
}

static int callback(void *data, int argc, char **argv, char **azColName) {
  Serial.printf("%s: ", (const char *)data);
  for (int i = 0; i < argc; i++) {
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

void stampaMemorySituation() {
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();

  Serial.print("Total bytes: ");
  Serial.println(total);
  Serial.print("Used bytes: ");
  Serial.println(used);
  Serial.print((static_cast<float>(used) / total) * 100);
  Serial.println("%");

  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createRecordToInsertIntot(char *name_sensor, int value, long long int ts, int synch, int pr) {
  if (db_open("/littlefs/dati.db", &db))
    return;
  char buffer[1000];
  memset(buffer, 0, sizeof(buffer));
  Serial.print("Not buffered: ");
  Serial.println(buffer);
  sprintf(buffer, "INSERT INTO t VALUES ('%s', %d, %lld, %d, %d);", name_sensor, value, ts, synch, pr);
  Serial.print("Buffered: ");
  Serial.println(buffer);
  int rc = db_exec(db, buffer);
  if (rc != SQLITE_OK) {
    Serial.println("Failed to insert record");
  } else {
    Serial.println("Record inserted successfully");

    rc = db_exec(db, "PRAGMA wal_checkpoint(FULL);");
    if (rc != SQLITE_OK) {
      Serial.println("Failed to checkpoint WAL");
    }

    sqlite3_close(db);
    if (db_open("/littlefs/dati.db", &db)) {
      Serial.println("Failed to reopen database");
    }
  }
  sqlite3_close(db);
}

// // Chiamo GB1 ogni tot di tempo
void garbageCollector1() {
  if (db_open("/littlefs/dati.db", &db))
    return;

  int rc = db_exec(db, "BEGIN TRANSACTION;");
  if (rc != SQLITE_OK) {
    Serial.println("Error starting transaction.");
    sqlite3_close(db);
    return;
  }

  char *sql = "DELETE FROM t WHERE synchronised = 1;";
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
    }
  }
  sqlite3_close(db);
}


// Chiamo GB2 se supero il 90% della memoria
void garbageCollector2() {
  if (db_open("/littlefs/dati.db", &db))
    return;

  int numRecordPre = countRecordsInTableT(db);
  int recordToDelete = numRecordPre / 2;
  Serial.print("Record totali: ");
  Serial.println(numRecordPre);
  Serial.print("Record da eliminare: ");
  Serial.println(recordToDelete);
  // In primis elimino i record già sincronizzati
  garbageCollector1();
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
  char selectSql[100];
  char deleteSql[1000];
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
