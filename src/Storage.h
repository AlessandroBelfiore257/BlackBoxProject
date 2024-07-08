#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>

#include "LocalTime.h"

/*
  Descrizione: inizializza il database e crea le tabelle necessarie per il monitoraggio dati, segnalazione di allarmi e profilazione guida del conducente
  Input: path, ovvero il percorso in cui memorizzare il database e le rispettive tabelle 
  Return: void
*/
void initializeStorage(char* path);

/*
  Descrizione: apre una connessione al database db SQLite
  Input: 
   - filename, il nome del file in cui è memorizzato il database
   - db, ovvero il database utilizzato per memorizzare i dati
  Return: 0 se l'apertura è riuscita, altrimenti un numero diverso da 0 indicante un codice d'errore
*/
int db_open(const char* filename, sqlite3** db);

/*
  Descrizione: esegue una query SQL sul database
  Input: 
   - db, ovvero il database utilizzato per memorizzare i dati
   - sql: query SQL da eseguire
  Return: il codice di ritorno di SQLite: 0 se l'esecuzione è riuscita, un numero diverso da 0 altrimenti
*/
int db_exec(sqlite3* db, const char* sql);

/*
  Descrizione: crea un record da inserire nella tabella t1 del database
  Input: 
   - name_sensor: nome del sensore
   - value: valore rilevato dal sensore 
   - data_type: tipo di dato del valore
   - unit_of_measure: unità di misura del valore
   - ts: timestamp della rilevazione
   - synch: flag che indica se il record è stato sincronizzato o meno
   - pr: priorità del record
  Return: void
 */
void createRecordToInsertIntoT1(char* name_sensor, const char* value, char* data_type, char* unit_of_measure, long long int ts, int synch, int pr);

/*
  Descrizione: restituisce il tempo di storage per un sensore specificato presente all'interno della tabella t2
  Input: 
   - db, ovvero il database utilizzato per memorizzare i dati
   - nome_sensore: nome del sensore di cui si vuole ottenere il tempo di storage
  Return: il tempo di storage del sensore
*/
long long int getTempoStorageFromT2(sqlite3* db, const char* nome_sensore);

/*
  Descrizione: recupera i parametri di configurazione per un sensore specificato dalla tabella t2
  Input: 
   - sensorName: nome del sensore di cui si vogliono ottenere i parametri
   - tempoStorage: il tempo di storage del sensore
   - fattoreIncrDecr: il fattore di incremento/decremento 
   - soglia: la soglia indicante un indice di omogeneità / eterogeneità tra i dati
   - tMinStorage: il tempo minimo di storage
   - tMaxStorage: il tempo massimo di storage
  Return: true se i parametri sono stati recuperati con successo, false altrimenti
*/
bool getSensorParametersFromT2(const char* sensorName, int& tempoStorage, int& fattoreIncrDecr, float& soglia, int& tMinStorage, int& tMaxStorage);

/*
  Descrizione: aggiorna il tempo di storage per un sensore specificato all'interno della tabella t2
  Input: 
   - sensorName: nome del sensore di cui si vuole aggiornare il tempo di storage
   - newTempoStorage: nuovo tempo di storage
  Return: void
 */
void updateTempoStorageIntoT2(const char* sensorName, int newTempoStorage);

/*
  Descrizione: crea un record da inserire nella tabella t3 relativa alla profilazione guida
  Input: 
   - date: data della profilazione
   - normalizedNumber: voto attribuito al conducente del veicolo
   - lastStorage: timestamp del voto attribuito al conducente
   - synchronised: flag che indica se il record è stato sincronizzato o meno
  Return: void
*/
void createRecordToInsertIntoT3(const char* date, const char* normalizedNumber, long long int lastStorage, int synchronised);

/*
  Descrizione: recupera l'ultimo timestamp (quello più recente) dalla tabella t3
  Input: db, ovvero il database utilizzato per memorizzare i dati
  Return: l'ultimo timestamp presente nella tabella t3
*/
long long int getLastTimestampFromT3(sqlite3* db);

/*
  Descrizione: crea un record da inserire nella tabella t4 relativa alla segnalazione degli allarmi rossi
  Input: 
   - problem: descrizione del problema
   - date: data del problema
   - synchronised: flag che indica se il record è stato sincronizzato o meno
 Return: void
*/
void createRecordToInsertIntoT4(const char* problem, const char* date, int synchronised);

/*
  Descrizione: verifica se un problema esiste già nella tabella t4
  Input: 
   - db: ovvero il database utilizzato per memorizzare i dati
   - problem: descrizione del problema da verificare
  Return: true se il problema esiste già, false altrimenti
*/
bool alreadyExistsIntoT4(sqlite3* db, const char* problem);

/*
  Descrizione: recupera il valore di una colonna specificata nella tabella t5 per un determinato problema
  Input: 
   - problem: descrizione del problema
   - column: nome della colonna di cui si vuole ottenere il valore
  Return: il valore della colonna per il problema specificato
*/
int getValueFromT5(const char* problem, const char* column);

/*
  Descrizione: aggiorna il valore di una colonna specificata nella tabella t5 per un determinato problema
  Input: 
   - problem: descrizione del problema
   - column: nome della colonna da aggiornare
   - value: nuovo valore per la colonna
  Return: void
*/
void updateColumnValueIntoT5(const char* problem, const char* column, int value);

#endif