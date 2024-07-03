#include <TaskScheduler.h>
#include <LittleFS.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MPU6050.h>

#include "BluetoothSerial.h"
#include "ELMduino.h"

#include "Secrets.h"
#include "Config.h"

#include "LCtime.h"
#include "Storage.h"
#include "Filesystem.h"
#include "Synch.h"
#include "Garbage.h"
#include "Dato.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_MPU6050 mpu;
Scheduler scheduler;
sqlite3* db;
RTC_DS3231 rtc;
TinyGPSPlus gps;
HardwareSerial ss = Serial2;

BluetoothSerial SerialBT;
ELM327 myELM327;
// Indirizzo MAC del dispositivo Bluetooth a cui ci si desidera connettere
// Nel nostro caso OBDII scanner: 10:21:3E:4C:6F:3A
uint8_t remoteAddress[] = { 0x10, 0x21, 0x3E, 0x4C, 0x6F, 0x3A };

Dato accelerometro_Sensor("Accelerometria", max_readings_Accelerometria);
Dato alcool_Sensor("Alcool", max_readings_Alcool);
Dato airQuality_Sensor("Qualità aria", max_readings_AirQuality);
Dato gps_Sensor("Coordinate GPS", max_readings_GPS);

typedef enum {
  // PROFILAZIONE
  ENG_RPM,   // Giri al minuto del motore
  KPH,       // Velocità del veicolo in chilometri orari
  THROTTLE,  // Posizione della valvola a farfalla
  LOAD,      // Carico del motore in percentuale
  // ALLARMI
  COOLANT_TEMP,     // Temperatura del liquido di raffreddamento
  TORQUE,           // Coppia del motore
  BATTERY_VOLTAGE,  // Voltaggio della batteria
  OIL_TEMP,         // Temperatura dell'olio motore
  FUEL_LEVEL        // Livello del carburante in percentuale
  // MONITORAGGIO: si può scegliere ciò che si ritiene essere più significativo
} obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float rpm = 0;
float kph = 0;
float throttle = 0;
float load = 0;
float coolant_temp = 0;
float torque = 0;
float battery_voltage = 0;
float oil_temp = 0;
float fuel_level = 0;

int campionamento_Profilazione = 0;

int campionamento_AllarmiRED = 0;
int nRilevamentiAlcool = 0;
int nRilevamentiGas = 0;
int nRilevamentiTempEngine = 0;
int nRilevamentiTempOil = 0;
int nRilevamentiCoppiaEngine = 0;

int campionamento_AllarmiYellow = 0;
int nRilevamentiFumo = 0;
int nRilevamentiFuoriStrada = 0;
int nRilevamentiFuoriNazione = 0;
int nRilevamentiRimorchi = 0;
int Superata_Soglia_Fumo = 0;
int Superata_Soglia_FuoriStrada = 0;
int Superata_Soglia_RangeGPS = 0;
int Superata_Soglia_Rimorchi = 0;

void accelerometriaCallback();
void airMonitoringCallback();
void gspTrackerCallback();

void profilazioneCallback();
void segnalazioneAllarmiRedCallback();
void segnalazioneAllarmiYellowCallback();

void synchDataCallback();
void cleanDataRoutineCallback();
void cleanDataMemoryFullCallback();
void cleanAllarmCallback();

void obdComunicationCallback();
void loggingCallBack();

Task accelerometriaTask(ACCELEROMETRIA* TASK_SECOND, TASK_FOREVER, &accelerometriaCallback);
Task airMonitoringTask(AIR* TASK_SECOND, TASK_FOREVER, &airMonitoringCallback);
Task gpsTrackerTask(GPS* TASK_SECOND, TASK_FOREVER, &gspTrackerCallback);

Task profilazioneTask(PROFILAZIONE* TASK_SECOND, TASK_FOREVER, &profilazioneCallback);
Task segnalazioneAllarmiRedTask(ALLARMI* TASK_SECOND, TASK_FOREVER, &segnalazioneAllarmiRedCallback);
Task segnalazioneAllarmiYellowTask(ALLARMI* TASK_SECOND, TASK_FOREVER, &segnalazioneAllarmiYellowCallback);

