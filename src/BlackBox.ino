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

#include "LocalTime.h"
#include "Storage.h"
#include "FileSystem.h"
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
uint8_t remoteAddress[] = { 0x10, 0x21, 0x3E, 0x4C, 0x6F, 0x3A };

// Dati relativi al monitoraggio
Dato accelerometro_Sensor("Accelerometria", MAX_READINGS_ACCELEROMETRIA);
Dato alcool_Sensor("Alcool", MAX_READINGS_ALCOOL);
Dato airQuality_Sensor("Qualità aria", MAX_READINGS_AIRQUALITY);
Dato gps_Sensor("Coordinate GPS", MAX_READINGS_GPS);
Dato coolantTemp_Sensor("Liquido raffreddamento", MAX_READINGS_LIQ_RAFFR);
Dato oilTemp_Sensor("Olio motore temp", MAX_READINGS_OIL);
Dato torque_Sensor("Coppia motore", MAX_READINGS_TORQUE);
Dato batteryV_Sensor("Voltaggio batteria", MAX_READINGS_BATTERIA_V);
Dato fuelLevel_Sensor("Livello carburante", MAX_READINGS_CARBURANTE_LEVEL);

typedef enum {
  ENG_RPM,          // Giri al minuto del motore
  KPH,              // Velocità del veicolo in chilometri orari
  THROTTLE,         // Posizione della valvola a farfalla
  LOAD,             // Carico del motore in percentuale
  COOLANT_TEMP,     // Temperatura del liquido di raffreddamento
  TORQUE,           // Coppia del motore
  BATTERY_VOLTAGE,  // Voltaggio della batteria
  OIL_TEMP,         // Temperatura dell'olio motore
  FUEL_LEVEL        // Livello del carburante in percentuale
} obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

// ### Dati OBD ###
float rpm = 0;
float kph = 0;
float throttle = 0;
float load = 0;
float coolant_temp = 0;
float torque = 0;
float battery_voltage = 0;
float oil_temp = 0;
float fuel_level = 0;

// ### Profilazione ###
int campionamento_Profilazione = 0;
int nRilevamentiAccDecBrusche = 0;
int nRilevamentiSterzateBrusche = 0;
int nRilevamentiFatt1 = 0;
int nRilevamentiFatt2 = 0;
int nRilevamentiCaricoMotoreOutOfRange = 0;

// ### Allarmi critici ###
int campionamento_AllarmiRed = 0;
int nRilevamentiAlcool = 0;
int nRilevamentiGas = 0;
int nRilevamentiTempEngineCritic = 0;
int nRilevamentiTempOilCritic = 0;
int nRilevamentiCoppiaEngineCritic = 0;

// ### Allarmi meno critici ###
int campionamento_AllarmiYellow = 0;
int nRilevamentiFumo = 0;
int nRilevamentiFuoriStrada = 0;
int nRilevamentiFuoriNazione = 0;
int nRilevamentiRimorchi = 0;
bool Superata_Soglia_Fumo = 0;
bool Superata_Soglia_FuoriStrada = 0;
bool Superata_Soglia_RangeGPS = 0;
bool Superata_Soglia_Rimorchi = 0;

// ### Assistenza
struct Button {
	const uint8_t PIN;
	bool pressed;
};
Button button = {BUTTON_PIN, false};

void accelerometriaCallback();
void airMonitoringCallback();
void gspTrackerCallback();
void obdComunicationCallback();
void carParametersCallback();

void profilazioneCallback();
void segnalazioneAllarmiRedCallback();
void segnalazioneAllarmiYellowCallback();

void synchDataCallback();
void cleanDataRoutineCallback();
void cleanDataMemoryFullCallback();
void cleanAllarmCallback();
#ifdef DEBUG
void loggingCallback();
#endif
void GPSTrackingCallback();
void accidentAndAssistanceCallback();

Task accelerometriaTask(ACCELEROMETRIA_READ_INTERVAL, TASK_FOREVER, &accelerometriaCallback);
Task airMonitoringTask(AIR_READ_INTERVAL, TASK_FOREVER, &airMonitoringCallback);
Task gpsTrackerTask(GPS_READ_INTERVAL, TASK_FOREVER, &gspTrackerCallback);
Task obdTask(OBD_READ_INTERVAL, TASK_FOREVER, &obdComunicationCallback);
Task carParametersTask(CAR_OBD_STORAGE_INTERVAL, TASK_FOREVER, &carParametersCallback);

Task profilazioneTask(PROFILAZIONE_INTERVAL, TASK_FOREVER, &profilazioneCallback);
Task segnalazioneAllarmiRedTask(ALLARMI_INTERVAL, TASK_FOREVER, &segnalazioneAllarmiRedCallback);
Task segnalazioneAllarmiYellowTask(ALLARMI_INTERVAL, TASK_FOREVER, &segnalazioneAllarmiYellowCallback);

