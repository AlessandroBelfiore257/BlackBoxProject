#include "BluetoothSerial.h"
#include "ELMduino.h"
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

BluetoothSerial SerialBT;
ELM327 myELM327;
TinyGPSPlus gps;
HardwareSerial hs(2);  // Utilizzo Serial2 per GPS
static const uint32_t GPSBaud = 38400;

TaskHandle_t TaskScheduling;
TaskHandle_t TaskOBDII;

typedef enum { ENG_RPM,
               SPEED,
               TEMPERATURE,
               VOLTAGE,
               ELOAD } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float RPM = 0;
float KPH = 0;
float TEMP = 0;
float VOLT = 0;
float E_LOAD = 0;

uint8_t remoteAddres[] = { 0x1C, 0xA1, 0x35, 0x69, 0x8D, 0xC5 };

void TaskSchedulingCode(void* pvParameters) {
  for (;;) {
    /*
    Serial.println("Eseguo scheduling...");
    double lat = -1;
    double lon = -1;
    while (hs.available() > 0) {
      gps.encode(hs.read());
    }
    if (gps.location.isValid()) {
      lat = gps.location.lat();
      lon = gps.location.lng();
    } else {
      Serial.println("Localizzazione GPS non valida");
    }
    String coordinates = String(lat, 6) + ", " + String(lon, 6);
    Serial.print("Coordinate GPS: ");
    Serial.println(coordinates); */
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
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void TaskOBDIICode(void* pvParameters) {
  for (;;) {
    centralinaMonitoringCallBack();
    //vTaskDelay(500 / portTICK_PERIOD_MS);  
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT", true);  // Master
  Serial.println("Attempting to connect to ELM327...");

  if (!SerialBT.connect(remoteAddres)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 1");
    while (1)
      ;
  }

  if (!myELM327.begin(SerialBT, true, 2000)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1)
      ;
  }

  Serial.println("Connected to ELM327");

  hs.begin(GPSBaud);

  xTaskCreatePinnedToCore(
    TaskSchedulingCode,
    "TaskScheduling",
    10000,
    NULL,
    2,
    &TaskScheduling,
    0);
  delay(500);

  xTaskCreatePinnedToCore(
    TaskOBDIICode,
    "TaskOBDII",
    10000,
    NULL,
    2,
    &TaskOBDII,
    1);
  delay(500);
}

void loop() {
  // Il loop è vuoto poiché i task gestiscono il lavoro
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
          obd_state = ELOAD;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = ELOAD;
        }
        break;
      }

    case ELOAD:
      {
        float engine_load = myELM327.engineLoad();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          Serial.print("engine_load: ");
          Serial.println(engine_load);
          E_LOAD = engine_load;
          obd_state = ENG_RPM;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          obd_state = ENG_RPM;
        }
        break;
      }
  }
}