Task synchDataTask(SYNCHRONIZATION_DATA* TASK_SECOND, TASK_FOREVER, &synchDataCallback);
Task cleanDataRoutineTask(CLEANING_ROUTINE* TASK_SECOND, TASK_FOREVER, &cleanDataRoutineCallback);
Task cleanDataMemoryFullTask(CLEANING_MEMORY_FULL* TASK_SECOND, TASK_FOREVER, &cleanDataMemoryFullCallback);
Task cleanAllarmTask(WEEK* TASK_SECOND, TASK_FOREVER, &cleanAllarmCallback);

Task obdTask(OBD, TASK_FOREVER, &obdComunicationCallback);
Task loggingTask(CENTRALINA_LOG* TASK_SECOND, TASK_FOREVER, &loggingCallBack);

void setup() {

  Serial.begin(115200);

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  ss.begin(GPSBAUD);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Benvenuto a");
  lcd.setCursor(0, 1);
  lcd.print("bordo");

  pinMode(LED_ROSSO, OUTPUT);
  pinMode(LED_GIALLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  digitalWrite(LED_ROSSO, HIGH);
  digitalWrite(LED_GIALLO, HIGH);
  digitalWrite(LED_VERDE, LOW);

  if (!mpu.begin(0x69)) {
    Serial.println("MPU6050 non trovato!");
  }
  Serial.println("MPU6050 trovato e inizializzato correttamente.");

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
  // Da integrare meglio con le righe sopra di codice in caso di riavvio della blacl box in assenza di internet
  configurationProcess();

  scheduler.init();

  scheduler.addTask(accelerometriaTask);
  scheduler.addTask(airMonitoringTask);
  scheduler.addTask(gpsTrackerTask);
  scheduler.addTask(synchDataTask);
  scheduler.addTask(cleanDataRoutineTask);
  scheduler.addTask(cleanDataMemoryFullTask);
  scheduler.addTask(obdTask);
  scheduler.addTask(loggingTask);

  scheduler.addTask(profilazioneTask);
  // scheduler.addTask(segnalazioneAllarmiRedTask);
  scheduler.addTask(segnalazioneAllarmiYellowTask);
  // scheduler.addTask(cleanAllarmTask);

  // cleanDataRoutineTask.enable();
  // cleanDataMemoryFullTask.enable();
  // cleanAllarmTask.enable();
  accelerometriaTask.enable();
  airMonitoringTask.enable();
  gpsTrackerTask.enable();
  synchDataTask.enable();
  obdTask.enable();
  loggingTask.enable();

  profilazioneTask.enable();
  // segnalazioneAllarmiRedTask.enable();
  segnalazioneAllarmiYellowTask.enable();

  obdConfig();
}

void loop() {
  scheduler.execute();
}

void configurationProcess() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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

void obdConfig() {
  SerialBT.begin("ESP32_MASTER", true);
  if (!SerialBT.connect(remoteAddress)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 1");
    while (1)
      ;
  }
  if (!myELM327.begin(SerialBT, false, 5000)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1)
      ;
  }
  Serial.println("Connected to ELM327");
}

