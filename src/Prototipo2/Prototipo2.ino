#include <TaskScheduler.h>
#include <LittleFS.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MPU6050.h>

#include "BluetoothSerial.h"
#include "ELMduino.h"
#include <HardwareSerial.h>

#include "Secrets.h"
#include "Config.h"

#include "LCtime.h"
#include "Storage.h"
#include "Filesystem.h"
#include "Synch.h"
#include "Garbage.h"

#include "Dato.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const uint16_t port = PORT;
const char* host = HOST;

static const uint32_t GPSBaud = 38400;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_MPU6050 mpu;

Scheduler scheduler;
sqlite3* db;
RTC_DS3231 rtc;
TinyGPSPlus gps;

BluetoothSerial SerialBT;
ELM327 myELM327;
// Indirizzo MAC del dispositivo Bluetooth a cui ci si desidera connettere
// Nel nostro caso OBDII: 1C:A1:35:69:8D:C5
uint8_t remoteAddres[] = { 0x1C, 0xA1, 0x35, 0x69, 0x8D, 0xC5 };

HardwareSerial hs = Serial2;

// Dato button
Dato button_Sensor("Button pressure", 20);
// GPS
Dato gps_Sensor("Coordinate GPS", 0);  // Si dovrebbe pensare ad una gerarchia? penso proprio di si poi vediamo
// Accelerometria
Dato accelerometro_Sensor("Accelerometria", 0);
// Alcool
Dato alcool_Sensor("Alcool", 30);
// Air Qulity
Dato airQuality_Sensor("Air quality", 30);
// RPM
/* Dato rpm_Sensor("rpm", 0);
// Temperatura liquido refrigerante
Dato temp_Sensor("liquido", 0); */

typedef enum { ENG_RPM,
               SPEED,
               TEMPERATURE,
               VOLTAGE,
               FUEL } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float RPM = 0;
float KPH = 0;
float TEMP = 0;
float VOLT = 0;
float BENZA = 0;

TaskHandle_t SchedulerTask;
TaskHandle_t OBDIITask;

// Prototipi delle funzioni chiamate dai task dello scheduler
void rilevoButtonPressureCallback();
void gspTrackerCallback();
void accelerometriaCallback();
void airMonitoringCallback();
// test
void loggingCallBack();

void synchDataCallback();
void cleanDataRoutineCallback();
void cleanDataMemoryFullCallback();

void schedulerTask(void* pvParameters);
// FreeRTOS task per letture OBD-II
void obdIITask(void* pvParameters);

// Definizione dei task
Task ButtonPressureTask(BUTTON_PRESSURE_DETECTION* TASK_SECOND, TASK_FOREVER, &rilevoButtonPressureCallback);
Task gpsTrackerTask(GPS* TASK_SECOND, TASK_FOREVER, &gspTrackerCallback);
Task synchDataTask(SYNCHRONIZATION_DATA* TASK_SECOND, TASK_FOREVER, &synchDataCallback);
Task cleanDataRoutineTask(CLEANING_ROUTINE* TASK_SECOND, TASK_FOREVER, &cleanDataRoutineCallback);
Task cleanDataMemoryFullTask(CLEANING_MEMORY_FULL* TASK_SECOND, TASK_FOREVER, &cleanDataMemoryFullCallback);
Task accelerometriaTask(ACCELEROMETRIA* TASK_SECOND, TASK_FOREVER, &accelerometriaCallback);
Task airMonitoringTask(AIR* TASK_SECOND, TASK_FOREVER, &airMonitoringCallback);
// test
Task loggingTask(CENTRALINA* TASK_SECOND, TASK_FOREVER, &loggingCallBack);

