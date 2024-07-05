#ifndef SYNCH_H
#define SYNCH_H

#include <Arduino.h>
#include <WiFi.h>
#include <sqlite3.h>
#include <vector>

/*
Descrizione: sincronizza i dati presenti nella tabella di memorizzazione relativa al monitoraggio
Input: db, ovvero il database utilizzato per memorizzare i dati
Return: void
*/
void syncMonitoraggioData(sqlite3* db);
/*
Descrizione: sincronizza i dati presenti nella tabella di memorizzazione relativa alla profilazione
Input: db, ovvero il database utilizzato per memorizzare i dati
Return: void
*/
void syncProfilazioneData(sqlite3* db);
/*
Descrizione: sincronizza i dati presenti nella tabella di memorizzazione relativa agli allarmi più critici
Input: db, ovvero il database utilizzato per memorizzare i dati
Return: void
*/
void syncAllarmiRossiData(sqlite3* db);
/*
Descrizione: sincronizza i dati presenti nella tabella di memorizzazione relativa agli allarmi meno critici
Input: db, ovvero il database utilizzato per memorizzare i dati
Return: void
*/
void syncAllarmiGialliData(sqlite3* db);

#endif