Task synchDataTask(SYNCHRONIZATION_DATA, TASK_FOREVER, &synchDataCallback);
Task cleanDataRoutineTask(CLEANING_ROUTINE, TASK_FOREVER, &cleanDataRoutineCallback);
Task cleanDataMemoryFullTask(CLEANING_MEMORY_FULL, TASK_FOREVER, &cleanDataMemoryFullCallback);
Task cleanAllarmTask(WEEK, TASK_FOREVER, &cleanAllarmCallback);

#ifdef DEBUG
Task loggingTask(CENTRALINA_LOG, TASK_FOREVER, &loggingCallback);
#endif

Task GPSTrackingTask(GPS_TRACKING_SECURITY, TASK_FOREVER, &GPSTrackingCallback);
Task accidentAndAssistanceTask(ACCIDENT_AND_ASSISTANCE, TASK_FOREVER, &accidentAndAssistanceCallback);

void IRAM_ATTR isr() {
	button.pressed = true;
}

void setup() {

  Serial.begin(115200);

  Wire.begin();
  if (!rtc.begin()) {
    #ifdef DEBUG
    Serial.println("Couldn't find RTC");
    #endif
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

  pinMode(button.PIN, INPUT_PULLUP);
	attachInterrupt(button.PIN, isr, RISING);

  if (!mpu.begin(0x69)) {
    #ifdef DEBUG
    Serial.println("MPU6050 non trovato!");
    #endif
  }
  #ifdef DEBUG
  Serial.println("MPU6050 trovato e inizializzato correttamente.");
  #endif

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

  obdConfig();

  scheduler.init();

  scheduler.addTask(accelerometriaTask);
  scheduler.addTask(airMonitoringTask);
  scheduler.addTask(gpsTrackerTask);
  scheduler.addTask(obdTask);
  scheduler.addTask(carParametersTask);

  scheduler.addTask(profilazioneTask);
  scheduler.addTask(segnalazioneAllarmiRedTask);
  scheduler.addTask(segnalazioneAllarmiYellowTask);

  scheduler.addTask(synchDataTask);
  scheduler.addTask(cleanDataRoutineTask);
  scheduler.addTask(cleanDataMemoryFullTask);
  scheduler.addTask(cleanAllarmTask);
  #ifdef DEBUG
  scheduler.addTask(loggingTask);
  #endif
  scheduler.addTask(GPSTrackingTask);
  scheduler.addTask(accidentAndAssistanceTask);

  accelerometriaTask.enable();
  airMonitoringTask.enable();
  gpsTrackerTask.enable();
  obdTask.enable();
  carParametersTask.enable();

  profilazioneTask.enable(); 
  // segnalazioneAllarmiRedTask.enable();
  segnalazioneAllarmiYellowTask.enable();

  synchDataTask.enable();
  // cleanDataRoutineTask.enable();
  // cleanDataMemoryFullTask.enable();
  // cleanAllarmTask.enable();
  #ifdef DEBUG
  loggingTask.enable();
  #endif
  // GPSTrackingTask.enable();
  // accidentAndAssistanceTask.enable();
}

void loop() {
  scheduler.execute();
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

  long long int result = processSensorData(accelerometro_Sensor.lastStoredTS, accelerometro_Sensor.name, res.c_str(), "String", "m/s^2", 6);

  if (result != -1) {
    accelerometro_Sensor.lastStoredTS = result;
  }
}

// AIR MONITORING TASK
void airMonitoringCallback() {
  // Lettura sensore MQ-3
  int mq3AnalogValue = analogRead(ALCOOL_PIN);

  // Lettura sensore MQ-2
  int mq2AnalogValue = analogRead(AIR_QUALITY_PIN);

  /* DA INSERIRE NELLA VERSIONE UFFICIALE
  alcool_Sensor.addReading(mq3AnalogValue);
  airQuality_Sensor.addReading(mq2AnalogValue);
  // Fine delle mie x letture e cambio del tempo di storage
  if(alcool_Sensor.currentIndex == alcool_Sensor.maxReadings) {
    changeTimeStorage(alcool_Sensor);
  }
  if(airQuality_Sensor.currentIndex == airQuality_Sensor.maxReadings) {
    changeTimeStorage(airQuality_Sensor);
  } 
  */

  long long int resultAlcool = processSensorData(alcool_Sensor.lastStoredTS, alcool_Sensor.name, String(mq3AnalogValue).c_str(), "Integer", "Assente", 2);
  long long int resultAirQuality = processSensorData(airQuality_Sensor.lastStoredTS, airQuality_Sensor.name, String(mq2AnalogValue).c_str(), "Integer", "Assente", 2);

  if (resultAlcool != -1) {
    alcool_Sensor.lastStoredTS = resultAlcool;
  }
  if (resultAirQuality != -1) {
    airQuality_Sensor.lastStoredTS = resultAirQuality;
  }
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
    #ifdef DEBUG
    Serial.println("INVALID LOCATION");
    #endif
  }

  String res = String(lat, 6) + " / " + String(lon, 6);

  long long int result = processSensorData(gps_Sensor.lastStoredTS, gps_Sensor.name, res.c_str(), "String", "°", 5);

  if (result != -1) {
    gps_Sensor.lastStoredTS = result;
  }
}

