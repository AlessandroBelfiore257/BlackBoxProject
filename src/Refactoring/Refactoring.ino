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

// Accelerometria
Dato accelerometro_Sensor("Accelerometria", 0);
// Alcool
Dato alcool_Sensor("Alcool", 10);
// Air Qulity
Dato airQuality_Sensor("Qualità aria", 10);
// GPS
Dato gps_Sensor("Coordinate GPS", 0);  // Si dovrebbe pensare ad una gerarchia? penso proprio di si poi vediamo

typedef enum { ENG_RPM,
               SPEED,
               TEMPERATURE,
               VOLTAGE,
               LOAD } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float RPM = 0;
float KPH = 0;
float TEMP = 0;
float VOLT = 0;
float E_LOAD = 0;

int campionamento = 0;
int nCampionamentiRED = 0;
int nRilevamentiAlcool = 0;
int nRilevamentiAirQuality = 0;

int nCampionamentiY = 0;
int nRilevamentiAirQualityY = 0;
int SuperatoFumo = 0;

// Prototipi delle funzioni chiamate dai task dello scheduler
void gspTrackerCallback();
void accelerometriaCallback();
void airMonitoringCallback();
// test
void rpmCallback();
void speedCallback();
void temperatureCallback();
void voltageCallback();
void engineLoadCallback();
void loggingCallBack();

void synchDataCallback();
void cleanDataRoutineCallback();
void cleanDataMemoryFullCallback();

void profilazioneCallback();

void segnalazioneAllarmiRedCallback();
void segnalazioneAllarmiYellowCallback();

// Definizione dei task
Task gpsTrackerTask(GPS* TASK_SECOND, TASK_FOREVER, &gspTrackerCallback);
Task synchDataTask(SYNCHRONIZATION_DATA* TASK_SECOND, TASK_FOREVER, &synchDataCallback);
Task cleanDataRoutineTask(CLEANING_ROUTINE* TASK_SECOND, TASK_FOREVER, &cleanDataRoutineCallback);
Task cleanDataMemoryFullTask(CLEANING_MEMORY_FULL* TASK_SECOND, TASK_FOREVER, &cleanDataMemoryFullCallback);
Task accelerometriaTask(ACCELEROMETRIA* TASK_SECOND, TASK_FOREVER, &accelerometriaCallback);
Task airMonitoringTask(AIR* TASK_SECOND, TASK_FOREVER, &airMonitoringCallback);
// test
Task letturaRpmTask(TASK_SECOND, TASK_FOREVER, &rpmCallback);
Task letturaSpeedTask(TASK_SECOND, TASK_FOREVER, &speedCallback);
Task letturaTempTask(TASK_SECOND, TASK_FOREVER, &temperatureCallback);
Task letturaVoltTask(TASK_SECOND, TASK_FOREVER, &voltageCallback);
Task letturaLoadEngineTask(TASK_SECOND, TASK_FOREVER, &engineLoadCallback);
Task loggingTask(CENTRALINA* TASK_SECOND, TASK_FOREVER, &loggingCallBack);

Task profilazioneTask(10 * TASK_SECOND, TASK_FOREVER, &profilazioneCallback);

