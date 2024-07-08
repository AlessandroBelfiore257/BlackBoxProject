#include "Synch.h"

#include "Storage.h"
#include "Secrets.h"
#include "Config.h"
#include <TimeLib.h>

WiFiClient client;
extern sqlite3* db;
long long int lastSynchTS = 0;

/*
  Descrizione: funzione per ottenere il tempo corrente in secondi dal 1 gennaio 1970 (epoch time)
  Input: -
  Return: il numero di secondi trascorsi dall'epoch
*/
long long int getTimeNowInSeconds() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    delay(1000);
    return 0;
  }
  time_t time_seconds = mktime(&timeinfo);
  return static_cast<long long int>(time_seconds);
}

/*
  Descrizione: la sincronizzazione è effettuata al massimo una volta al giorno nel senso che da una sincronizzazione all'altra devono trascorrere almeno 24h
  Input: -
  Return: true se la black box è pronta per la sincronizzazione, false altrimenti
*/
bool shouldSync() {
  long long int currentTime = getTimeNowInSeconds();
  if (currentTime - lastSynchTS >= 86400) {
    return true;
  }
  return false;
}

/*
  Descrizione: funzione di padding
  Input: il numero in cui verrà effettuato il parsing
  Return: il numero parsato
*/
String zeroPad(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

// ########################################### Monitoraggio ###########################################

/*
  Descrizione: ritorna i record ordinati per timestamp e priorità (i più vecchi aventi priorità più alta)
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - records, dove vengono memorizzati i record secondo l'ordine prestabilito
  Return: void
*/
void getOrderedRecordsbyTSandPriority(sqlite3* db, std::vector<std::vector<String>>& records) {
  const char* sql = "SELECT nome_sensore, valore, tipo_dato, unita_misura, timestamp, synchronised, priority FROM t1 WHERE synchronised = 0 ORDER BY timestamp ASC, priority DESC";
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
    record.push_back(String(sqlite3_column_int64(stmt, 4)));                                // timestamp
    record.push_back(String(sqlite3_column_int(stmt, 5)));                                  // synchronised
    record.push_back(String(sqlite3_column_int(stmt, 6)));                                  // priority
    records.push_back(record);
  }
  sqlite3_finalize(stmt);
}