// PARAMETRI PROVENIENTI DAL VEICOLO TASK
void carParametersCallback() {
  String str1 = String(coolant_temp, 2);
  long long int res1 = processSensorData(coolantTemp_Sensor.lastStoredTS, coolantTemp_Sensor.name, str1.c_str(), "Integer", "°C", 1);
  String str2 = String(oil_temp, 2);
  long long int res2 = processSensorData(oilTemp_Sensor.lastStoredTS, oilTemp_Sensor.name, str2.c_str(), "Integer", "°C", 1);
  String str3 = String(torque, 2);
  long long int res3 = processSensorData(torque_Sensor.lastStoredTS, torque_Sensor.name, str3.c_str(), "Integer", "Newton metri (Nm)", 2);
  String str4 = String(battery_voltage, 2);
  long long int res4 = processSensorData(batteryV_Sensor.lastStoredTS, batteryV_Sensor.name, str4.c_str(), "Float", "Volt (V)", 3);
  String str5 = String(fuel_level, 2);
  long long int res5 = processSensorData(fuelLevel_Sensor.lastStoredTS, fuelLevel_Sensor.name, str5.c_str(), "Float", "%", 4);
  if (res1 != -1) {
    coolantTemp_Sensor.lastStoredTS = res1;
  }
  if (res2 != -1) {
    oilTemp_Sensor.lastStoredTS = res2;
  }
  if (res3 != -1) {
    torque_Sensor.lastStoredTS = res3;
  }
  if (res4 != -1) {
    batteryV_Sensor.lastStoredTS = res4;
  }
  if (res5 != -1) {
    fuelLevel_Sensor.lastStoredTS = res5;
  }
  /* DA INSERIRE NELLA VERSIONE UFFICIALE
  coolantTemp_Sensor.addReading(coolant_temp);
  oilTemp_Sensor.addReading(oil_temp);
  // Fine delle mie x letture e cambio del tempo di storage
  if(coolantTemp_Sensor.currentIndex == coolantTemp_Sensor.maxReadings) {
    changeTimeStorage(coolantTemp_Sensor);
  }
  if(oilTemp_Sensor.currentIndex == oilTemp_Sensor.maxReadings) {
    changeTimeStorage(oilTemp_Sensor);
  }
  */
}

// PROFILAZIONE TASK
void profilazioneCallback() {
  campionamento_Profilazione++;
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x = a.acceleration.x;
  float y = a.acceleration.y;

  // Accelerazioni / decelerazioni brusche
  if ((x / FORZA_G) > (ACCELERAZIONE_BRUSCA) || (x / FORZA_G) < (DECELERAZIONE_BRUSCA)) {
    nRilevamentiAccDecBrusche++;
  }
  // Sterzate sinistra e destra brusche
  if ((y / FORZA_G) > (STERZATA_DX_BRUSCA) || (y / FORZA_G) < (STERZATA_SX_BRUSCA)) {
    nRilevamentiSterzateBrusche++;
  }
  // Fattore 1
  float fatt1 = ((kph / MAX_SPEED_CAR) / (rpm / MAX_RPM_CAR));
  if ((fatt1 < RANGE_INF) || (fatt1 > RANGE_SUP)) {
    nRilevamentiFatt1++;
  }
  // Fattore 2
  float fatt2 = ((throttle / 100) / (rpm / MAX_RPM_CAR));
  if ((fatt2 < RANGE_INF) || (fatt2 > RANGE_SUP)) {
    nRilevamentiFatt2++;
  }
  // Carico motore
  if ((load < LOAD_MIN) || (load > LOAD_MAX)) {
    nRilevamentiCaricoMotoreOutOfRange++;
  }
  if (campionamento_Profilazione == MAX_CAMPIONAMENTI_PROFILAZIONE) {  // (3 fa riferimento al numero di campionamenti che si desidera) && è trascorso un giorno dall'ultima storage (if presente all'interno della funzione updateCampionamentoProfilazione())
    updateCampionamentoProfilazione();
  }
}