Task segnalazioneAllarmiRedTask(6 * TASK_SECOND, TASK_FOREVER, &segnalazioneAllarmiRedCallback);
Task segnalazioneAllarmiYellowTask(6 * TASK_SECOND, TASK_FOREVER, &segnalazioneAllarmiYellowCallback);

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

  // La prima accensione va fatta in presenza di una connessione internet in modo tale da poter sincronizzare tutte le informazioni necessare per un corretto funzionamento futuro della Black Box
  configurationProcess();

  scheduler.init();

  scheduler.addTask(cleanDataRoutineTask);
  scheduler.addTask(cleanDataMemoryFullTask);
  scheduler.addTask(synchDataTask);
  scheduler.addTask(gpsTrackerTask);
  scheduler.addTask(accelerometriaTask);
  scheduler.addTask(airMonitoringTask);
  scheduler.addTask(loggingTask);

  /*
  scheduler.addTask(letturaRpmTask);
  scheduler.addTask(letturaSpeedTask);
  scheduler.addTask(letturaTempTask);
  scheduler.addTask(letturaVoltTask);
  scheduler.addTask(letturaLoadEngineTask); */
  scheduler.addTask(profilazioneTask);
  // scheduler.addTask(segnalazioneAllarmiRedTask);
  scheduler.addTask(segnalazioneAllarmiYellowTask);

  // cleanDataRoutineTask.enable();
  // cleanDataMemoryFullTask.enable();
  synchDataTask.enable();
  gpsTrackerTask.enable();
  accelerometriaTask.enable();
  airMonitoringTask.enable();
  loggingTask.enable();

  profilazioneTask.enable();
  // segnalazioneAllarmiRedTask.enable();
  segnalazioneAllarmiYellowTask.enable();

  /*
  letturaRpmTask.enable();
  letturaSpeedTask.enable();
  letturaTempTask.enable();
  letturaVoltTask.enable();
  letturaLoadEngineTask.enable(); */
  /*
  SerialBT.begin("ESP32_BT", true);  // Master
  Serial.println("Attempting to connect to ELM327...");

  if (!SerialBT.connect(remoteAddres)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 1");
  }

  if (!myELM327.begin(SerialBT, true, 5000)) {  // Problema !!! errori di timeout
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
  }
  Serial.println("Connected to ELM327"); */
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
  String res = String(lat, 6) + " / " + String(lon, 6);
  Serial.print("Coordinate gps: ");
  Serial.println(res);
  long long int result = parteComune(gps_Sensor.lastStoredTS, gps_Sensor.name, res.c_str(), "String", "°", 5);
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
  String res = String(x) + " / " + String(y) + " / " + String(z);
  Serial.println(res);
  long long int result = parteComune(accelerometro_Sensor.lastStoredTS, accelerometro_Sensor.name, res.c_str(), "String", "m/s^2", 6);
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

  long long int resultAlcool = parteComune(alcool_Sensor.lastStoredTS, alcool_Sensor.name, String(mq3AnalogValue).c_str(), "Integer", "Assente per adesso", 2);
  long long int resultAirQuality = parteComune(airQuality_Sensor.lastStoredTS, airQuality_Sensor.name, String(mq2AnalogValue).c_str(), "Integer", "Assente per adesso", 2);

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
    syncT3Data(db);
    syncT4Data(db);
    syncT5Data(db);
  } else {
    Serial.println("Nessuna connessione WiFi");
  }
}

void cleanDataRoutineCallback() {
  Serial.println("---------------------------- Pulisco routine ----------------------------");
  garbageCollectorRoutine();
  cleanAllarmiRed(db);
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

  Serial.print("Engine load (%): ");
  Serial.println(E_LOAD, 2);  // Stampa con due decimali e termina con una nuova riga
}

void rpmCallback() {
  float rpm = myELM327.rpm();
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    Serial.print("rpm: ");
    Serial.println(rpm);
    RPM = rpm;
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
    myELM327.printError();
  }
}

void speedCallback() {
  int32_t speed = myELM327.kph();
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    Serial.print("speed: ");
    Serial.println(speed);
    KPH = speed;
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
    myELM327.printError();
  }
}

void temperatureCallback() {
  float temp = myELM327.engineCoolantTemp();
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    Serial.print("engine coolant temperature: ");
    Serial.println(temp);
    TEMP = temp;
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
    myELM327.printError();
  }
}

void voltageCallback() {
  float volt = myELM327.batteryVoltage();
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    Serial.print("battery voltage: ");
    Serial.println(volt);
    VOLT = volt;
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
    myELM327.printError();
  }
}

void engineLoadCallback() {
  float e_load = myELM327.engineLoad();
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    Serial.print("engine load: ");
    Serial.println(e_load);
    E_LOAD = e_load;
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
    myELM327.printError();
  }
}

void profilazioneCallback() {
  campionamento++;
  if (campionamento == 3) {  // (3 fa riferimento al numero di campionamenti che si desidera) and è trascorso un giorno dall'ultima storage (if seguente)
    DateTime now = rtc.now();
    long long int timestamp = DateTimeToEpoch(now);
    long long int last = getLastTimestampFromt3(db);
    Serial.print("Ultima registrazione effettuata (VOTO): ");
    Serial.println(last);
    if (timestamp - last >= 0) {  // al posto dello 0 andranno inseriti i secondi presenti all'interno di una giornata (86400)
      campionamento = 0;
      Serial.println("--------------------------------------- ENTRATO ---------------------------------------");
      // Il voto, randomico adesso, deve essere calcolasto in base ai parametri citati sulla descrizione del progetto
      long randNumber = random(0, 100);
      float normalizedNumber = (float)randNumber / 100.0;
      char normalizedNumberStr[10];
      snprintf(normalizedNumberStr, sizeof(normalizedNumberStr), "%.2f", normalizedNumber);
      char dateTimeStr[20];
      sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d",
              now.year(), now.month(), now.day(),
              now.hour(), now.minute(), now.second());
      createRecordToInsertIntot3(dateTimeStr, normalizedNumberStr, timestamp, NO_SYNCHRONIZED);
      // Se sono presenti più di x voti già sincronizzati pulisco la tabella
      //eliminazioneVotiProfilazione(db);
    }
  }
}

