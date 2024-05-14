#include <stdio.h>
#include <stdlib.h>
#include <TaskScheduler.h>
#include <LittleFS.h>

#include "Storage.h"
#include "LCtime.h"

#include "Config.h"
#include "Secrets.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const uint16_t port = PORT;
const char* host = HOST;
const int BUTTON_PIN = BUTTON;
const char* data = "Callback function called";

Scheduler scheduler;
WiFiClient client;
sqlite3* db;
struct tm timeinfo;
// Rimpiazzo le variabili sottostanti con quelle dei sensori veri e propri della BB
// Sensore numero casuale
int mydata = 0;
// Sensore button
int buttonState = 0;

int rc = 0;
bool passed = false;
long long int lastStoredTS = 0;

void rilevoButtonPressureCallback();
//void rilevoCasualNumberCallback();

Task ButtonPressureTask(TASK_BUTTON_PRESSURE, TASK_FOREVER, &rilevoButtonPressureCallback);
//Task CasualNumberTask(TASK_CASUAL_NUMBER, TASK_FOREVER, &rilevoCasualNumberCallback);

void setup() {
  Serial.begin(115200);

  setTimeIT();

  // Richiesta di connessione che andrà in un task (polling)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected\n");

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
  client.connect(host, port);
}

void loop() {
  scheduler.execute();
}

void rilevoButtonPressureCallback() {
  buttonState = digitalRead(BUTTON_PIN);

  // Converte il timestamp attuale in millisecondi
  struct tm timeinfo = getDateAndTime();
  time_t timestampNow_ret = mktime(&timeinfo);
  unsigned long timestampNow = timestampNow_ret;

  if (db_open("/littlefs/dati.db", &db))
    return;
  // Query e verifico se è passato il tempo di campionamento:

  const char* sql = "SELECT * FROM t1 WHERE nome_sensore = ? AND timestamp = ?";
  // Prepara la dichiarazione SQL
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println("Errore durante la preparazione della query SQL");
    sqlite3_close(db);
    return;
  }
  // Associa i valori dei parametri (nome sensore e timestamp) alla query
  sqlite3_bind_text(stmt, 1, "Button pressure", -1, SQLITE_TRANSIENT);
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
    passed = true;
  } else {
    passed = false;
  }
  Serial.print(timestampNow);
  Serial.print(" - ");
  Serial.println(lastStoredTS);
  char b1[20];
  char b2[20];
  SecondsToString(timestampNow, b1);
  SecondsToString(lastStoredTS, b2);
  Serial.print(b1);
  Serial.print(" - ");
  Serial.println(b2);

  // Se è passato il tempo di campionamento, esegui le azioni desiderate
  if (passed) {
    String timeStamp = createTimeStampStringFormat(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    int syn = NO_SYNCHRONIZED;
    long long int ts = stringToMilliseconds(timeStamp.c_str());
    createRecordToInsertIntot1("Button pressure", buttonState, ts, syn, 2);
    lastStoredTS = timestampNow;

    // Fase di synch (In seguito)
    if (!client.connected()) {
    if (!client.connect(host, port)) {
      Serial.println("Connection to host failed");
      return;
    } else {
      Serial.println("Connected to server successful!");
    }
  }
  SendToServer("Button pressure", buttonState, b1, 0, 2);
  } else {
    Serial.println("Mancato");
  }
}

void rilevoCasualNumberCallback() {
  mydata = random(0, 1000);

  if (db_open("/littlefs/dati.db", &db))
    return;
  struct tm timeinfo = getDateAndTime();
  String timeStamp = createTimeStampStringFormat(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  int syn = NO_SYNCHRONIZED;
  long long int ts = stringToMilliseconds(timeStamp.c_str());
  createRecordToInsertIntot1("Casual number", mydata, ts, syn, 1);
  rc = db_exec(db, "SELECT * FROM t1");
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    return;
  }

  sqlite3_close(db);

  // Fase di synch (In seguito)
  /*
  if (!client.connected()) {
    if (!client.connect(host, port)) {
      Serial.println("Connection to host failed");
      return;
    } else {
      Serial.println("Connected to server successful!");
    }
  }
  SendToServer("Casual number", mydata, fattaAmano.c_str(), syn, 1);
  */
} 

/*
  Invio al server le informazioni utili
*/
void SendToServer(char* name_sensor, int value, const char* timestamp, int synchronized, int priority) {
  client.print(String(name_sensor) + "," + String(value) + "," + timestamp + "," + String(synchronized) + "," + String(priority) + "\n");
}

/*
  Creazione timestamp sotto forma di stringa nel formato "YYYY-MM-DD HH:MM:SS"
*/
String createTimeStampStringFormat(int day, int month, int year, int hour, int minute, int second) {
  return String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second);
}

/*
  Funzione per convertire un timestamp sotto forma di stringa in secondi
*/
long long int stringToMilliseconds(const char* timestamp) {
  struct tm tm;
  strptime(timestamp, "%Y-%m-%d %H:%M:%S", &tm);
  time_t time_seconds = mktime(&tm);
  return time_seconds;
}

// Funzione per convertire un timestamp sotto forma di secondi in una stringa nel formato "YYYY-MM-DD HH:MM:SS"
void SecondsToString(long long int total_seconds, char* date_string) {
  time_t raw_time = (time_t)total_seconds;
  struct tm* timeinfo = localtime(&raw_time);
  strftime(date_string, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}
