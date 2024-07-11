#include "LocalTime.h"

#include "Secrets.h"
#include "Config.h"

extern RTC_DS3231 rtc;
struct tm timeinfo;

/*
  Descrizione: stampa sulla seriale l'ora locale attuale 
  Input: -
  Return: void
*/
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    #ifdef DEBUG
    Serial.println("No time available (yet)");
    #endif
    return;
  }
  #ifdef DEBUG
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  #endif
}

/*
  Descrizione: funzione chiamata quando si riceve un aggiornamento dell'ora tramite NTP
  Input: t, ovvero la struttura contenente tutte le informazioni relative all'orario
  Return: void
*/
void timeavailable(struct timeval* t) {
  #ifdef DEBUG
  Serial.println("Got time adjustment from NTP!");
  #endif
  printLocalTime();
}

void setTimeNation() {
  sntp_set_time_sync_notification_cb(timeavailable);
  /**
   * NTP server address could be aquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE aquired NTP server address
   */
  sntp_servermode_dhcp(1);  // (optional)
  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagicaly.
   */
  configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER1, NTP_SERVER2);
  if (getLocalTime(&timeinfo)) {
    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
  } else {
    #ifdef DEBUG
    Serial.println("Failed to obtain time");
    #endif
  }
  DateTime now = rtc.now();
  #ifdef DEBUG
  Serial.print("Primo: ");
  Serial.printf("%02d-%02d-%02d %02d:%02d:%02d\n", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  #endif
}

void secondsToString(long long int total_seconds, char* date_string) {
  time_t raw_time = (time_t)total_seconds;
  struct tm* timeinfo = localtime(&raw_time);
  strftime(date_string, DATE_TIME_LEN, DATE_TIME_FORMAT, timeinfo);
}

long long int stringToMilliseconds(const char* timestamp) {
  struct tm tm;
  strptime(timestamp, DATE_TIME_FORMAT, &tm);
  time_t time_seconds = mktime(&tm);
  return time_seconds;
}

DateTime epochToDateTime(long long int epoch) {
  return DateTime(static_cast<uint32_t>(epoch));
}

long long int dateTimeToEpoch(DateTime& dt) {
  return dt.unixtime();
}

String dateTimeToString(const DateTime& dt) {
  char buffer[DATE_TIME_LEN];
  snprintf(buffer, sizeof(buffer), "%02d-%02d-%02d %02d:%02d:%02d", dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}