void setup() {

  Serial.begin(115200);

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  hs.begin(GPSBaud);

  // Display LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Benvenuto a");
  lcd.setCursor(0, 1);
  lcd.print("bordo");

  // LED
  pinMode(LED_ROSSO, OUTPUT);
  pinMode(LED_GIALLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  digitalWrite(LED_ROSSO, HIGH);
  digitalWrite(LED_GIALLO, HIGH);
  digitalWrite(LED_VERDE, LOW);

  // Inizializza l'accelerometro
  if (!mpu.begin(0x69)) {
    Serial.println("MPU6050 non trovato!");
    while (1)
      ;
  }
  Serial.println("MPU6050 trovato e inizializzato correttamente.");

  // Alcool e air quality
  pinMode(ALCOOL_PIN, INPUT);
  pinMode(AIR_QUALITY_PIN, INPUT);

  /* DA USARE PER LA VERSIONE UFFICIALE
  LittleFS.begin();
  if (!LittleFS.exists(DB_FILE)) {
    configurationProcess();
  } else {
    Serial.println("Database already exist");
  } */

  obdConfig();

  // La prima accensione va fatta in presenza di una connessione internet in modo tale da poter sincronizzare tutte le informazioni necessare per un corretto funzionamento futuro della Black Box
  configurationProcess();

  scheduler.init();

  scheduler.addTask(cleanDataRoutineTask);
  scheduler.addTask(cleanDataMemoryFullTask);
  scheduler.addTask(synchDataTask);
  scheduler.addTask(ButtonPressureTask);
  scheduler.addTask(gpsTrackerTask);
  scheduler.addTask(accelerometriaTask);
  scheduler.addTask(airMonitoringTask);
  scheduler.addTask(loggingTask);

  // cleanDataRoutineTask.enable();
  // cleanDataMemoryFullTask.enable();
  synchDataTask.enable();
  ButtonPressureTask.enable();
  gpsTrackerTask.enable();
  accelerometriaTask.enable();
  airMonitoringTask.enable();
  loggingTask.enable();

  xTaskCreatePinnedToCore(schedulerTask, "SchedulerTask", 10000, NULL, 2, &SchedulerTask, 1);
  xTaskCreatePinnedToCore(obdIITask, "OBDIITask", 4096, NULL, 1, &OBDIITask, 0);  

}

void loop() {

}

void obdConfig() {
  SerialBT.begin("ESP32_BT", true);  // Master
  Serial.println("Attempting to connect to ELM327...");

  if (!SerialBT.connect(remoteAddres)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 1");
    while (1) // bloccante
    ;
  }
  if (!myELM327.begin(SerialBT, true, 2000)) { // errori di timeout 
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1) // bloccante
    ;
  }
  Serial.println("Connected to ELM327");
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
  Serial.print(nameSensor);
  Serial.print("----");
  Serial.println(lastStoredTS);
  // Esegui la query
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // Il record è stato trovato, ora puoi recuperare i valori dei campi e visualizzarli
    long long int timestamp = sqlite3_column_int64(stmt, 4);  // Modifica qui se il timestamp è un intero a 64 bit
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

  // Verifico che sia passato abbastanza tempo dall'ultima storage del sensore coinvolto
  long long int tempoStorage = getTempoStorage(db, nameSensor);
  Serial.print("Tempo di storage: ");
  Serial.println(tempoStorage);
  Serial.print("Tempo di adesso: ");
  Serial.println(timestampNow);
  Serial.print("Tempo ultima storage: ");
  Serial.println(lastStoredTS);
  if ((timestampNow - lastStoredTS) >= tempoStorage) {
    return true;
  } else {
    return false;
  }
}

// BUTTON TASK
void rilevoButtonPressureCallback() {
  int buttonState = digitalRead(BUTTON_PIN);
  button_Sensor.addReading(buttonState);
  // Fine delle mie x letture e verifica + cambio del tempo di storage del dato in questione
  if (button_Sensor.currentIndex == button_Sensor.maxReadings) {
    changeTimeStorage(button_Sensor);
  }
  long long int result = parteComune(button_Sensor.lastStoredTS, button_Sensor.name, String(buttonState).c_str(), "Integer", "Assente", 2);
  if (result != -1) {
    button_Sensor.lastStoredTS = result;
  }
  Serial.print("Ultima mem button: ");
  Serial.println(button_Sensor.lastStoredTS);
}

// GPS TASK
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
  long long int result = parteComune(gps_Sensor.lastStoredTS, gps_Sensor.name, res.c_str(), "String", "(°)", 3);
  if (result != -1) {
    gps_Sensor.lastStoredTS = result;
  }
  Serial.print("Ultima mem gps: ");
  Serial.println(gps_Sensor.lastStoredTS);
}

// ACCELEROMETRIA TASK
void accelerometriaCallback() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x, y, z;
  x = a.acceleration.x;
  y = a.acceleration.y;
  z = a.acceleration.z;
  String res = String(x) + " - " + String(y) + " - " + String(z);
  Serial.println(res);
  long long int result = parteComune(accelerometro_Sensor.lastStoredTS, accelerometro_Sensor.name, res.c_str(), "String", "m/s^2", 5);
  if (result != -1) {
    accelerometro_Sensor.lastStoredTS = result;
  }
  Serial.print("Ultima mem accelerometria: ");
  Serial.println(accelerometro_Sensor.lastStoredTS);
}

// AIR MONITORING TASK
void airMonitoringCallback() {

  // Lettura sensore MQ-3
  int mq3AnalogValue = analogRead(ALCOOL_PIN);
  Serial.print("Alcool value detected: ");
  Serial.println(mq3AnalogValue);

  // Lettura sensore MQ-2
  int mq2AnalogValue = analogRead(AIR_QUALITY_PIN);
  Serial.print("Air quality value detected: ");
  Serial.println(mq2AnalogValue);

  long long int resultAlcool = parteComune(alcool_Sensor.lastStoredTS, alcool_Sensor.name, String(mq3AnalogValue).c_str(), "Integer", "Assente", 4);
  long long int resultAirQuality = parteComune(airQuality_Sensor.lastStoredTS, airQuality_Sensor.name, String(mq2AnalogValue).c_str(), "Integer", "Assente", 6);

  if (resultAlcool != -1) {
    alcool_Sensor.lastStoredTS = resultAlcool;
  }
  Serial.print("Ultima mem alcool: ");
  Serial.println(alcool_Sensor.lastStoredTS);

  if (resultAirQuality != -1) {
    airQuality_Sensor.lastStoredTS = resultAirQuality;
  }
  Serial.print("Ultima mem air quality: ");
  Serial.println(airQuality_Sensor.lastStoredTS);
}