bool spentEnoughTimeFromLastStrorage(char* nameSensor, long long int timestampNow, long long int lastStoredTS) {
  if (db_open(PATH_STORAGE_DATA_DB, &db))
    return false;
  // Query e verifico se è passato il tempo di campionamento:
  const char* sql = "SELECT * FROM t1 WHERE nome_sensore = ? AND timestamp = ?";
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
  long long int result = processSensorData(accelerometro_Sensor.lastStoredTS, accelerometro_Sensor.name, res.c_str(), "String", "m/s^2", 6);
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

  long long int resultAlcool = processSensorData(alcool_Sensor.lastStoredTS, alcool_Sensor.name, String(mq3AnalogValue).c_str(), "Integer", "Assente per adesso", 2);
  long long int resultAirQuality = processSensorData(airQuality_Sensor.lastStoredTS, airQuality_Sensor.name, String(mq2AnalogValue).c_str(), "Integer", "Assente per adesso", 2);

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

// GPS TASK
void gspTrackerCallback() {
  double lat = -1;
  double lon = -1;
  while (ss.available() > 0) {
    gps.encode(ss.read());
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
  long long int result = processSensorData(gps_Sensor.lastStoredTS, gps_Sensor.name, res.c_str(), "String", "°", 5);
  if (result != -1) {
    gps_Sensor.lastStoredTS = result;
  }
  Serial.print("Ultima mem gps: ");
  Serial.println(gps_Sensor.lastStoredTS);
}

void synchDataCallback() {
  if (WiFi.status() == WL_CONNECTED) {  // ed è passato abbastanza tempo >= 24h dall'ultima sincronizzazione
    Serial.println("Connessione WiFi attiva");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
}

void cleanDataMemoryFullCallback() {
  /*
  if (raggiungo un 90 % di capienza adotto la mia politica) {
    Serial.println("---------------------------- Pulisco memory ----------------------------");
    garbageCollectorMemoryFull();
  } */
}

// Restituisce l'ultimo tempo di memorizzazione se è passato il tempo di storage, -1 altrimenti
long long int processSensorData(long long int lastStoredTS, char* nome_sensore, const char* valore, char* tipo, char* misura, int priority) {
  long long int res = -1;
  DateTime dt = rtc.now();
  long long int timestampNow = DateTimeToEpoch(dt);
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
  // Attuo la mia politica di storage intelligente che cambia in base alla natura dei dati i tempi di storage
  // Calcola la deviazione standard delle letture
  float stdDev = dato.calculateStdDev();
  // Ottieni i parametri dal database
  int tempoStorage, fattoreIncrDecr, tMinStorage, tMaxStorage;
  float soglia;
  if (!getSensorParameters(dato.name, tempoStorage, fattoreIncrDecr, soglia, tMinStorage, tMaxStorage)) {
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
  }
  // Verifica se il nuovo tempo di storage supera il valore massimo consentito
  if (tempoStorage > tMaxStorage) {
    tempoStorage = tMaxStorage;
  }
  // Verifica se il nuovo tempo di storage non scenda sotto al valore minimo consentito
  if (tempoStorage < tMinStorage) {
    tempoStorage = tMinStorage;
  }
  // Aggiorna il database con il nuovo tempo di storage
  updateTempoStorage(dato.name, tempoStorage);
  dato.currentIndex = 0;
}

void loggingCallBack() {
  Serial.print("Rpm (giri/min): "); Serial.print(rpm); Serial.print(" | ");
  Serial.print("Kph (km/h): "); Serial.print(kph); Serial.print(" | ");
  Serial.print("Throttle position (%): "); Serial.print(throttle); Serial.print(" | ");
  Serial.print("Engine load (%): "); Serial.print(load); Serial.print(" | ");
  Serial.print("Coolant temp (°C): "); Serial.print(coolant_temp); Serial.print(" | ");
  Serial.print("Torque (Nm): "); Serial.print(torque); Serial.print(" | ");
  Serial.print("Battery voltage (V): "); Serial.print(battery_voltage, 2); Serial.print(" | ");
  Serial.print("Oil temp (°C): "); Serial.print(oil_temp); Serial.print(" | ");
  Serial.print("Fuel level (%): "); Serial.println(fuel_level);
}

void profilazioneCallback() {
  campionamento_Profilazione++;
  if (campionamento_Profilazione == MAX_CAMPIONAMENTI_PROFILAZIONE) {  // (3 fa riferimento al numero di campionamenti che si desidera) && è trascorso un giorno dall'ultima storage (if seguente)
    DateTime now = rtc.now();
    long long int timestamp = DateTimeToEpoch(now);
    long long int last = getLastTimestampFromt3(db);
    if (timestamp - last >= 0) {  // al posto dello 0 andranno inseriti i secondi presenti all'interno di una giornata (86400) dato che vorrei profilare al più una volta al giorno
      campionamento_Profilazione = 0;
      Serial.println("--------------------------------------- PROFILAZIONE ---------------------------------------");
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
      // eliminazioneVotiProfilazione(db);
    }
  }
}

void segnalazioneAllarmiRedCallback() {
  campionamento_AllarmiRED++;
  // Alcool
  int alcool = analogRead(ALCOOL_PIN);
  if (alcool > 15) {
    nRilevamentiAlcool++;
  }
  // Gas abitacolo
  int gas = analogRead(AIR_QUALITY_PIN);
  if (gas < 0) {
    nRilevamentiGas++;
  }
  // Temperatura liquido di raffreddamento
  if (coolant_temp > MAX_COOLANT_TEMP) {
    nRilevamentiTempEngine++;
  }
  // Temperatura olio motore
  if (oil_temp > MAX_OIL_TEMP) {
    nRilevamentiTempOil++;
  }
  // Coppia motore
  if (torque < MIN_TORQUE_THRESHOLD || torque > MAX_TORQUE_THRESHOLD) {
    nRilevamentiCoppiaEngine++;
  }
  // Consumo anomalo di carburante
  // Distanza di frenata anomala
  if (campionamento_AllarmiRED == MAX_CAMPIONAMENTI_ALLARMI_ROSSI) {  // bisogna mettere un valore molto alto dato che alcuni valori potrebbero essere errati per vari motivi + si vorrebbe essere il più precisi possibili
    regularDisplay();
    DateTime now = rtc.now();
    char dateTimeStr[20];
    sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    Serial.println("-------------------------------- ALLARMI ROSSI --------------------------------");
    // se noto che più della metà delle rilevazioni sono favorevoli alla presenza di alcool significa che è presente nell'aria
    if ((nRilevamentiAlcool / MAX_CAMPIONAMENTI_ALLARMI_ROSSI > 0.5) && alreadyExists(db, "Alcool") == false) {
      String problema = "Alcool";
      Serial.println("-------------------------------- Problema alcool rilevato e memorizzato --------------------------------");
      // Problema: store + lcd + led
      createRecordToInsertIntot4(problema.c_str(), dateTimeStr, NO_SYNCHRONIZED);
      visualizzaRedProblem("Rilevato alcool", "nell'abitacolo !");
    }
    if ((nRilevamentiGas / MAX_CAMPIONAMENTI_ALLARMI_ROSSI > 0.5) && alreadyExists(db, "Gas") == false) {
      Serial.println("-------------------------------- Problema gas rilevato e memorizzato --------------------------------");
      // Problema: store + lcd + led
      String problema = "Gas";
      createRecordToInsertIntot4(problema.c_str(), dateTimeStr, NO_SYNCHRONIZED);
      visualizzaRedProblem("Rilevato gas", "nell'abitacolo !");
    }
    if ((nRilevamentiTempEngine / MAX_CAMPIONAMENTI_ALLARMI_ROSSI > 0.5) && alreadyExists(db, "Liquido raffreddamento") == false) {
      Serial.println("-------------------------------- Problema liquido di raffreddamento rilevato e memorizzato --------------------------------");
      // Problema: store + lcd + led
      String problema = "Liquido raffreddamento";
      createRecordToInsertIntot4(problema.c_str(), dateTimeStr, NO_SYNCHRONIZED);
      visualizzaRedProblem("Temperatura", "raffreddamento alta!");
    }
    if ((nRilevamentiTempOil / MAX_CAMPIONAMENTI_ALLARMI_ROSSI > 0.5) && alreadyExists(db, "Olio motore") == false) {
      Serial.println("-------------------------------- Problema olio motore rilevato e memorizzato --------------------------------");
      // Problema: store + lcd + led
      String problema = "Olio motore";
      createRecordToInsertIntot4(problema.c_str(), dateTimeStr, NO_SYNCHRONIZED);
      visualizzaRedProblem("Temperatura olio", "motore alta!");
    }
    if ((nRilevamentiCoppiaEngine / MAX_CAMPIONAMENTI_ALLARMI_ROSSI > 0.5) && alreadyExists(db, "Coppia motore") == false) {
      Serial.println("-------------------------------- Problema coppia motore rilevato e memorizzato --------------------------------");
      // Problema: store + lcd + led
      String problema = "Coppia motore";
      createRecordToInsertIntot4(problema.c_str(), dateTimeStr, NO_SYNCHRONIZED);
      visualizzaRedProblem("Malfunzionamento", "coppia motore !");
    }
    campionamento_AllarmiRED = 0;
    nRilevamentiAlcool = 0;
    nRilevamentiGas = 0;
    nRilevamentiTempEngine = 0;
    nRilevamentiTempOil = 0;
    nRilevamentiCoppiaEngine = 0;
  }
}

void segnalazioneAllarmiYellowCallback() {
  campionamento_AllarmiYellow++;
  // Fumo
  int airQuality = analogRead(AIR_QUALITY_PIN);
  if (airQuality > 0) {
    nRilevamentiFumo++;
  }
  /*
  // Fuori strada
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float z = a.acceleration.z;
  if (z < (ACCELERAZIONE_G - TOLLERANZA_OFF_ROAD) || z > (ACCELERAZIONE_G + TOLLERANZA_OFF_ROAD)) {
    nRilevamentiFuoriStrada++;
  }
  // Range GPS
  double lat = -1;
  double lon = -1;
  if (ss.available() > 0) {
    gps.encode(ss.read());
  }
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
    if (!isInsideNation(lat, lon)) {
      nRilevamentiFuoriNazione++;
    }
  }
  // Trasporto/rimorchio
  if (isTrailerAttached()) {
    nRilevamentiRimorchi++;
  }
  */
  if (campionamento_AllarmiYellow == MAX_CAMPIONAMENTI_ALLARMI_GIALLI) {  // bisogna mettere un valore molto alto dato che alcuni valori potrebbero essere errati per vari motivi + si vorrebbe essere il più precisi possibili
    regularDisplay();
    Serial.println("-------------------------------- ALLARMI GIALLI --------------------------------");
    // se noto che più della metà delle rilevazioni sono favorevoli alla presenza di fumo significa che è presente nell'abitacolo
    String rilevamenti = "rilevamenti";
    String soglia = "soglia";
    String da_Mandare = "daMandare";
    if ((nRilevamentiFumo / MAX_CAMPIONAMENTI_ALLARMI_GIALLI > 0.5) && Superata_Soglia_Fumo == 0) {
      // Problema: store + lcd + led
      String problema = "Fumo";
      int rilevamenti_precedenti = getValueFromT5(problema.c_str(), rilevamenti.c_str());
      int theshold = getValueFromT5(problema.c_str(), soglia.c_str());
      if (rilevamenti_precedenti + 1 == theshold) {
        Serial.println("-------------------------------- Problema fumo rilevato e memorizzato --------------------------------");
        updateColumnValue(problema.c_str(), da_Mandare.c_str(), 1);
        Superata_Soglia_Fumo = 1;
      }
      updateColumnValue(problema.c_str(), rilevamenti.c_str(), rilevamenti_precedenti + 1);
      visualizzaYellowProblem("Rilevato fumo", "nell'abitacolo !");
    }
    if ((nRilevamentiFuoriStrada / MAX_CAMPIONAMENTI_ALLARMI_GIALLI > 0.5) && Superata_Soglia_FuoriStrada == 0) {
      // Problema: store + lcd + led
      String problema = "Fuori strada";
      int rilevamenti_precedenti = getValueFromT5(problema.c_str(), rilevamenti.c_str());
      int theshold = getValueFromT5(problema.c_str(), soglia.c_str());
      if (rilevamenti_precedenti + 1 == theshold) {
        Serial.println("-------------------------------- Problema fuori strada rilevato e memorizzato --------------------------------");
        updateColumnValue(problema.c_str(), da_Mandare.c_str(), 1);
        Superata_Soglia_FuoriStrada = 1;
      }
      updateColumnValue(problema.c_str(), rilevamenti.c_str(), rilevamenti_precedenti + 1);
      visualizzaYellowProblem("Verificata guida", "offroad !");
    }
    if ((nRilevamentiFuoriNazione / MAX_CAMPIONAMENTI_ALLARMI_GIALLI > 0.5) && Superata_Soglia_RangeGPS == 0) {
      // Problema: store + lcd + led
      String problema = "Fuori nazione";
      int rilevamenti_precedenti = getValueFromT5(problema.c_str(), rilevamenti.c_str());
      int theshold = getValueFromT5(problema.c_str(), soglia.c_str());
      if (rilevamenti_precedenti + 1 == theshold) {
        Serial.println("-------------------------------- Problema fuori nazione rilevato e memorizzato --------------------------------");
        updateColumnValue(problema.c_str(), da_Mandare.c_str(), 1);
        Superata_Soglia_RangeGPS = 1;
      }
      updateColumnValue(problema.c_str(), rilevamenti.c_str(), rilevamenti_precedenti + 1);
      visualizzaYellowProblem("Rilevata guida", "fuori nazione !");
    }
    if ((nRilevamentiRimorchi / MAX_CAMPIONAMENTI_ALLARMI_GIALLI > 0.5) && Superata_Soglia_Rimorchi == 0) {
      // Problema: store + lcd + led
      String problema = "Rimorchio/traino";
      int rilevamenti_precedenti = getValueFromT5(problema.c_str(), rilevamenti.c_str());
      int theshold = getValueFromT5(problema.c_str(), soglia.c_str());
      if (rilevamenti_precedenti + 1 == theshold) {
        Serial.println("-------------------------------- Problema fumo rilevato e memorizzato --------------------------------");
        updateColumnValue(problema.c_str(), da_Mandare.c_str(), 1);
        Superata_Soglia_Rimorchi = 1;
      }
      updateColumnValue(problema.c_str(), rilevamenti.c_str(), rilevamenti_precedenti + 1);
      visualizzaYellowProblem("Rilevato", "traino merce !");
    }
    campionamento_AllarmiYellow = 0;
    nRilevamentiFumo = 0;
    nRilevamentiFuoriStrada = 0;
    nRilevamentiFuoriNazione = 0;
    nRilevamentiRimorchi = 0;
  }
}

void obdComunicationCallback() {
  switch (obd_state) {
    case ENG_RPM:
      rpm = myELM327.rpm();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("RPM: ");
        Serial.println(rpm);
        obd_state = KPH;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case KPH:
      kph = myELM327.kph();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("KPH: ");
        Serial.println(kph);
        obd_state = THROTTLE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case THROTTLE:
      throttle = myELM327.throttle();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Throttle: ");
        Serial.println(throttle);
        obd_state = LOAD;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case LOAD:
      load = myELM327.engineLoad();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Engine Load: ");
        Serial.println(load);
        obd_state = COOLANT_TEMP;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case COOLANT_TEMP:
      coolant_temp = myELM327.engineCoolantTemp();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Coolant Temp: ");
        Serial.println(coolant_temp);
        obd_state = TORQUE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case TORQUE:
      torque = myELM327.torque();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Torque: ");
        Serial.println(torque);
        obd_state = BATTERY_VOLTAGE;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case BATTERY_VOLTAGE:
      battery_voltage = myELM327.batteryVoltage();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Battery Voltage: ");
        Serial.println(battery_voltage);
        obd_state = OIL_TEMP;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case OIL_TEMP:
      oil_temp = myELM327.oilTemp();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Oil Temp: ");
        Serial.println(oil_temp);
        obd_state = FUEL_LEVEL;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;

    case FUEL_LEVEL:
      fuel_level = myELM327.fuelLevel();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        Serial.print("Fuel Level: ");
        Serial.println(fuel_level);
        obd_state = ENG_RPM;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
      }
      break;
  }
}

bool isInsideNation(float lat, float lon) {
  return (lat >= LAT_MIN && lat <= LAT_MAX && lon >= LON_MIN && lon <= LON_MAX);
}

float getLongitudinalAcceleration() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  return a.acceleration.x;
}

// Funzione che determina se un veicolo sta trasportando un rimorchio
bool isTrailerAttached() {
  static float prev_accel = 0.0;
  float current_accel = getLongitudinalAcceleration();
  float accel_diff = fabs(current_accel - prev_accel);
  prev_accel = current_accel;

  // Calcolo del rapporto tra carico motore e velocità
  float load_per_kph = (kph > 0) ? (load / kph) : 0;

  // Verifica se l'accelerazione longitudinale e il carico del motore superano le soglie definite
  bool is_accel_smooth = accel_diff < ACCEL_THRESHOLD;
  bool is_load_high = load_per_kph > LOAD_THRESHOLD;

  // Se entrambe le condizioni sono soddisfatte, il veicolo probabilmente sta trasportando un rimorchio
  return is_accel_smooth && is_load_high;
}

void regularDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nessun problema");
  lcd.setCursor(0, 1);
  lcd.print("rilevato");
  digitalWrite(LED_ROSSO, HIGH);
  digitalWrite(LED_GIALLO, HIGH);
  digitalWrite(LED_VERDE, LOW);
}

void visualizzaYellowProblem(String firstLine, String secondLine) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
  digitalWrite(LED_GIALLO, LOW);
  digitalWrite(LED_VERDE, HIGH);
}

void visualizzaRedProblem(String firstLine, String secondLine) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
  digitalWrite(LED_ROSSO, LOW);
  digitalWrite(LED_VERDE, HIGH);
}

void cleanAllarmCallback() {
  Superata_Soglia_Fumo = 0;
  Superata_Soglia_FuoriStrada = 0;
  Superata_Soglia_RangeGPS = 0;
  Superata_Soglia_Rimorchi = 0;
  cleanAllarmiYellow(db);
  cleanAllarmiRed(db);
}