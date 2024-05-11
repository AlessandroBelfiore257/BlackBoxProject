#ifndef LCTIME_H
#define LCTIME_H

#include <WiFi.h>
#include "time.h"
#include "sntp.h"

void setTimeIT();
struct tm getDateAndTime();
void printLocalTime();
void timeavailable(struct timeval *t);

#endif