// SEGNALAZIONE ALLARMI ROSSI TASK
void segnalazioneAllarmiRedCallback() {
  campionamento_AllarmiRed++;
  // Alcool
  int alcool = analogRead(ALCOOL_PIN);
  if (alcool > SOGLIA_ALCOOL) {
    nRilevamentiAlcool++;
  }
  // Gas abitacolo
  int gas = analogRead(AIR_QUALITY_PIN);
  if (gas < 0) {  // SOGLIA_QUAL_ARIA
    nRilevamentiGas++;
  }
  // Temperatura liquido di raffreddamento
  if (coolant_temp > MAX_COOLANT_TEMP) {
    nRilevamentiTempEngineCritic++;
  }
  // Temperatura olio motore
  if (oil_temp > MAX_OIL_TEMP) {
    nRilevamentiTempOilCritic++;
  }
  // Coppia motore
  if ((kph != 0) && (torque < MIN_TORQUE_THRESHOLD || torque > MAX_TORQUE_THRESHOLD)) {
    nRilevamentiCoppiaEngineCritic++;
  }
  // Consumo anomalo di carburante ?
  // Distanza di frenata anomala ?
  if (campionamento_AllarmiRed == MAX_CAMPIONAMENTI_ALLARMI_ROSSI) {  // bisogna mettere un valore molto alto dato che alcuni valori potrebbero essere errati per vari motivi + si vorrebbe essere il più precisi possibili
    updateRedAllarm();
  }
}

// SEGNALAZIONE ALLARMI GIALLI TASK
void segnalazioneAllarmiYellowCallback() {
  campionamento_AllarmiYellow++;
  // Fumo
  int airQuality = analogRead(AIR_QUALITY_PIN);
  if (airQuality > 0) {  // Andrà impostata una soglia per il fumo
    nRilevamentiFumo++;
  }
  /*
  // Fuori strada
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float z = a.acceleration.z;
  if (z < (FORZA_G - TOLLERANZA_OFF_ROAD) || z > (FORZA_G + TOLLERANZA_OFF_ROAD)) {
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
  if (isTrailerAttached(rpm, kph, throttle, load, torque)) {
    nRilevamentiRimorchi++;
  }
  */
  if (campionamento_AllarmiYellow == MAX_CAMPIONAMENTI_ALLARMI_GIALLI) {  // bisogna mettere un valore molto alto dato che alcuni valori potrebbero essere errati per vari motivi + si vorrebbe essere il più precisi possibili
    updateYellowAllarm();
  }
}

// SINCRONIZZAZIONE TASK
void synchDataCallback() {
  obdTask.disable();
  if (WiFi.status() == WL_CONNECTED) {  // ed è passato abbastanza tempo >= 24h dall'ultima sincronizzazione
    #ifdef DEBUG
    Serial.println("Connessione WiFi attiva");
    #endif
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      #ifdef DEBUG
      Serial.println("Connecting to WiFi...");
      #endif
    }
    #ifdef DEBUG
    Serial.println("WiFi connected");
    Serial.println("---------------------------- Sincronizzazione ----------------------------");
    #endif
    syncMonitoraggioData(db);
    syncProfilazioneData(db);
    syncAllarmiRossiData(db);
    syncAllarmiGialliData(db);
  } else {
    #ifdef DEBUG
    Serial.println("Nessuna connessione WiFi");
    #endif
  }
  restoreBluetoothConnection();
  obdTask.enable();
}

// CLEAN ROUTINE TASK
void cleanDataRoutineCallback() {
  #ifdef DEBUG
  Serial.println("---------------------------- Pulisco routine ----------------------------");
  #endif
  garbageCollectorRoutine();
}

// CLEAN MEMORY FULL TASK
void cleanDataMemoryFullCallback() {
  /*
  if (raggiungo un 90 % di capienza adotto la mia politica) { -> non riesco a tener traccia di quanto il filesystem sia pieno !!!
    #ifdef DEBUG
    Serial.println("---------------------------- Pulisco memory ----------------------------");
    #endif
    garbageCollectorMemoryFull();
  } */
}

// CLEAN ALLARMI TASK
void cleanAllarmCallback() {
  Superata_Soglia_Fumo = false;
  Superata_Soglia_FuoriStrada = false;
  Superata_Soglia_RangeGPS = false;
  Superata_Soglia_Rimorchi = false;
  cleanAllarmiYellow(db);
  cleanAllarmiRed(db);
}

