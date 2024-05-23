#include <Wire.h>               // Libreria per la comunicazione I2C
#include <LiquidCrystal_I2C.h>  // Libreria per display LCD I2C su ESP8266/ESP32
#include <Adafruit_MPU6050.h>   // Libreria per l'accelerometro MPU6050
#include <Adafruit_Sensor.h>
#include <RTClib.h>

RTC_DS1307 rtc;

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Inizializza il display LCD con l'indirizzo I2C e le dimensioni corrette

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // Inizializza la comunicazione I2C

  // Inizializza l'accelerometro
  if (!mpu.begin(0x69)) {
    Serial.println("MPU6050 non trovato!");
    while (1)
      ;
  }
  Serial.println("MPU6050 trovato e inizializzato correttamente.");

  // Inizializza il display LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Accelerazione:");

  // Inizializza modulo rtc
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // January 21, 2021 at 3am you would call;
  rtc.adjust(DateTime(2021, 1, 21, 3, 0, 0));

  delay(2000);  // Attendi 2 secondi
}

void loop() {
  // Leggi i dati dell'accelerometro
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Cancella l'intero display LCD prima di stampare i nuovi dati
  lcd.clear();
  // Mostra i dati sull'accelerazione sul display LCD
  lcd.setCursor(0, 0);
  lcd.print("X:");
  lcd.print(a.acceleration.x);
  Serial.println(a.acceleration.x);
  lcd.setCursor(10, 0);
  lcd.print("Y:");
  lcd.print(a.acceleration.y);
  Serial.println(a.acceleration.y);
  lcd.setCursor(0, 1);
  lcd.print("Z:");
  lcd.print(a.acceleration.z);
  Serial.println(a.acceleration.z);
  Serial.println("###############################");
  delay(10000);

  DateTime now = rtc.now();

  // Creazione delle stringhe per data e ora
  char dateStr[11];  // "YYYY/MM/DD" + '\0' = 11 caratteri
  char timeStr[9];   // "HH:MM:SS" + '\0' = 9 caratteri

  sprintf(dateStr, "%04d/%02d/%02d", now.year(), now.month(), now.day());
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Stampa delle stringhe
  Serial.print("DATA: ");
  Serial.println(dateStr);

  Serial.print("ORA: ");
  Serial.println(timeStr);

  lcd.setCursor(0, 0);
  lcd.print("DATE: ");
  lcd.setCursor(6, 0);
  lcd.print(dateStr);

  lcd.setCursor(0, 1);
  lcd.print("TIME: ");
  lcd.setCursor(6, 1);
  lcd.print(timeStr);

  delay(10000);
}
