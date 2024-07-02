#ifndef CONFIG_H
#define CONFIG_H

#define FORMAT_LITTLEFS_IF_FAILED true

#define max_readings_Accelerometria 0
#define max_readings_Alcool 10
#define max_readings_AirQuality 10
#define max_readings_GPS 0

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET 3600
#define DAYLIGHT_OFFSET 3600

#define RX_PIN 16
#define TX_PIN 17
#define GPSBAUD 38400

#define BUTTON_PIN 4
#define ALCOOL_PIN 33
#define AIR_QUALITY_PIN 32

#define LED_ROSSO 13
#define LED_GIALLO 14
#define LED_VERDE 27

#define FILE_SYSTEM "/littlefs"
#define PATH_STORAGE_DATA_DB "/littlefs/dati.db"
#define DB_FILE "/dati.db"

#define SYNCHRONIZATION_DATA 60  // adesso sincronizzo ogni 60 sec, nella versione ufficiale la sincronizzazione avverrà in presenza di connessione e con un tempo maggiore di 24h dall'ultima sincronizzazione
#define CLEANING_ROUTINE 120     // adesso sono 120 secondi, nella versione ufficiale saranno 6h
#define CLEANING_MEMORY_FULL 300
#define GPS 30
#define ACCELEROMETRIA 15
#define AIR 20
#define CENTRALINA_LOG 15
#define PROFILAZIONE 10
#define ALLARMI 6
#define OBD 2

#define BUFFER_SIZE 1000

#define SYNCHRONIZED 1
#define NO_SYNCHRONIZED 0

#define DATE_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define DATE_TIME_LEN 20

#define MAX_CAMPIONAMENTI_PROFILAZIONE 3
#define MAX_CAMPIONAMENTI_ALLARMI_ROSSI 6
#define MAX_CAMPIONAMENTI_ALLARMI_GIALLI 6

#endif