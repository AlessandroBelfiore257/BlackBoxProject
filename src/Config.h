#ifndef CONFIG_H
#define CONFIG_H

#define FORMAT_LITTLEFS_IF_FAILED true

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET 3600
#define DAYLIGHT_OFFSET 3600

#define FILE_SYSTEM "/littlefs"
#define PATH_STORAGE_DATA_DB "/littlefs/dati.db"
#define DB_FILE "/dati.db"

#define MAX_READINGS_ACCELEROMETRIA 0
#define MAX_READINGS_ALCOOL 10
#define MAX_READINGS_AIRQUALITY 10
#define MAX_READINGS_GPS 0

#define DEBUG_OBD true

#define ACCELEROMETRIA_READ_INTERVAL 15 * TASK_SECOND
#define AIR_READ_INTERVAL 20 * TASK_SECOND
#define GPS_READ_INTERVAL 30 * TASK_SECOND
#define OBD_READ_INTERVAL 1 * TASK_SECOND
#define CENTRALINA_LOG 10 * TASK_SECOND
#define PROFILAZIONE_INTERVAL 10 * TASK_SECOND
#define ALLARMI_INTERVAL 6 * TASK_SECOND
#define WEEK (7 * 86400) * TASK_SECOND
#define SYNCHRONIZATION_DATA 60 * TASK_SECOND  // adesso sincronizzo ogni 60 sec, nella versione ufficiale la sincronizzazione avverrà in presenza di connessione e con un tempo maggiore di 24h dall'ultima sincronizzazione
#define CLEANING_ROUTINE 120 * TASK_SECOND     // adesso sono 120 secondi, nella versione ufficiale saranno 6h
#define CLEANING_MEMORY_FULL 300 * TASK_SECOND

#define GPSBAUD 38400

#define LED_ROSSO 13
#define LED_GIALLO 14
#define LED_VERDE 27

#define BUTTON_PIN 4
#define ALCOOL_PIN 33
#define AIR_QUALITY_PIN 32

#define BUFFER_SIZE 1000

#define SYNCHRONIZED 1
#define NO_SYNCHRONIZED 0

#define DATE_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define DATE_TIME_LEN 20

#define MAX_CAMPIONAMENTI_PROFILAZIONE 3
#define MAX_CAMPIONAMENTI_ALLARMI_ROSSI 6
#define MAX_CAMPIONAMENTI_ALLARMI_GIALLI 6

#define ACCELERAZIONE_G 9.81
#define TOLLERANZA_OFF_ROAD 5
#define LAT_MIN 36.624
#define LAT_MAX 47.092
#define LON_MIN 6.626
#define LON_MAX 18.480
#define ACCEL_THRESHOLD 0.5
#define LOAD_THRESHOLD 80.0

#define MAX_COOLANT_TEMP 150
#define MAX_OIL_TEMP 120
#define MIN_TORQUE_THRESHOLD 5
#define MAX_TORQUE_THRESHOLD 300

#define N_CLASSIFICATORI 5
#define ACCELERAZIONE_BRUSCA 4
#define DECELERAZIONE_BRUSCA -4
#define STERZATA_DX_BRUSCA 3
#define STERZATA_SX_BRUSCA -3
#define RANGE_INF 0.9
#define RANGE_SUP 1.3
#define LOAD_MIN 20
#define LOAD_MAX 80
#define CLEAN_PROFILAZIONE_DAY 30

#endif