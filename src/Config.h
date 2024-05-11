#ifndef _CONFIG_H
#define _CONFIG_H

#define FORMAT_LITTLEFS_IF_FAILED true

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET 3600
#define DAYLIGHT_OFFSET 3600

// Regola del fuso orario per Europa/Roma, inclusa la regolazione dell'ora legale (opzionale)
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

#define BUTTON 4

#define TASK_BUTTON_PRESSURE 30000
#define TASK_CASUAL_NUMBER 30000

#define BUFFER_SIZE 1000

#define SYNCHRONIZED 1
#define NO_SYNCHRONIZED 0

#endif