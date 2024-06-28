#include "Synch.h"

#include "Storage.h"
#include "Secrets.h"
#include "Config.h"
#include <TimeLib.h>

WiFiClient client;
extern sqlite3* db;
long long int lastSynchTS = 0;

// Ritorna i record ordinati per timestamp e priorità (i più vecchi aventi priorità più alta)
void getOrderedRecords(sqlite3* db, std::vector<std::vector<String>>& records) {
  const char* sql = "SELECT nome_sensore, valore, tipo_dato, unita_misura, timestamp, synchronised, priority FROM t1 ORDER BY timestamp ASC, priority DESC";
  sqlite3_stmt* stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.println("Failed to prepare statement");
    return;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::vector<String> record;
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));  // nome_sensore
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));  // valore
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));  // tipo_dato
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));  // unita_misura
    record.push_back(String(sqlite3_column_int64(stmt, 4)));  // timestamp
    record.push_back(String(sqlite3_column_int(stmt, 5)));  // synchronised
    record.push_back(String(sqlite3_column_int(stmt, 6)));  // priority
    records.push_back(record);
  }

  sqlite3_finalize(stmt);
}

// Funzione che dato un db e un record ne aggiorna il campo syncronised from 0 to 1
void updateSyncStatus(sqlite3* db, const std::vector<String>& record) {
  char sql[256];
  // Ciò che identifica unicamente un record è il nome del sensore con il timestamp associato
  snprintf(sql, sizeof(sql), "UPDATE t1 SET synchronised = 1 WHERE nome_sensore = '%s' AND timestamp = %s;",
           record[0].c_str(), record[4].c_str());

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
  long long int currentTime = getTimeNowInSeconds();
  if (currentTime - lastSynchTS >= 86400) {  // 24 ore in millisecondi
    return true;
  }
  return false;
}

String zeroPad(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

void SendToServer(String nome_sensore, String valore, String tipo_dato, String unit_misura, String timestamp, String synchronised, String priority) {
  // Converte il timestamp in un oggetto DateTime
  time_t epochTime = atoll(timestamp.c_str());

  // Aggiunge 2 ore al timestamp
  //epochTime += 2 * 3600; // Converti le ore in secondi (2 ore * 3600 secondi/ora)

  // Converte l'orario regolato in una stringa
  DateTime dt = DateTime(epochTime);
  String s = String(dt.year()) + "-" + zeroPad(dt.month()) + "-" + zeroPad(dt.day()) + " " + zeroPad(dt.hour()) + ":" + zeroPad(dt.minute()) + ":" + zeroPad(dt.second());

  // Costruisce la stringa dei dati da inviare al server
  String res = "Monitoraggio: " + nome_sensore + "," + valore + "," + tipo_dato + "," + unit_misura + "," + s + "," + priority;

  if (client.connected()) {
    Serial.println("Sending data to server...");
    Serial.print("Data: ");
    Serial.println(res);
    client.println(res);
  } else {
    Serial.println("Connection lost, failed to send data");
  }
}


void syncData(sqlite3* db) {
  /* DA INSERIRE NELLA VERSIONE UFFICIALE
  if (!shouldSync()) {
    Serial.println("Sync not needed, last sync was less than 24 hours ago.");
    return;
  }
  */
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  std::vector<std::vector<String>> records;
  getOrderedRecords(db, records);
  sqlite3_close(db);

  for (auto& record : records) {
    if (record[5] == "0") {  // Check if record is not already synced
      String nome_sensore = record[0];
      String valore = record[1];
      String tipo_dato = record[2];
      String unit_misura = record[3];
      String timestamp = record[4];
      String synchronised = record[5];
      String priority = record[6];
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
        if (!client.connect(HOST, PORT)) {
          Serial.println("Connection to host failed");
        } else {
          Serial.println("Connected to server successful!");
        }
        Serial.println("passatooo");
      }
      SendToServer(nome_sensore, valore, tipo_dato, unit_misura, timestamp, synchronised, priority);
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
  client.stop();
}

// Profilazione ###########################################
// Funzione per ottenere i record non sincronizzati dalla tabella t3, ordinati per lastStorage crescente
void getUnsyncedRecordsFromT3(sqlite3* db, std::vector<std::vector<String>>& records) {
  const char* sql = "SELECT data, voto, lastStorage, synchronised FROM t3 WHERE synchronised = 0 ORDER BY lastStorage ASC";
  sqlite3_stmt* stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::vector<String> record;
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));  // date
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));  // normalizedNumber
    record.push_back(String(sqlite3_column_int64(stmt, 2)));  // lastStorage
    record.push_back(String(sqlite3_column_int(stmt, 3)));  // synchronised
    records.push_back(record);
  }

  sqlite3_finalize(stmt);
}

