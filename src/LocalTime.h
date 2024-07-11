#ifndef LOCALTIME_H
#define LOCALTIME_H

#include <WiFi.h>
#include <time.h>
#include <sntp.h>
#include <RTClib.h>

/*
  Descrizione: configura e sincronizza l'orologio interno del dispositivo utilizzando NTP
  Input: -
  Return: void
*/
void setTimeNation();

/*
  Descrizione: converte un tempo espresso in secondi dall'epoch in una stringa formattata che rappresenta data e ora
  Input: 
    - total_seconds: numero di secondi dall'epoch (01/01/1970)
    - date_string: un puntatore a un array di caratteri dove sarà memorizzata la stringa formattata
  Return: void
*/
void secondsToString(long long int total_seconds, char* date_string);

/*
  Descrizione: converte una stringa timestamp formattata in millisecondi dall'epoch
  Input: timestamp, una stringa formattata rappresentante una data e un'ora
  Return: il numero di millisecondi trascorsi dall'epoch
*/
long long int stringToMilliseconds(const char* timestamp);

/*
  Descrizione: converte un tempo in secondi dall'epoch in un oggetto DateTime
  Input: epoch, un numero di secondi trascorsi dall'epoch (01/01/1970)
  Return: un ggetto DateTime rappresentante la data e l'ora specificate
*/
DateTime epochToDateTime(long long int epoch);

/*
  Descrizione: converte un oggetto DateTime in secondi dall'epoch
  Input: dt, un riferimento a un oggetto DateTime
  Return: il numero di secondi trascorsi dall'epoch
*/
long long int dateTimeToEpoch(DateTime& dt);

/*
  Descrizione: converte un oggetto DateTime in una stringa formattata rappresentante la data e l'ora
  Input: dt, in riferimento costante a un oggetto DateTime
  Return: una stringa formattata rappresentante la data e l'ora
*/
String dateTimeToString(const DateTime& dt);

#endif