#include <stdio.h>
#include <stdlib.h>
#include <TaskScheduler.h>
#include <Wire.h>
#include <LittleFS.h>

#include "Storage.h"
#include "Filesystem.h"
#include "Synch.h"
#include "LCtime.h"

#include "Config.h"
#include "Secrets.h"

// Credenziale di accesso alla rete
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
// Porta di ascolto e indirizzo IP (del server remoto ovvero la stazione di controllo dell'autonoleggio)
const uint16_t port = PORT;
const char* host = HOST;
// Sensore bottone
const int BUTTON_PIN = BUTTON;

Scheduler scheduler;
sqlite3* db;
WiFiClient client;
RTC_DS3231 rtc;
const char* data = "Callback function called";
// Rimpiazzo le variabili sottostanti con quelle dei sensori e della rete CAN-BUS
// Sensore bottone
int buttonState = 0;
// Sensore numero casuale
int mydata = 0;

int rc = 0;
bool passed = false;
long long int lastStoredTS = 0;
long long int lastSynchTS = 0;

// Prototipi delle funzioni chiamate dai task dello scheduler
void rilevoButtonPressureCallback();
void synchDataCallback();  // -> sarà messo in un task a priorità alta nella versione finale

// Definizione dei task
Task ButtonPressureTask(TASK_BUTTON_PRESSURE, TASK_FOREVER, &rilevoButtonPressureCallback);
Task synchDataTask(240000, TASK_FOREVER, &synchDataCallback);

void setup() {

  Serial.begin(115200);

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  /* DA USARE PER LA VERSIONE UFFICIALE
  LittleFS.begin();
  if (!LittleFS.exists(DB_FILE)) {
    configurationProcess();
  } else {
    Serial.println("Database alredy exist");
  } */

  // La prima accensione va fatta in presenza di una connessione internet in modo tale da poter sincronizzare tutte le informazioni necessare per un corretto funzionamento futuro della Black Box
  configurationProcess();

  scheduler.init();
  scheduler.addTask(synchDataTask);
  scheduler.addTask(ButtonPressureTask);
  synchDataTask.enable();
  ButtonPressureTask.enable();
}

void loop() {
  scheduler.execute();
}

void configurationProcess() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected\n");

  // Viene assegnato all'RTC un orario di "nascita" (configurazione che va fatta con una connessione internet per contattare i server NTP)
  setTimeIT();
  // Montaggio filesystem
  if (mountingFS(FILE_SYSTEM)) {
    Serial.println("Little FS Mounted Successfully");
  } else {
    Serial.println("Failed to mount file system");
  }
  // Apertura filesystem + cancellazione precedenti dati memorizzati in "/dati.db"
  openFS();
  // Creazione file dati.db + tabella principale di memorizzazione (t1)
  initializeStorage(PATH_STORAGE_DATA);

  // Disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool spentEnoughTimeFromLastStrorage(char* nameSensor, long long int timestampNow, long long int lastStoredTS) {
  if (db_open(PATH_STORAGE_DATA, &db))
    return false;
  // Query e verifico se è passato il tempo di campionamento:
  const char* sql = "SELECT * FROM t1 WHERE nome_sensore = ? AND timestamp = ?";
  // Prepara la dichiarazione SQL
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println("Errore durante la preparazione della query SQL");
    sqlite3_close(db);
    return false;
  }
  // Associa i valori dei parametri (nome sensore e timestamp) alla query
  sqlite3_bind_text(stmt, 1, nameSensor, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, lastStoredTS);
  // Esegui la query
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // Il record è stato trovato, ora puoi recuperare i valori dei campi e visualizzarli
    const char* nome_sensore = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    double valore = sqlite3_column_double(stmt, 1);
    long long int timestamp = sqlite3_column_int64(stmt, 2);  // Modifica qui se il timestamp è un intero a 64 bit
    int sincronizzato = sqlite3_column_int(stmt, 3);
    int priorita = sqlite3_column_int(stmt, 4);
    Serial.print("Last registered record: ");
    Serial.print(timestamp);
    Serial.print(": ");
    char prova[20];
    SecondsToString(timestamp, prova);
    Serial.println(prova);
  } else {
    Serial.println("Nessun record trovato per il sensore specificato e il timestamp specificato");
  }
  // Rilascia la dichiarazione SQL
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  // Verifica se è passato il tempo di campionamento --> politica di storage!!! (Da rivedere)
  if ((timestampNow - lastStoredTS) >= 60) {
    return true;
  } else {
    return false;
  }
}

// TASK 1
void rilevoButtonPressureCallback() {

  buttonState = digitalRead(BUTTON_PIN);

  // Converte il timestamp attuale in secondi trascorsi dal 1970/01/01
  const DateTime dt = rtc.now();
  long long int timestampNow = DateTimeToEpoch(dt);
  ;

  Serial.print(timestampNow);
  Serial.print(" - ");
  Serial.println(lastStoredTS);
  char b1[20];
  char b2[20];
  SecondsToString(timestampNow, b1);
  SecondsToString(lastStoredTS, b2);
  Serial.print(DateTimeToString(EpochToDateTime(timestampNow)));
  Serial.print(" - ");
  Serial.println(DateTimeToString(EpochToDateTime(lastStoredTS)));

  passed = spentEnoughTimeFromLastStrorage("Button pressure", timestampNow, lastStoredTS);

  // Se è passato il tempo di storage, esegui le azioni desiderate
  if (passed) {
    String timeStamp = DateTimeToString(dt);
    int syn = NO_SYNCHRONIZED;
    long long int ts = stringToMilliseconds(timeStamp.c_str());
    createRecordToInsertIntot1("Button pressure", buttonState, ts, syn, 2);
    lastStoredTS = timestampNow;
  } else {
    Serial.println("Non è trascorso abbastanza tempo dall'ultima storage del dato");
  }
}

// TASK 2
void synchDataCallback() {
  Serial.println("--------------------------------------------            --------------------------------------------");
  Serial.println("Syncronisation...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected\n");
  syncData(db);
}
