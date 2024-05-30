#include <TaskScheduler.h>
#include <LittleFS.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#include "Secrets.h"
#include "Config.h"

#include "LCtime.h"
#include "Storage.h"
#include "Filesystem.h"
#include "Synch.h"
#include "Garbage.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const uint16_t port = PORT;
const char* host = HOST;

static const uint32_t GPSBaud = 38400;

Scheduler scheduler;
sqlite3* db;
RTC_DS3231 rtc;
TinyGPSPlus gps;

// Rimpiazzo le variabili sottostanti con quelle dei sensori e della rete CAN-BUS
// Sensore bottone
int buttonState = 0;
HardwareSerial hs = Serial2;

long long int lastStoredTS1 = 0;
long long int lastStoredTS2 = 0;

// Prototipi delle funzioni chiamate dai task dello scheduler
void rilevoButtonPressureCallback();
void gspTrackerCallback();

void synchDataCallback();
void cleanDataRoutineCallback();
void cleanDataMemoryFullCallback();

// Definizione dei task
Task ButtonPressureTask(BUTTON_PRESSURE_DETECTION* TASK_SECOND, TASK_FOREVER, &rilevoButtonPressureCallback);
Task synchDataTask(SYNCHRONIZATION_DATA* TASK_SECOND, TASK_FOREVER, &synchDataCallback);
Task cleanDataRoutineTask(CLEANING_ROUTINE* TASK_SECOND, TASK_FOREVER, &cleanDataRoutineCallback);
Task cleanDataMemoryFullTask(CLEANING_MEMORY_FULL* TASK_SECOND, TASK_FOREVER, &cleanDataMemoryFullCallback);
Task gpsTrackerTask(GPS* TASK_SECOND, TASK_FOREVER, &gspTrackerCallback);

void setup() {

  Serial.begin(115200);

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  hs.begin(GPSBaud);

  /* DA USARE PER LA VERSIONE UFFICIALE
  LittleFS.begin();
  if (!LittleFS.exists(DB_FILE)) {
    configurationProcess();
  } else {
    Serial.println("Database already exist");
  } */

  // La prima accensione va fatta in presenza di una connessione internet in modo tale da poter sincronizzare tutte le informazioni necessare per un corretto funzionamento futuro della Black Box
  configurationProcess();

  scheduler.init();

  scheduler.addTask(cleanDataRoutineTask);
  scheduler.addTask(cleanDataMemoryFullTask);
  scheduler.addTask(synchDataTask);
  scheduler.addTask(ButtonPressureTask);
  scheduler.addTask(gpsTrackerTask);

  // cleanDataRoutineTask.enable();
  // cleanDataMemoryFullTask.enable();
  synchDataTask.enable();
  ButtonPressureTask.enable();
  gpsTrackerTask.enable();
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
  initializeStorage(PATH_STORAGE_DATA_DB);
}

bool spentEnoughTimeFromLastStrorage(char* nameSensor, long long int timestampNow, long long int lastStoredTS) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
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
    Serial.print("Last registered record timestamp: ");
    char prova[20];
    SecondsToString(timestamp, prova);
    Serial.println(prova);
  } else {
    Serial.println("Nessun record trovato per il sensore specificato e il timestamp specificato");
  }
  // Rilascia la dichiarazione SQL
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  // Il valore hard-coded verrà sostituito con una tabella dei tempi di campionamento variabili disponibili per ogni sensore
  if ((timestampNow - lastStoredTS) >= 5) {
    return true;
  } else {
    return false;
  }
}

void rilevoButtonPressureCallback() {
  buttonState = digitalRead(BUTTON_PIN);
  long long int result = parteComune(lastStoredTS1, "Button pressure", String(buttonState).c_str(), "Integer", "Assente", 2);
  if(result != -1) {
    lastStoredTS1 = result;
  }
}

void synchDataCallback() {
  if (WiFi.status() == WL_CONNECTED) {  // ed è passato abbastanza tempo >= 24h dall'ultima sincronizzazione
    Serial.println("Connessione WiFi attiva");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");
    Serial.println("---------------------------- Sincronizzazione ----------------------------");
    syncData(db);
  } else {
    Serial.println("Nessuna connessione WiFi");
  }
}

void cleanDataRoutineCallback() {
  Serial.println("---------------------------- Pulisco routine ----------------------------");
  garbageCollectorRoutine();
}

void cleanDataMemoryFullCallback() {
  /*
  if (raggiungo un 90 % di capienza adotto la mia politica) {
    Serial.println("---------------------------- Pulisco memory ----------------------------");
    garbageCollectorMemoryFull();
  } */
}

void gspTrackerCallback() {
  double lat = -1;
  double lon = -1;
  while (hs.available() > 0) {
    gps.encode(hs.read());
  }
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
  } else {
    Serial.println("INVALID LOCATION");
  }
  Serial.print(lat, 6);
  Serial.print(F(","));
  Serial.println(lon, 6);
  String res = String(lat, 6) + " - " + String(lon, 6);
  Serial.print("Coordinate gps: ");
  Serial.println(res);
  long long int result = parteComune(lastStoredTS2, "Coordinate GPS", res.c_str(), "String", "(°)", 3);
  if(result != -1) {
    lastStoredTS2 = result;
  }
}

// return lastStoredTS1 changed se è passato il tempo di storage, -1 otherwise
long long int parteComune(long long int lastStoredTS, char* nome_sensore, const char* valore, char* tipo, char* misura, int priority) {
  long long int res = -1;
  // Converte il timestamp attuale in secondi trascorsi dal 1970/01/01
  DateTime dt = rtc.now();
  long long int timestampNow = DateTimeToEpoch(dt);

  Serial.print(timestampNow);
  Serial.print(" - ");
  Serial.println(lastStoredTS);
  char b1[DATE_TIME_LEN];
  char b2[DATE_TIME_LEN];
  SecondsToString(timestampNow, b1);
  SecondsToString(lastStoredTS, b2);
  Serial.print(DateTimeToString(EpochToDateTime(timestampNow)));
  Serial.print(" - ");
  Serial.println(DateTimeToString(EpochToDateTime(lastStoredTS)));

  bool passed = spentEnoughTimeFromLastStrorage(nome_sensore, timestampNow, lastStoredTS);

  // Se è passato il tempo di storage, esegui le azioni desiderate
  if (passed) {
    String timeStamp = DateTimeToString(dt);
    long long int ts = stringToMilliseconds(timeStamp.c_str());
    createRecordToInsertIntot1(nome_sensore, valore, tipo, misura, ts, NO_SYNCHRONIZED, priority);
    res = timestampNow;
  } else {
    Serial.println("Non è trascorso abbastanza tempo dall'ultima storage del dato");
  }
  return res;
}