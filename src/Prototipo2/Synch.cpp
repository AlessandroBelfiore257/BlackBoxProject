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
  String res = nome_sensore + "," + valore + "," + tipo_dato + "," + unit_misura + "," + s + "," + synchronised + "," + priority;

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