// SIMULAZIONE 
/*
void obdComunicationCallback() {
  rpm = random(750, MAX_RPM_CAR);
  kph = random(0, MAX_SPEED_CAR);
  throttle = random(10, 101);
  load = random(10, 101);
  coolant_temp = random(20, MAX_COOLANT_TEMP + 1);
  torque = random(MIN_TORQUE_THRESHOLD, MAX_TORQUE_THRESHOLD);
  battery_voltage = random(110, 130) / 10;
  oil_temp = random(25, MAX_OIL_TEMP + 1);
  fuel_level = random(0, 101);
} */
// COMUNICAZIONE OBD TASK
void obdComunicationCallback() {
    switch (obd_state)
    {
        case ENG_RPM:
        {
            float rpm_r = myELM327.rpm();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                rpm = rpm_r;
                obd_state = KPH;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = KPH;
            }
            break;
        }

        case KPH:
        {
            float kph_r = myELM327.kph();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {   
                kph = kph_r;
                obd_state = THROTTLE;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = THROTTLE;
            }
            break;
        }

        case THROTTLE:
        {
            float throttle_r = myELM327.throttle();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            { 
                throttle = throttle_r;
                obd_state = LOAD;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = LOAD;
            }
            break;
        }

        case LOAD:
        {
            float load_r = myELM327.engineLoad();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                load = load_r;
                obd_state = COOLANT_TEMP;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = COOLANT_TEMP;
            }
            break;
        }

        case COOLANT_TEMP:
        {
            float coolant_temp_r = myELM327.engineCoolantTemp();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                coolant_temp = coolant_temp_r;
                obd_state = TORQUE;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = TORQUE;
            }
            break;
        }

        case TORQUE:
        {
            float torque_r = myELM327.torque();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                torque = torque_r;
                obd_state = BATTERY_VOLTAGE;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = BATTERY_VOLTAGE;
            }
            break;
        }

        case BATTERY_VOLTAGE:
        {
            float battery_voltage_r = myELM327.batteryVoltage();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                battery_voltage = battery_voltage_r;
                obd_state = OIL_TEMP;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = OIL_TEMP;
            }
            break;
        }

        case OIL_TEMP:
        {
            float oil_temp_r = myELM327.oilTemp();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                oil_temp = oil_temp_r;
                obd_state = FUEL_LEVEL;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = FUEL_LEVEL;
            }
            break;
        }

        case FUEL_LEVEL:
        {
            float fuel_level_r = myELM327.fuelLevel();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {   
                fuel_level = fuel_level_r;
                obd_state = ENG_RPM;  // Riparti dal primo stato
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = ENG_RPM;
            }
            break;
        }
    }
} 

// LOGGING DATI CENTRALINA TASK
void loggingCallback() {
  Serial.print("Rpm (giri/min): ");
  Serial.print(rpm);
  Serial.print(" | ");
  Serial.print("Kph (km/h): ");
  Serial.print(kph);
  Serial.print(" | ");
  Serial.print("Throttle position (%): ");
  Serial.print(throttle);
  Serial.print(" | ");
  Serial.print("Engine load (%): ");
  Serial.print(load);
  Serial.print(" | ");
  Serial.print("Coolant temp (°C): ");
  Serial.print(coolant_temp);
  Serial.print(" | ");
  Serial.print("Torque (Nm): ");
  Serial.print(torque);
  Serial.print(" | ");
  Serial.print("Battery voltage (V): ");
  Serial.print(battery_voltage, 2);
  Serial.print(" | ");
  Serial.print("Oil temp (°C): ");
  Serial.print(oil_temp);
  Serial.print(" | ");
  Serial.print("Fuel level (%): ");
  Serial.println(fuel_level);
}

// GPS SECURITY TRACKING TASK
void GPSTrackingCallback() {
  double lat = -1;
  double lon = -1;
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
  } else {
    #ifdef DEBUG
    Serial.println("INVALID LOCATION");
    #endif
  }
  String res = "Posizione GPS: " + String(lat, 6) + " / " + String(lon, 6);
  // Invia posizione GPS alla centrale via GSM con le seguenti informazione... Posizione GPS: lat, long - data
}

// INCIDENTE E ASSISTENZA TASK
void accidentAndAssistanceCallback() {
  // RILEVAMENTO INCIDENTE
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float x, y, z;
  x = a.acceleration.x;
  y = a.acceleration.y;
  z = a.acceleration.z;
  float forzaG = calcolaForzaG(x, y, z) / FORZA_G;
  // Deve essere studiato il caso di rilevamento incidente con maggior accuratezza e ulteriori filtri
  if (forzaG >= SOGLIA_RILEVAMENTO_INCIDENTE) {
    // -> Invio rilevamento "incidente" via GSM alla centrale operativa con le seguenti informazioni... Incidente rilevato: data - accelerazione asse x - kph
  }
  // RILEVAMENTO RICHIESTA DI ASSISTENZA
  if(button.pressed) {
    // -> Invio rilevamento "richiesta di assistenza" alla centrale operativa col le seguenti informazioni... Assistenza cliente x: contattare il cliente x per richiesta di assistenza
    #ifdef DEBUG
    Serial.println(" ------------------------ ASSISTENZA ------------------------ ");
    #endif
    button.pressed = false;
  }
}

