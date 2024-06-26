#ifndef LCTIME_H
#define LCTIME_H

#include <WiFi.h>
#include <time.h>
#include <sntp.h>
#include <RTClib.h>

void setTimeIT();
void SecondsToString(long long int total_seconds, char* date_string);
long long int stringToMilliseconds(const char* timestamp);
DateTime EpochToDateTime(long long int epoch);
long long int DateTimeToEpoch(DateTime& dt);
String DateTimeToString(const DateTime& dt);

#endif