// Funzione per aggiornare lo stato di sincronizzazione nella tabella t3
void updateSyncStatusInT3(sqlite3* db, const std::vector<String>& record) {
  char sql[256];
  snprintf(sql, sizeof(sql), "UPDATE t3 SET synchronised = 1 WHERE data = '%s' AND lastStorage = %s;",
           record[0].c_str(), record[2].c_str());

  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to update sync status: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Sync status updated successfully.");
  }
}

// Funzione per inviare i dati al server
void SendToServerProfilazione(String date, String normalizedNumber) {

  String res = "Profilazione: " + normalizedNumber + "," + date;

  if (client.connected()) {
    Serial.println("Sending data to server...");
    Serial.print("Data: ");
    Serial.println(res);
    client.println(res);
  } else {
    Serial.println("Connection lost, failed to send data");
  }
}

// Funzione principale per sincronizzare i dati dalla tabella t3
void syncT3Data(sqlite3* db) {
  /*
  if (!shouldSync()) {
    Serial.println("Sync not needed, last sync was less than 24 hours ago.");
    return;
  } */

  if (db_open(PATH_STORAGE_DATA_DB, &db)) {
    return;
  }

  std::vector<std::vector<String>> records;
  getUnsyncedRecordsFromT3(db, records);
  sqlite3_close(db);

  for (auto& record : records) {
    if (record[3] == "0") {  // Se il record non è già sincronizzato
      String date = record[0];
      String normalizedNumber = record[1];
      String lastStorage = record[2];
      String synchronised = record[3];

      while (!client.connected()) {
        if (!client.connect(HOST, PORT)) {
          Serial.println("Connection to host failed");
          delay(1000);
        } else {
          Serial.println("Connected to server successfully!");
        }
      }

      SendToServerProfilazione(date, normalizedNumber);

      if (db_open(PATH_STORAGE_DATA_DB, &db)) {
        return;
      }
      updateSyncStatusInT3(db, record);
      sqlite3_close(db);
    }
  }

  lastSynchTS = getTimeNowInSeconds();
  client.stop();
}

// t4

// Funzione per ottenere i record non sincronizzati dalla tabella t4, ordinati per timestamp crescente
void getUnsyncedRecordsFromT4(sqlite3* db, std::vector<std::vector<String>>& records) {
    const char* sql = "SELECT problema, data, synchronised FROM t4 WHERE synchronised = 0";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::vector<String> record;
        record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));  // problema
        record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));  // data
        record.push_back(String(sqlite3_column_int(stmt, 2)));  // synchronised
        records.push_back(record);
    }

    sqlite3_finalize(stmt);
}

// Funzione per aggiornare lo stato di sincronizzazione nella tabella t4
void updateSyncStatusInT4(sqlite3* db, const std::vector<String>& record) {
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE t4 SET synchronised = 1 WHERE problema = '%s' AND data = '%s';",
             record[0].c_str(), record[1].c_str());

    int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        Serial.printf("Failed to update sync status: %s\n", sqlite3_errmsg(db));
    } else {
        Serial.println("Sync status updated successfully.");
    }
}

// Funzione per inviare i dati al server
void SendToServerT4(String problema, String data, String synchronised) {
    // Costruisce la stringa dei dati da inviare al server
    String res = problema + "," + data + "," + synchronised;

    if (client.connected()) {
        Serial.println("Sending data to server...");
        Serial.print("Data: ");
        Serial.println(res);
        client.println(res);
    } else {
        Serial.println("Connection lost, failed to send data");
    }
}