/*
  Configurazione rtc e apertura file system
*/
void configurationProcess() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
    Serial.println("...");
    #endif
  }
  #ifdef DEBUG
  Serial.print("WiFi connected\n");
  #endif

  // Viene assegnato all'RTC un orario di "nascita" (configurazione che va fatta con una connessione internet per contattare i server NTP)
  setTimeNation();
  // Montaggio filesystem
  if (mountingFS(FILE_SYSTEM)) {
    #ifdef DEBUG
    Serial.println("Little FS Mounted Successfully");
    #endif
  } else {
    #ifdef DEBUG
    Serial.println("Failed to mount file system");
    #endif
  }
  // Apertura filesystem + cancellazione precedenti dati memorizzati in "/dati.db"
  openFS();

  initializeStorage(PATH_STORAGE_DATA_DB);
}

/*
  Configurazione connessione bluetooth esp32-lettore obd scanner
*/
void obdConfig() {
  SerialBT.begin("ESP32_MASTER", true);
  SerialBT.setPin("1234");

  if (!SerialBT.connect(remoteAddress)) {
    #ifdef DEBUG
    Serial.println("Couldn't connect to OBD scanner - Phase 1");
    #endif
  }
  if (!myELM327.begin(SerialBT, DEBUG_OBD, RESPONSE_TIME)) {
    #ifdef DEBUG
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    #endif
  }
  #ifdef DEBUG
  Serial.println("Connected to ELM327");
  #endif
}

/*
  Restituisce true se è trascorso abbastanza tempo di storage dall'ultima memorizzazione, false altrimenti
*/
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

  sqlite3_bind_text(stmt, 1, nameSensor, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, lastStoredTS);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    // Il record è stato trovato, ora puoi recuperare i valori dei campi e visualizzarli
    long long int timestamp = sqlite3_column_int64(stmt, 4);  // Modifica qui se il timestamp è un intero a 64 bit
    #ifdef DEBUG
    Serial.print("Last registered record timestamp: ");
    char prova[20];
    secondsToString(timestamp, prova);
    Serial.println(prova);
    #endif
  } else {
    #ifdef DEBUG
    Serial.println("Nessun record trovato per il sensore specificato e il timestamp specificato");
    #endif
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  // Verifico che sia passato abbastanza tempo dall'ultima storage del sensore coinvolto
  long long int tempoStorage = getTempoStorageFromT2(db, nameSensor);

  if ((timestampNow - lastStoredTS) >= tempoStorage) {
    return true;
  } else {
    return false;
  }
}

/*
  Restituisce l'ultimo tempo di memorizzazione di "nome_sensore", -1 altrimenti; il dato in questione viene memorizzato all'interno della
  tabella di monitoraggio t1 se è passato abbastanza tempo dall'ultima memorizzazione di un suo valore, altrimenti non viene memorizzato
*/
long long int processSensorData(long long int lastStoredTS, char* nome_sensore, const char* valore, char* tipo, char* misura, int priority) {
  long long int res = -1;
  DateTime dt = rtc.now();

  long long int timestampNow = dateTimeToEpoch(dt);
  char b1[DATE_TIME_LEN];
  char b2[DATE_TIME_LEN];
  #ifdef DEBUG
  secondsToString(timestampNow, b1);
  secondsToString(lastStoredTS, b2);
  Serial.print(dateTimeToString(epochToDateTime(timestampNow)));
  Serial.print(" - ");
  Serial.println(dateTimeToString(epochToDateTime(lastStoredTS)));
  #endif
  bool passed = spentEnoughTimeFromLastStrorage(nome_sensore, timestampNow, lastStoredTS);

  // Se è passato il tempo di storage, esegui le azioni desiderate
  if (passed) {
    createRecordToInsertIntoT1(nome_sensore, valore, tipo, misura, timestampNow, NO_SYNCHRONIZED, priority);
    res = timestampNow;
  } else {
    #ifdef DEBUG
    Serial.println("Non è trascorso abbastanza tempo dall'ultima storage del dato");
    #endif
  }
  return res;
}

