#include "LCtime.h"

#include "Secrets.h"
#include "Config.h"

const char* ntpServer1 = NTP_SERVER1;
const char* ntpServer2 = NTP_SERVER2;
const long gmtOffset_sec = GMT_OFFSET;
const int daylightOffset_sec = DAYLIGHT_OFFSET;

extern RTC_DS3231 rtc;
struct tm timeinfo;

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval* t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setTimeIT() {
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
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  // set the time to the RTC
  if (getLocalTime(&timeinfo)) {
    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
  } else {
    Serial.println("Failed to obtain time");
  }
  DateTime now = rtc.now();
  Serial.print("Primo: ");
  Serial.printf("%02d-%02d-%02d %02d:%02d:%02d\n", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
}

// Funzione per convertire un timestamp sotto forma di secondi in una stringa nel formato "YYYY-MM-DD HH:MM:SS"
void SecondsToString(long long int total_seconds, char* date_string) {
  time_t raw_time = (time_t)total_seconds;
  struct tm* timeinfo = localtime(&raw_time);
  strftime(date_string, DATE_TIME_LEN, DATE_TIME_FORMAT, timeinfo);
}

/*
  Funzione per convertire un timestamp sotto forma di stringa in secondi
*/
long long int stringToMilliseconds(const char* timestamp) {
  struct tm tm;
  strptime(timestamp, DATE_TIME_FORMAT, &tm);
  time_t time_seconds = mktime(&tm);
  return time_seconds;
}

DateTime EpochToDateTime(long long int epoch) {
  return DateTime(static_cast<uint32_t>(epoch));
}

long long int DateTimeToEpoch(DateTime& dt) {
  return dt.unixtime();
}

String DateTimeToString(const DateTime& dt) {
  char buffer[DATE_TIME_LEN];
  snprintf(buffer, sizeof(buffer), "%02d-%02d-%02d %02d:%02d:%02d", dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}