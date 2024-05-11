#include <stdio.h>
#include <stdlib.h>
#include <SPI.h>
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <TaskScheduler.h>
#include <WiFi.h>

#include "LCtime.h"
#include "Storage.h"

#include "Secrets.h"
#include "Config.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const uint16_t port = PORT;
const char *host = HOST;
const int BUTTON_PIN = BUTTON;

Scheduler scheduler;
WiFiClient client;
int mydata;
int buttonState = 0;
int rc;
int synch;

void rilevoButtonPressureCallback();
void rilevoCasualNumberCallback();

Task ButtonPressureTask(TASK_BUTTON_PRESSURE, TASK_FOREVER, &rilevoButtonPressureCallback);
Task CasualNumberTask(TASK_CASUAL_NUMBER, TASK_FOREVER, &rilevoCasualNumberCallback);

const char *data = "Callback function called";

void setup() {
  Serial.begin(115200);
  setTimeIT();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
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
  initializeStorage();
  scheduler.init();
  scheduler.addTask(ButtonPressureTask);
  scheduler.addTask(CasualNumberTask);
  ButtonPressureTask.enable();
  CasualNumberTask.enable();
}

void loop() {
  scheduler.execute();
}

void rilevoButtonPressureCallback() {
  buttonState = digitalRead(BUTTON_PIN);

  if (db_open("/littlefs/dati.db", &db))
    return;
  struct tm timeinfo = getDateAndTime();
  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1;
  int year = timeinfo.tm_year + 1900;
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;
  String fattaAmano = String(day) + "/" + String(month) + "/" + String(year) + " - " + String(hour) + ":" + String(minute) + ":" + String(second);
  char buffer[BUFFER_SIZE];
  synch = NO_SYNCHRONIZED;
  sprintf(buffer, "INSERT INTO t1 VALUES ('Button pressure', %d, '%s', %d, 2);", buttonState, fattaAmano.c_str(), synch);
  rc = db_exec(db, buffer);
  sqlite3_close(db);

  if (!client.connected()) {
    if (!client.connect(host, port)) {
      Serial.println("Connection to host failed");
      return;
    } else {
      Serial.println("Connected to server successful!");
    }
  }
  client.print("Button pressure," + String(buttonState) + "," + fattaAmano + "," + String(NO_SYNCHRONIZED) + ",2");
  client.print("\n");
}

void rilevoCasualNumberCallback() {
  mydata = random(0, 1000);

  if (db_open("/littlefs/dati.db", &db))
    return;
  struct tm timeinfo = getDateAndTime();
  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1;
  int year = timeinfo.tm_year + 1900;
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;
  String fattaAmano = String(day) + "/" + String(month) + "/" + String(year) + " - " + String(hour) + ":" + String(minute) + ":" + String(second);
  char buffer[BUFFER_SIZE];
  synch = NO_SYNCHRONIZED;
  sprintf(buffer, "INSERT INTO t1 VALUES ('Casual number', %d, '%s', %d, 1);", mydata, fattaAmano.c_str(), synch);
  rc = db_exec(db, buffer);
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
  client.print("Casual number," + String(mydata) + "," + fattaAmano + "," + String(NO_SYNCHRONIZED) + ",1");
  client.print("\n");
}
