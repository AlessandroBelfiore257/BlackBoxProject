#include <stdio.h>
#include <stdlib.h>
#include <SPI.h>
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <TaskScheduler.h>
#include <WiFi.h>
#include "time.h"

#include "Storage.h"
#include "LCtime.h"

#include "Config.h"
#include "Secrets.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const uint16_t port = PORT;
const char* host = HOST;
const int BUTTON_PIN = BUTTON;

Scheduler scheduler;
WiFiClient client;
// Emulazione sensori veri e propri
int mydata;
int buttonState = 0;

char buffer[BUFFER_SIZE];
int rc;
int synch;
bool passed;

void rilevoButtonPressureCallback();
void rilevoCasualNumberCallback();

Task ButtonPressureTask(TASK_BUTTON_PRESSURE, TASK_FOREVER, &rilevoButtonPressureCallback);
Task CasualNumberTask(TASK_CASUAL_NUMBER, TASK_FOREVER, &rilevoCasualNumberCallback);

const char* data = "Callback function called";

void setup() {
  Serial.begin(115200);
  setTimeIT();
  // Richiesta di connessione che andrà in un task (polling)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
  // Gestione di montaggio e apertura del filesystem (verifica dei file presenti in esso)
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, "/littlefs")) {
    Serial.println("Failed to mount file system");
    return;
  } else {
    Serial.println("Little FS Mounted Successfully");
  }
  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  LittleFS.remove("/dati.db");
  // Creazione file dati.db + tabella principale di memorizzazione (t1)
  initializeStorage();
  // Inizializzazione dello scheduler
  scheduler.init();
  // Aggiunta Task allo scheduler
  scheduler.addTask(ButtonPressureTask);
  // scheduler.addTask(CasualNumberTask);
  // Abilito i Task
  ButtonPressureTask.enable();
  // CasualNumberTask.enable();
}

void loop() {
  scheduler.execute();
}

void rilevoButtonPressureCallback() {
  buttonState = digitalRead(BUTTON_PIN);

  if (db_open("/littlefs/dati.db", &db))
    return;

  // query e verifico se è passato il tempo di campionamento:

  // Converte il timestamp attuale in millisecondi
  struct tm timeinfo = getDateAndTime();
  time_t timestampNow_seconds = mktime(&timeinfo);
  unsigned long timestampNow_millis = timestampNow_seconds * 1000;
  
  // Variabile per memorizzare il timestamp del record più recente in secondi
  unsigned long timestampQuery_millis = 0;

  // Query SQL per selezionare il record con il timestamp più recente per un determinato sensore
  const char* sql = "SELECT * FROM t1 WHERE nome_sensore = ? ORDER BY datetime(timestamp) DESC LIMIT 1;";
  // Prepara la dichiarazione SQL
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println("Errore durante la preparazione della query SQL");
    return;
  }
  // Associa il valore del parametro (nome sensore) alla query
  sqlite3_bind_text(stmt, 1, "Button pressure", -1, SQLITE_TRANSIENT);
  // Esegui la query
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // Il record è stato trovato, ora puoi recuperare i valori dei campi e visualizzarli
    const char* nome_sensore = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    double valore = sqlite3_column_double(stmt, 1);
    const char* timestampQ = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    int sincronizzato = sqlite3_column_int(stmt, 3);
    int priorita = sqlite3_column_int(stmt, 4);

    // Visualizza i valori dei campi
    Serial.print("Nome sensore: ");
    Serial.println(nome_sensore);
    Serial.print("Valore: ");
    Serial.println(valore);
    // Visualizza gli altri campi come nel precedente esempio
    struct tm timestamp_Q;
    strptime(timestampQ, "%Y-%m-%d %H:%M:%S", &timestamp_Q);
    time_t timestamp_seconds = mktime(&timestamp_Q);
    timestampQuery_millis = timestamp_seconds * 1000; // Assegnazione del valore
  } else {
    // Nessun record trovato
    Serial.println("Nessun record trovato per il sensore specificato");
  }
  // Rilascia la dichiarazione SQL
  sqlite3_finalize(stmt);
  
  // Verifica se è passato il tempo di campionamento
  if((timestampNow_millis - timestampQuery_millis) >= 60000) {
    passed = true;
  } else {
    passed = false;
  }
  Serial.print(timestampNow_millis);
  Serial.print(" - ");
  Serial.println(timestampQuery_millis);

  // Se è passato il tempo di campionamento, esegui le azioni desiderate
  if (passed) {
    String fattaAmano = createTimeStampStringFormat(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    int synch = NO_SYNCHRONIZED;
    createRecordToInsertIntot1("Button pressure", buttonState, fattaAmano.c_str(), synch, 2);

    sqlite3_close(db);

    if (!client.connected()) {
      if (!client.connect(host, port)) {
        Serial.println("Connection to host failed");
        return;
      } else {
        Serial.println("Connected to server successful!");
      }
    }

    SendToServer("Button pressure", buttonState, fattaAmano.c_str(), synch, 2);
  } else {
    Serial.println("Mancato");
  }
}

void rilevoCasualNumberCallback() {
  mydata = random(0, 1000);

  if (db_open("/littlefs/dati.db", &db))
    return;
  struct tm timeinfo = getDateAndTime();
  String fattaAmano = createTimeStampStringFormat(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  synch = NO_SYNCHRONIZED;
  createRecordToInsertIntot1("Casual number", mydata, fattaAmano.c_str(), synch, 1);
  rc = db_exec(db, "SELECT * FROM t1");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }

  sqlite3_close(db);


  if (!client.connected()) {
    if (!client.connect(host, port)) {
      Serial.println("Connection to host failed");
      return;
    } else {
      Serial.println("Connected to server successful!");
    }
  }

  SendToServer("Casual number", mydata, fattaAmano.c_str(), synch, 1);
}

/*
  Creazione timestamp sotto forma di stringa 
*/
String createTimeStampStringFormat(int day, int month, int year, int hour, int minute, int second) {
  return String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second);
}

/*
  Creazione record e inserimento nella tabella t1 
*/
void createRecordToInsertIntot1(char* name_sensor, int value, const char* timestamp, int synchronized, int priority) {
  sprintf(buffer, "INSERT INTO t1 VALUES ('%s', %d, '%s', %d, %d);", name_sensor, value, timestamp, synchronized, priority);
  rc = db_exec(db, buffer);
}

/*
  Invio al server le informazioni utili
*/
void SendToServer(char* name_sensor, int value, const char* timestamp, int synchronized, int priority) {
  client.print(String(name_sensor) + "," + String(value) + "," + timestamp + "," + String(synchronized) + "," + String(priority) + "\n");
}
