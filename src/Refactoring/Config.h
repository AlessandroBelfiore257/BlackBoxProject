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
#define CENTRALINA_LOG 8
#define PROFILAZIONE 10
#define ALLARMI 6
#define OBD 200     // atrent: 1) attribuisci nomi più parlanti (es. qui sarebbe OBD_READ_INTERVAL o simile); 2) 200ms mi sa che sono troppo pochi, temo non riesca ad ottenere dati così frequentemente
#define WEEK 7 * 86400

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
#define LAT_MIN 36.624 // Latitudine Minima
#define LAT_MAX 47.092 // Latitudine Massima
#define LON_MIN 6.626  // Longitudine Minima
#define LON_MAX 18.480 // Longitudine Massima
#define ACCEL_THRESHOLD 0.5  // Variazione significativa nell'accelerazione
#define LOAD_THRESHOLD 80.0  // Aumento significativo del carico del motore

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