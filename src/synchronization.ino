#include <LittleFS.h>
#include <sqlite3.h>
#include <vector>
#include <WiFi.h>
#include "time.h"

const char *ssid = "MY_SSID";
const char *password = "MY_PASSWORD";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const uint16_t port = 8090;
const char *host = "192.168.1.206";

#define FORMAT_LITTLEFS_IF_FAILED true

sqlite3 *db;
int i = 0;
long long int lastSynchTS = 0;
long long int currentTime;
WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Connessione Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  // Configurazione NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, "/littlefs")) {
    Serial.println("Failed to mount file system");
    return;
  } else {
    Serial.println("Little FS Mounted Successfully");
  }
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
  LittleFS.remove("/dati.db");
  initializeStorage();
  client.connect(host, port);
}

void loop() {
  int mydata = random(0, 1000);
  if (i < 5) {
    createRecordToInsertIntot("Just synch", mydata, i + 1, 1, 3);
  } else if (i >= 5 && i < 15) {
    createRecordToInsertIntot("Pressure oil", mydata, i + 1, 0, 3);
    createRecordToInsertIntot("Temperature oil", mydata, i + 1, 0, 7);
  } else if (i >= 15 && i < 25) {
    createRecordToInsertIntot("Più recenti", mydata, i + 1, 0, 2);
  } else if(i == 30) {
    createRecordToInsertIntot("Arrivato dopo", mydata, i + 1, 0, 9);
  }
  // simuliamo una connessione ad internet
  if (i == 25 || i == 35) {
    // stampo i record in ordine di mandata:
    syncData(db);
  }
  if (i == 26 || i == 36) {
    // Fase di controllo se vengono effettuate le synch = 1 all'interno del db
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

void getOrderedRecords(sqlite3 *db, std::vector<std::vector<String>> &records) {
  const char *sql = "SELECT nome_sensore, valore, timestamp, synchronised, priority FROM t ORDER BY timestamp ASC, priority DESC";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.println("Failed to prepare statement");
    return;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::vector<String> record;
    record.push_back(String(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))));
    record.push_back(String(sqlite3_column_double(stmt, 1)));
    record.push_back(String(sqlite3_column_int64(stmt, 2)));
    record.push_back(String(sqlite3_column_int(stmt, 3)));
    record.push_back(String(sqlite3_column_int(stmt, 4)));
    records.push_back(record);
  }

  sqlite3_finalize(stmt);
}

void updateSyncStatus(sqlite3 *db, const std::vector<String> &record) {
  char sql[256];
  // Ciò che identifica unicamente un record è il nome del sensore con il timestamp associato
  snprintf(sql, sizeof(sql), "UPDATE t SET synchronised = 1 WHERE nome_sensore = '%s' AND timestamp = %s;",
           record[0].c_str(), record[2].c_str());

  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to update sync status: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Sync status updated successfully.");
  }
}

/*
  Funzione per ottenere il tempo corrente in secondi dal 1 gennaio 1970 (epoch time)
  Ritorna un long long int
*/
long long int getTimeNowInSeconds() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    delay(1000);
    return 0;  // Ritorna 0 in caso di fallimento
  }
  time_t time_seconds = mktime(&timeinfo);
  return static_cast<long long int>(time_seconds);
}

// la sincronizzazione è effettuata al massimo una volta al giorno nel senso che da una sincronizzazione all'altra devono passare almeno 24h
bool shouldSync() {
  currentTime = getTimeNowInSeconds();
  if (currentTime - lastSynchTS >= 86400) {  // 24 ore in millisecondi
    return true;
  }
  return false;
}

void SendToServer(String name_sensor, String value, String timestamp, String synchronized, String priority) {
  String res = name_sensor + "," + value + "," + timestamp + "," + synchronized + "," + priority + "\n";
  client.print(res);
}

void syncData(sqlite3 *db) {
  /*
  if (!shouldSync()) {
    Serial.println("Sync not needed, last sync was less than 24 hours ago.");
    return;
  }
  */
  if (db_open("/littlefs/dati.db", &db))
    return;
  std::vector<std::vector<String>> records;
  getOrderedRecords(db, records);
  sqlite3_close(db);

  for (auto &record : records) {
    if (record[3] == "0") {  // Check if record is not already synced
      String nome_sensore = record[0];
      String valore = record[1];
      String timestamp = record[2];
      String synchronised = record[3];
      String priority = record[4];
      // simulazione send dati al server
      Serial.print(nome_sensore);
      Serial.print(" | ");
      Serial.print(valore);
      Serial.print(" | ");
      Serial.print(timestamp);
      Serial.print(" | ");
      Serial.print(synchronised);
      Serial.print(" | ");
      Serial.println(priority);
      while (!client.connected()) {
        if (!client.connect(host, port)) {
          Serial.println("Connection to host failed");
        } else {
          Serial.println("Connected to server successful!");
        }
      }
      SendToServer(nome_sensore, valore, timestamp, synchronised, priority);
    } else {
      Serial.println("Mancato");
    }
    if (db_open("/littlefs/dati.db", &db))
      return;
    updateSyncStatus(db, record);
    sqlite3_close(db);
    /*
      if (sendDataToServer(record)) {
        //updateSyncStatus(db, record);

      } else {
        Serial.println("Failed to send data to server");
      }
      */
  }
  // Aggiorna l'ultimo timestamp di sincronizzazione
lastSynchTS = getTimeNowInSeconds();
}