// Funzione principale per sincronizzare i dati dalla tabella t4
void syncT4Data(sqlite3* db) {
    /*
    if (!shouldSync()) {
        Serial.println("Sync not needed, last sync was less than 24 hours ago.");
        return;
    }
    */

    if (db_open(PATH_STORAGE_DATA_DB, &db)) {
        return;
    }

    std::vector<std::vector<String>> records;
    getUnsyncedRecordsFromT4(db, records);
    sqlite3_close(db);

    for (auto& record : records) {
        if (record[2] == "0") {  // Se il record non è già sincronizzato
            String problema = record[0];
            String data = record[1];
            String synchronised = record[2];

            while (!client.connected()) {
                if (!client.connect(HOST, PORT)) {
                    Serial.println("Connection to host failed");
                    delay(1000);
                } else {
                    Serial.println("Connected to server successfully!");
                }
            }

            SendToServerT4(problema, data, synchronised);

            if (db_open(PATH_STORAGE_DATA_DB, &db)) {
                return;
            }
            updateSyncStatusInT4(db, record);
            sqlite3_close(db);
        }
    }

    lastSynchTS = getTimeNowInSeconds();
    client.stop();
}

// t5
void getUnsyncedRecordsFromT5(sqlite3* db, std::vector<std::vector<String>>& records) {
    const char* sql = "SELECT problema, rilevamenti, soglia, daMandare, synchronised FROM t5 WHERE synchronised = 0 AND daMandare = 1";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::vector<String> record;
        record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));  // problema
        record.push_back(String(sqlite3_column_int(stmt, 1)));  // rilevamenti
        record.push_back(String(sqlite3_column_int(stmt, 2)));  // soglia
        record.push_back(String(sqlite3_column_int(stmt, 3)));  // daMandare
        record.push_back(String(sqlite3_column_int(stmt, 4)));  // synchronised
        records.push_back(record);
    }

    sqlite3_finalize(stmt);
}

void SendToServerT5(String problema, String rilevamenti, String soglia, String daMandare, String synchronised) {
    // Costruisce la stringa dei dati da inviare al server
    String res = "Allarmi (giallo): " + problema + "," + rilevamenti + "," + soglia;

    if (client.connected()) {
        Serial.println("Sending data to server...");
        Serial.print("Data: ");
        Serial.println(res);
        client.println(res);
    } else {
        Serial.println("Connection lost, failed to send data");
    }
}

void updateSyncStatusInT5(sqlite3* db, const std::vector<String>& record) {
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE t5 SET synchronised = 1 WHERE problema = '%s' AND rilevamenti = %s AND soglia = %s;",
             record[0].c_str(), record[1].c_str(), record[2].c_str());

    int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        Serial.printf("Failed to update sync status: %s\n", sqlite3_errmsg(db));
    } else {
        Serial.println("Sync status updated successfully.");
    }
}

void syncT5Data(sqlite3* db) {
    /*
    if (!shouldSync()) {
        Serial.println("Sync not needed, last sync was less than 24 hours ago.");
        return;
    }
    */

    if (db_open(PATH_STORAGE_DATA_DB, &db)) {
        return;
    }

    std::vector<std::vector<String>> records;
    getUnsyncedRecordsFromT5(db, records);
    sqlite3_close(db);

    for (auto& record : records) {
        if (record[4] == "0" && record[3] == "1") {  // Se il record non è già sincronizzato e daMandare è 1
            String problema = record[0];
            String rilevamenti = record[1];
            String soglia = record[2];
            String daMandare = record[3];
            String synchronised = record[4];

            while (!client.connected()) {
                if (!client.connect(HOST, PORT)) {
                    Serial.println("Connection to host failed");
                    delay(1000);
                } else {
                    Serial.println("Connected to server successfully!");
                }
            }

            SendToServerT5(problema, rilevamenti, soglia, daMandare, synchronised);

            if (db_open(PATH_STORAGE_DATA_DB, &db)) {
                return;
            }
            updateSyncStatusInT5(db, record);
            sqlite3_close(db);
        }
    }

    lastSynchTS = getTimeNowInSeconds();
    client.stop();
}