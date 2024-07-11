#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG

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
#define MAX_READINGS_LIQ_RAFFR 10
#define MAX_READINGS_OIL 10
#define MAX_READINGS_TORQUE 0 
#define MAX_READINGS_BATTERIA_V 0
#define MAX_READINGS_CARBURANTE_LEVEL 0

#define DEBUG_OBD true

#define ACCELEROMETRIA_READ_INTERVAL 15 * TASK_SECOND
#define AIR_READ_INTERVAL 20 * TASK_SECOND
#define GPS_READ_INTERVAL 30 * TASK_SECOND
#define CAR_OBD_STORAGE_INTERVAL 45 * TASK_SECOND
#define OBD_READ_INTERVAL 0.1 * TASK_SECOND
#define RESPONSE_TIME 2000
#define CENTRALINA_LOG 10 * TASK_SECOND
#define PROFILAZIONE_INTERVAL 10 * TASK_SECOND
#define ALLARMI_INTERVAL 6 * TASK_SECOND
#define WEEK (7 * 86400) * TASK_SECOND
#define SYNCHRONIZATION_DATA 60 * TASK_SECOND  // adesso sincronizzo ogni 60 sec, nella versione ufficiale la sincronizzazione avverrà in presenza di connessione e con un tempo maggiore di 24h dall'ultima sincronizzazione
#define CLEANING_ROUTINE 120 * TASK_SECOND     // adesso sono 120 secondi, nella versione ufficiale saranno 6h
#define CLEANING_MEMORY_FULL 300 * TASK_SECOND

#define GPS_TRACKING_SECURITY 300 * TASK_SECOND // ogni 5 min mando via GSM la posizione del veicolo
#define ACCIDENT_AND_ASSISTANCE 2 * TASK_SECOND // Rilevo frequentemente la forga G esercitata sul veicolo + la pressione del bottone

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

#define FORZA_G 9.81
#define TOLLERANZA_OFF_ROAD 4
#define LAT_MIN 36.624
#define LAT_MAX 47.092
#define LON_MIN 6.626
#define LON_MAX 18.480
#define ACCEL_THRESHOLD 0.5
#define RPM_THRESHOLD 3000 // Giri al minuto considerato alto per il traino
#define SPEED_THRESHOLD 50 // Km/h considerato basso per il traino a RPM alti
#define LOAD_THRESHOLD 75 // Percentuale di carico del motore considerato alto
#define TORQUE_THRESHOLD 200 // Nm di coppia considerato alto
#define THROTTLE_THRESHOLD 60 // Percentuale di apertura della valvola a farfalla considerato alto

#define SOGLIA_ALCOOL 500 // Soglia reale: 2000 dato che in assenza di alcool ci si aggira interno ai 1600/1700
#define SOGLIA_QUAL_ARIA 700 // Soglia reale: 700 dato che in assenza di gas nocivi ci si aggira intorno ai 450/500
#define MAX_COOLANT_TEMP 120
#define MAX_OIL_TEMP 230
#define MIN_TORQUE_THRESHOLD 100
#define MAX_TORQUE_THRESHOLD 250

#define N_CLASSIFICATORI 5
#define ACCELERAZIONE_BRUSCA 0.5
#define DECELERAZIONE_BRUSCA -0.5
#define STERZATA_DX_BRUSCA 0.5
#define STERZATA_SX_BRUSCA -0.5
#define MAX_SPEED_CAR 220
#define MAX_RPM_CAR 8000
#define RANGE_INF 0.9
#define RANGE_SUP 1.3
#define LOAD_MIN 20
#define LOAD_MAX 50
#define CLEAN_PROFILAZIONE_DAY 30

#define SOGLIA_RILEVAMENTO_INCIDENTE 4
#define LEN_BUTTON_ARRAY 6

#define REGULAR_DISPLAY_FIRST_LINE "Nessun problema"
#define REGULAR_DISPLAY_SECOND_LINE "rilevato"
#define SOGLIA_ALLARMI_ROSSI 0.5
#define SOGLIA_ALLARMI_GIALLI 0.5

#endif