/*
  Cambia il tempo di storage di monitoraggio di un particolare dato
*/
void changeTimeStorage(Dato& dato) {
  // Attuo la mia politica di storage intelligente che cambia in base alla natura dei dati i tempi di storage
  // Calcola la deviazione standard delle letture
  float stdDev = dato.calculateStdDev();
  // Ottieni i parametri dal database
  int tempoStorage, fattoreIncrDecr, tMinStorage, tMaxStorage;
  float soglia;
  if (!getSensorParametersFromT2(dato.name, tempoStorage, fattoreIncrDecr, soglia, tMinStorage, tMaxStorage)) {
    #ifdef DEBUG
    Serial.println("Failed to get sensor parameters.");
    #endif
    return;
  }
  #ifdef DEBUG
  Serial.println(tempoStorage);
  Serial.println(fattoreIncrDecr);
  Serial.println(soglia);
  Serial.println(tMinStorage);
  #endif
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
  updateTempoStorageIntoT2(dato.name, tempoStorage);
  dato.currentIndex = 0;
}

/*
  Restituisce true se il veicolo è all'interno di determinate coordinate data la sua posizione attuale
*/
bool isInsideNation(float lat, float lon) {
  return (lat >= LAT_MIN && lat <= LAT_MAX && lon >= LON_MIN && lon <= LON_MAX);
}

/*
  Funzione che determina se un veicolo sta trasportando un rimorchio
*/
bool isTrailerAttached(float currentRPM, float currentSpeed, float throttlePosition, float engineLoad, float torque) {
  // Se almeno quattro parametri sono sopra le soglie, considero il traino
  int countIndicators = 0;
  if (currentRPM > RPM_THRESHOLD) countIndicators++;
  if (currentSpeed < SPEED_THRESHOLD) countIndicators++;
  if (engineLoad > LOAD_THRESHOLD) countIndicators++;
  if (torque > TORQUE_THRESHOLD) countIndicators++;
  if (throttlePosition > THROTTLE_THRESHOLD) countIndicators++;

  return countIndicators >= 4;
}

void regularDisplay() {
  lcdDisplay(REGULAR_DISPLAY_FIRST_LINE, REGULAR_DISPLAY_SECOND_LINE);
  digitalWrite(LED_ROSSO, HIGH);
  digitalWrite(LED_GIALLO, HIGH);
  digitalWrite(LED_VERDE, LOW);
}

void visualizzaYellowProblem(String firstLine, String secondLine) {
  lcdDisplay(firstLine, secondLine);
  digitalWrite(LED_GIALLO, LOW);
  digitalWrite(LED_VERDE, HIGH);
}

void visualizzaRedProblem(String firstLine, String secondLine) {
  lcdDisplay(firstLine, secondLine);
  digitalWrite(LED_ROSSO, LOW);
  digitalWrite(LED_VERDE, HIGH);
}

void lcdDisplay(String firstLine, String secondLine) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
}

/*
  Calcola la risultante della forza G esercitata all'interno del veicolo
*/
float calcolaForzaG(float gx, float gy, float gz) {
  return sqrt(gx * gx + gy * gy + gz * gz);
}

/*
  Aggiorna i dati relativi al campionamento della profilazione
*/
void updateCampionamentoProfilazione() {
  DateTime now = rtc.now();
  long long int timestamp = dateTimeToEpoch(now);
  long long int last = getLastTimestampFromT3(db);
  if (spentEnoughTimeFromLastProfilazione(timestamp, last)) {
    storeProfilazioneVote(now, timestamp);
  }
}

bool spentEnoughTimeFromLastProfilazione(long long int timestamp, long long int last) {
  return timestamp - last >= 0;  // al posto dello 0 andranno inseriti i secondi presenti all'interno di una giornata (86400) dato che vorrei profilare al più una volta al giorno
}

void storeProfilazioneVote(DateTime now, long long int timestamp) {
  #ifdef DEBUG
  Serial.println("--------------------------------------- PROFILAZIONE ---------------------------------------");
  #endif
  float voto = float((nRilevamentiAccDecBrusche) + (nRilevamentiSterzateBrusche) + (nRilevamentiFatt1) + (nRilevamentiFatt2) + (nRilevamentiCaricoMotoreOutOfRange)) / (N_CLASSIFICATORI * MAX_CAMPIONAMENTI_PROFILAZIONE);
  char votoStr[10];
  snprintf(votoStr, sizeof(votoStr), "%.2f", voto);
  char dateTimeStr[20];
  sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  createRecordToInsertIntoT3(dateTimeStr, votoStr, timestamp, NO_SYNCHRONIZED);
  campionamento_Profilazione = 0;
  nRilevamentiAccDecBrusche = 0;
  nRilevamentiSterzateBrusche = 0;
  nRilevamentiFatt1 = 0;
  nRilevamentiFatt2 = 0;
  nRilevamentiCaricoMotoreOutOfRange = 0;
  // Se sono presenti più di x voti già sincronizzati pulisco la tabella
  // eliminazioneVotiProfilazione(db);
}