void centralinaMonitoringCallBack() {
  switch (obd_state) {
    case ENG_RPM:
      {
        float rpm = myELM327.rpm();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          Serial.print("rpm: ");
          Serial.println(rpm);
          RPM = rpm;
          obd_state = SPEED;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = SPEED;
        }
        break;
      }

    case SPEED:
      {
        int32_t kph = myELM327.kph();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          Serial.print("kph: ");
          Serial.println(kph);
          KPH = kph;
          obd_state = TEMPERATURE;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = TEMPERATURE;
        }
        break;
      }

    case TEMPERATURE:
      {
        float temp = myELM327.engineCoolantTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          Serial.print("temp: ");
          Serial.println(temp);
          TEMP = temp;
          obd_state = VOLTAGE;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = VOLTAGE;
        }
        break;
      }

    case VOLTAGE:
      {
        float volt = myELM327.batteryVoltage();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          Serial.print("volt: ");
          Serial.println(volt);
          VOLT = volt;
          obd_state = FUEL;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = FUEL;
        }
        break;
      }

    case FUEL:
      {
        float fuel = myELM327.fuelLevel();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          Serial.print("fuel level: ");
          Serial.println(fuel);
          BENZA = fuel;
          obd_state = ENG_RPM;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = ENG_RPM;
        }
        break;
      }
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
    createRecordToInsertIntot1(nome_sensore, valore, tipo, misura, timestampNow, NO_SYNCHRONIZED, priority);
    res = timestampNow;
  } else {
    Serial.println("Non è trascorso abbastanza tempo dall'ultima storage del dato");
  }
  return res;
}

void changeTimeStorage(Dato& dato) {
  // attuo la mia politica di storage intelligente che cambia in base alla natura dei dati i tempi di storage
  // Calcola la deviazione standard delle letture
  float stdDev = dato.calculateStdDev();
  // Ottieni i parametri dal database
  int tempoStorage, fattoreIncrDecr, tMinStorage, tMaxStorage;
  float soglia;
  if (!getSensorParameters("Button pressure", tempoStorage, fattoreIncrDecr, soglia, tMinStorage, tMaxStorage)) {
    Serial.println("Failed to get sensor parameters.");
    return;
  }
  Serial.println(tempoStorage);
  Serial.println(fattoreIncrDecr);
  Serial.println(soglia);
  Serial.println(tMinStorage);
  // Aggiorna il tempo di storage in base alla deviazione standard
  if (stdDev <= soglia) {
    tempoStorage += fattoreIncrDecr;  // Incrementa il tempo di storage
  } else {
    tempoStorage -= fattoreIncrDecr;  // Decrementa il tempo di storage
    if (tempoStorage < tMinStorage) {
      tempoStorage = tMinStorage;  // Assicurati che non scenda sotto il minimo
    }
  }
  // Verifica se il nuovo tempo di storage supera il valore massimo consentito
  if (tempoStorage > tMaxStorage) {
    // Se supera il valore massimo, imposta il tempo di storage al valore massimo consentito
    tempoStorage = tMaxStorage;
    Serial.println("Il nuovo tempo di storage supera il valore massimo consentito. Viene impostato al valore massimo consentito.");
  }
  // Aggiorna il database con il nuovo tempo di storage
  updateTempoStorage("Button pressure", tempoStorage);
  dato.currentIndex = 0;
}

void loggingCallBack() {
  Serial.print("RPM: ");
  Serial.print(RPM, 2);  // Stampa con due decimali
  Serial.print(" | ");

  Serial.print("KPH: ");
  Serial.print(KPH, 2);  // Stampa con due decimali
  Serial.print(" | ");

  Serial.print("Temperature (C): ");
  Serial.print(TEMP, 2);  // Stampa con due decimali
  Serial.print(" | ");

  Serial.print("Voltage (V): ");
  Serial.print(VOLT, 2);  // Stampa con due decimali
  Serial.print(" | ");

  Serial.print("Fuel Level (%): ");
  Serial.println(BENZA, 2);  // Stampa con due decimali e termina con una nuova riga
}

void obdIITask(void* pvParameters) {
  while (true) {
    centralinaMonitoringCallBack();
  }
}

void schedulerTask(void* pvParameters) {
    while (true) {
        scheduler.execute();
    }
}