/* 
  Descrizione: funzione che dato un db e un record ne aggiorna il campo syncronised
  Input:
    - db, ovvero il database in cui sono memorizzati i dati
    - record, ovvero il record da modificare
  Return: void
*/
void updateSyncStatusMonitoraggio(sqlite3* db, const std::vector<String>& record) {
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
  Descrizione: funzione che manda i dati di monitoraggio al server
  Input: 
    - nome_sensore: il nome del sensore
    - valore: il valore rilevato dal sensore
    - tipo_dato: il tipo di dato del valore del sensore
    - unit_misura: l'unità di misura del valore
    - timestamp: il timestamp in cui il dato è stato rilevato
    - synchronised: lo stato di sincronizzazione del dato
    - priority: la priorità del dato
  Return: void
*/
void SendToServerMonitoraggio(String nome_sensore, String valore, String tipo_dato, String unit_misura, String timestamp, String synchronised, String priority) {
  time_t epochTime = atoll(timestamp.c_str());
  DateTime dt = DateTime(epochTime);
  String s = String(dt.year()) + "-" + zeroPad(dt.month()) + "-" + zeroPad(dt.day()) + " " + zeroPad(dt.hour()) + ":" + zeroPad(dt.minute()) + ":" + zeroPad(dt.second());
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

void syncMonitoraggioData(sqlite3* db) {
  /* DA INSERIRE NELLA VERSIONE UFFICIALE !!!
  if (!shouldSync()) {
    Serial.println("Sync not needed, last sync was less than 24 hours ago.");
    return;
  }
  */
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return;
  std::vector<std::vector<String>> records;
  getOrderedRecordsbyTSandPriority(db, records);
  sqlite3_close(db);
  for (auto& record : records) {
    String nome_sensore = record[0];
    String valore = record[1];
    String tipo_dato = record[2];
    String unit_misura = record[3];
    String timestamp = record[4];
    String synchronised = record[5];
    String priority = record[6];
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
    SendToServerMonitoraggio(nome_sensore, valore, tipo_dato, unit_misura, timestamp, synchronised, priority);
    if (db_open("/littlefs/dati.db", &db))
      return;
    updateSyncStatusMonitoraggio(db, record);
    sqlite3_close(db);
    /*
      if (sendDataToServer(record)) {
        //updateSyncStatus(db, record);

      } else {
        Serial.println("Failed to send data to server");
      }
      */
  }
  lastSynchTS = getTimeNowInSeconds();
  client.stop();
}

// ########################################### Profilazione ###########################################

/*
  Descrizione: ottiene i record non sincronizzati dalla tabella t3 (profilazione), ordinati per lastStorage crescente.
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - records, dove verranno memorizzati i record sincronizzati
  Return: void
*/
void getUnsyncedRecordsFromProfilazione(sqlite3* db, std::vector<std::vector<String>>& records) {
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
    record.push_back(String(sqlite3_column_int64(stmt, 2)));                                // lastStorage
    record.push_back(String(sqlite3_column_int(stmt, 3)));                                  // synchronised
    records.push_back(record);
  }
  sqlite3_finalize(stmt);
}

/*
  Descrizione: aggiorna lo stato di sincronizzazione nella tabella t3
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - record, il record di cui aggiornare lo stato di sincronizzazione
  Return: void
*/
void updateSyncStatusProfilazione(sqlite3* db, const std::vector<String>& record) {
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

/*
  Descrizione: funzione che manda i dati di profilazione al server.
  Input: 
    - date, la data in cui è stato calcolato il voto del conducente
    - normalizedNumber, il voto del conducente
  Return: void
*/
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

void syncProfilazioneData(sqlite3* db) {
  /*
  if (!shouldSync()) {
    Serial.println("Sync not needed, last sync was less than 24 hours ago.");
    return;
  } */
  if (db_open(PATH_STORAGE_DATA_DB, &db)) {
    return;
  }
  std::vector<std::vector<String>> records;
  getUnsyncedRecordsFromProfilazione(db, records);
  sqlite3_close(db);
  for (auto& record : records) {
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
    updateSyncStatusProfilazione(db, record);
    sqlite3_close(db);
  }
  lastSynchTS = getTimeNowInSeconds();
  client.stop();
}

// ########################################### Allarmi rossi ###########################################

/*
  Descrizione: ottiene i record non sincronizzati dalla tabella t4 (allarmi critici)
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - records, dove verranno memorizzati i record non sincronizzati
  Return: void
*/
void getUnsyncedRecordsFromRedAllarm(sqlite3* db, std::vector<std::vector<String>>& records) {
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
    record.push_back(String(sqlite3_column_int(stmt, 2)));                                  // synchronised
    records.push_back(record);
  }
  sqlite3_finalize(stmt);
}

/*
  Descrizione: aggiorna lo stato di sincronizzazione nella tabella t4
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - record, ovvero il record di cui aggiornare lo stato di sincronizzazione
  Return: void
*/
void updateSyncStatusAllarmiRossi(sqlite3* db, const std::vector<String>& record) {
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

/*
  Descrizione: funzione che manda i dati relativi agli allarmi più critici al server
  Input: 
    - problema, ovvero il problema rilevato
    - data, ovvero la data in cui è stato rilevato il problema
    - synchronised, ovvero lo stato di sincronizzazione del problema
  Return: void
*/
void SendToServerAllarmiRossi(String problema, String data, String synchronised) {
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

void syncAllarmiRossiData(sqlite3* db) {
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
  getUnsyncedRecordsFromRedAllarm(db, records);
  sqlite3_close(db);
  for (auto& record : records) {
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
    SendToServerAllarmiRossi(problema, data, synchronised);
    if (db_open(PATH_STORAGE_DATA_DB, &db)) {
      return;
    }
    updateSyncStatusAllarmiRossi(db, record);
    sqlite3_close(db);
  }
  lastSynchTS = getTimeNowInSeconds();
  client.stop();
}

// ########################################### Allarmi gialli ###########################################

/*
  Descrizione: ottiene i record non sincronizzati dalla tabella t5 
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - records, ovvero dove saranno memorizzati i record non sincronizzati
  Return: void
*/
void getUnsyncedRecordsFromYellowAllarm(sqlite3* db, std::vector<std::vector<String>>& records) {
  const char* sql = "SELECT problema, rilevamenti, soglia, daMandare, synchronised FROM t5 WHERE synchronised = 0 AND daMandare = 1";
  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return;
  }
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::vector<String> record;
    record.push_back(String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));  // problema
    record.push_back(String(sqlite3_column_int(stmt, 1)));                                  // rilevamenti
    record.push_back(String(sqlite3_column_int(stmt, 2)));                                  // soglia
    record.push_back(String(sqlite3_column_int(stmt, 3)));                                  // daMandare
    record.push_back(String(sqlite3_column_int(stmt, 4)));                                  // synchronised
    records.push_back(record);
  }
  sqlite3_finalize(stmt);
}

/*
  Descrizione: aggiorna lo stato di sincronizzazione nella tabella t5
  Input: 
    - db, ovvero il database in cui sono memorizzati i dati
    - record, ovvero il record di cui aggiornare lo stato di sincronizzazione
  Return: void
*/
void updateSyncStatusAllarmiGialli(sqlite3* db, const std::vector<String>& record) {
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

/*
  Descrizione: funzione che manda i dati relativi agli allarmi meno critici al server.
  Input: 
    - problema: il problema rilevato
    - rilevamenti: il numero di rilevamenti
    - soglia: la soglia di rilevamento
    - daMandare: flag per inviare il dato
    - synchronised: lo stato di sincronizzazione del dato
  Return: void
*/
void SendToServerAllarmiGialli(String problema, String rilevamenti, String soglia, String daMandare, String synchronised) {
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

void syncAllarmiGialliData(sqlite3* db) {
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
  getUnsyncedRecordsFromYellowAllarm(db, records);
  sqlite3_close(db);
  for (auto& record : records) {
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
    SendToServerAllarmiGialli(problema, rilevamenti, soglia, daMandare, synchronised);
    if (db_open(PATH_STORAGE_DATA_DB, &db)) {
      return;
    }
    updateSyncStatusAllarmiGialli(db, record);
    sqlite3_close(db);
  }
  lastSynchTS = getTimeNowInSeconds();
  client.stop();
}