/*
  Aggiorna i dati relativi al campionamento degli allarmi rossi
*/
void updateRedAllarm() {
  regularDisplay();
  Serial.println("-------------------------------- ALLARMI ROSSI --------------------------------");
  DateTime now = rtc.now();
  char dateTimeStr[20];
  sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  Serial.println("-------------------------------- ALLARMI ROSSI --------------------------------");
  // se noto che più della metà delle rilevazioni sono favorevoli alla presenza di un problema significa che è presente nell'aria
  storeRedProblem(nRilevamentiAlcool, "Alcool", dateTimeStr, "Rilevato alcool", "nell'abitacolo");
  storeRedProblem(nRilevamentiGas, "Gas", dateTimeStr, "Rilevato gas", "nell'abitacolo");
  storeRedProblem(nRilevamentiTempEngineCritic, "Liquido raffreddamento", dateTimeStr, "Temperatura", "raffreddamento alta");
  storeRedProblem(nRilevamentiTempOilCritic, "Olio motore", dateTimeStr, "Temperatura olio", "motore alta");
  storeRedProblem(nRilevamentiCoppiaEngineCritic, "Coppia motore", dateTimeStr, "Malfunzionamento", "coppia motore");
  campionamento_AllarmiRed = 0;
  nRilevamentiAlcool = 0;
  nRilevamentiGas = 0;
  nRilevamentiTempEngineCritic = 0;
  nRilevamentiTempOilCritic = 0;
  nRilevamentiCoppiaEngineCritic = 0;
}

void storeRedProblem(int nRilevamentiProblema, String nomeProblema, String dateTime, String firstLine, String secondLine) {
  if ((nRilevamentiProblema / (float)MAX_CAMPIONAMENTI_ALLARMI_ROSSI > SOGLIA_ALLARMI_ROSSI) && alreadyExistsIntoT4(db, nomeProblema.c_str()) == false) {
    createRecordToInsertIntoT4(nomeProblema.c_str(), dateTime.c_str(), NO_SYNCHRONIZED);
    visualizzaRedProblem(firstLine, secondLine);
  }
}

/*
  Aggiorna i dati relativi al campionamento degli allarmi gialli
*/
void updateYellowAllarm() {
  regularDisplay();
  #ifdef DEBUG
  Serial.println("-------------------------------- ALLARMI GIALLI --------------------------------");
  #endif
  // se noto che più della metà delle rilevazioni sono favorevoli alla presenza di un problema significa che è presente
  storeYellowProblem(nRilevamentiFumo, Superata_Soglia_Fumo, "Fumo", "Rilevato fumo", "nell'abitacolo");
  storeYellowProblem(nRilevamentiFuoriStrada, Superata_Soglia_FuoriStrada, "Fuori strada", "Verificata guida", "offroad");
  storeYellowProblem(nRilevamentiFuoriNazione, Superata_Soglia_RangeGPS, "Fuori nazione", "Rilevata guida", "fuori nazione");
  storeYellowProblem(nRilevamentiRimorchi, Superata_Soglia_Rimorchi, "Rimorchio/traino", "Rilevato", "traino merce");
  campionamento_AllarmiYellow = 0;
  nRilevamentiFumo = 0;
  nRilevamentiFuoriStrada = 0;
  nRilevamentiFuoriNazione = 0;
  nRilevamentiRimorchi = 0;
}

void storeYellowProblem(int nRilevamenti, bool &superata_soglia_rilevamenti, String problema, String firstLine, String secondLine) {
  if ((nRilevamenti / (float)MAX_CAMPIONAMENTI_ALLARMI_GIALLI > SOGLIA_ALLARMI_GIALLI) && superata_soglia_rilevamenti == false) {
    int rilevamenti_precedenti = getValueFromT5(problema.c_str(), "rilevamenti");
    int theshold = getValueFromT5(problema.c_str(), "soglia");
    if (rilevamenti_precedenti + 1 == theshold) {
      #ifdef DEBUG
      Serial.println("-------------------------------- Problema fumo rilevato e memorizzato --------------------------------");
      #endif
      updateColumnValueIntoT5(problema.c_str(), "daMandare", 1);
      superata_soglia_rilevamenti = true;
    }
    updateColumnValueIntoT5(problema.c_str(), "rilevamenti", rilevamenti_precedenti + 1);
    visualizzaYellowProblem(firstLine, secondLine);
  }
}

void restoreBluetoothConnection() {
    #ifdef DEBUG
    Serial.print("Bluetooth connection before: ");
    Serial.println(SerialBT.connected());
    #endif
    SerialBT.connect(remoteAddress);
    #ifdef DEBUG
    Serial.print("Bluetooth connection after: ");
    Serial.println(SerialBT.connected());
    #endif
}