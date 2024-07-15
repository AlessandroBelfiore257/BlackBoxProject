#ifndef GARBAGE_H
#define GARBAGE_H

#include <sqlite3.h>
#include <vector>

#include "Storage.h"
#include "Config.h"

/*
  Descrizione: elimina i record appartenenti alla tabella t1 (monitoraggio) sincronizzati, liberando così spazio
  Input: -
  Return: void
*/
void garbageCollectorRoutine();

/*
  Descrizione: esegue una pulizia quando la memoria è piena, eliminando una parte significativa (50%) dei record presenti all'interno di T1 più vecchi
  Input: -
  Return: void
*/
void garbageCollectorMemoryFull();

/*
  Descrizione: elimina i voti di profilazione presenti all'interno della tabella t3 che sono stati sincronizzati e che hanno superato una certa soglia temporale
  Input: db, ovvero il db di memorizzazione dei dati
  Return: void
*/
void eliminazioneVotiProfilazione(sqlite3* db);

/*
  Descrizione: elimina i record di allarmi critici (rossi) che sono stati sincronizzati con il server
  Input: db, ovvero il db di memorizzazione dei dati
  Return: void
*/
void cleanAllarmiRed(sqlite3* db);

/*
  Descrizione: reimposta lo stato di sincronizzazione e altri campi per i record di allarmi meno critici (gialli)
  Input: db, ovvero il db di memorizzazione dei dati
  Return: void
*/
void cleanAllarmiYellow(sqlite3* db);

#endif