void segnalazioneAllarmiRedCallback() {
  nCampionamentiRED++;
  int alcool = analogRead(ALCOOL_PIN);
  int airQuality = analogRead(AIR_QUALITY_PIN);
  if (alcool > 15) {
    nRilevamentiAlcool++;
  }
  if (airQuality < 0) {
    nRilevamentiAirQuality++;
  }
  if (nCampionamentiRED == 6) {  // bisogna mettere un valore molto alto dato che alcuni valori potrebbero essere errati per vari motivi + si vorrebbe essere il più precisi possibili
    // se noto che più della metà delle rilevazioni sono favorevoli alla presenza di alcool significa che è presente nell'aria
    Serial.print("Allarmi: ");
    Serial.print(nRilevamentiAlcool);
    Serial.print(" - ");
    Serial.println(nRilevamentiAirQuality);
    normalDisplay();
    Serial.println("-------------------------------- ALLARMI --------------------------------");
    if ((nRilevamentiAlcool / 6 > 0.5) && alreadyExists(db,"Alcool") == false) {
      Serial.println("-------------------------------- Problema rilevato --------------------------------");
      // Problema: store + lcd + led 
      String problema = "Alcool";
      DateTime now = rtc.now();
      char dateTimeStr[20];
      sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d",
              now.year(), now.month(), now.day(),
              now.hour(), now.minute(), now.second());
      createRecordToInsertIntot4(problema.c_str(), dateTimeStr, NO_SYNCHRONIZED);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rilevato alcool");
      lcd.setCursor(0, 1);
      lcd.print("nell'abitacolo !");
      digitalWrite(LED_ROSSO, LOW);
      digitalWrite(LED_VERDE, HIGH);
    }
    if (nRilevamentiAirQuality / 6 > 0.5) {
      // Problema: store + lcd
    }
    nRilevamentiAlcool = 0;
    nRilevamentiAirQuality = 0;
    nCampionamentiRED = 0;
  }
}

void segnalazioneAllarmiYellowCallback() {
  nCampionamentiY++;
  int airQuality = analogRead(AIR_QUALITY_PIN);
  // andranno inseriti gli altri 3 fuori strada / range GPS / trasporto/rimorchio
  if (airQuality > 0) {
    nRilevamentiAirQualityY++;
  }
  if (nCampionamentiY == 6) {  // bisogna mettere un valore molto alto dato che alcuni valori potrebbero essere errati per vari motivi + si vorrebbe essere il più precisi possibili
    // se noto che più della metà delle rilevazioni sono favorevoli alla presenza di alcool significa che è presente nell'aria
    Serial.print("Allarmi Gialli: ");
    Serial.println(nRilevamentiAirQualityY);
    normalDisplay();
    Serial.println("-------------------------------- ALLARMI GIALLI --------------------------------");
    normalDisplay();
    if ((nRilevamentiAirQualityY / 6 > 0.5) && SuperatoFumo == 0) {
      // Problema: store + lcd + led 
      String problema = "Fumo";
      String rilevamenti = "rilevamenti";
      String soglia = "soglia";
      String daMandare = "daMandare";
      int prec = getValueFromT5(problema.c_str(), rilevamenti.c_str());
      int s = getValueFromT5(problema.c_str(), soglia.c_str());
      if(prec + 1 == s) {
        Serial.println("-------------------------------- Problema fumo rilevato e memorizzato --------------------------------");
        Serial.println("Rilevato fumo 3 volte");
        updateColumnValue(problema.c_str(), rilevamenti.c_str(), prec + 1);
        updateColumnValue(problema.c_str(), daMandare.c_str(), 1); 
        SuperatoFumo = 1;
      } else {
        updateColumnValue(problema.c_str(), rilevamenti.c_str(), prec + 1);
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rilevato fumo");
      lcd.setCursor(0, 1);
      lcd.print("nell'abitacolo !");
      digitalWrite(LED_GIALLO, LOW);
      digitalWrite(LED_VERDE, HIGH);
    }
    nCampionamentiY = 0;
    nRilevamentiAirQualityY = 0;
  }
}

void normalDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nessun problema");
  lcd.setCursor(0, 1);
  lcd.print("rilevato");
  digitalWrite(LED_ROSSO, HIGH);
  digitalWrite(LED_GIALLO, HIGH);
  digitalWrite(LED_VERDE, LOW);
}