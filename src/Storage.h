#ifndef STORAGE_H
#define STORAGE_H

#include <sqlite3.h>
#include "LocalTime.h"

/*
  Descrizione: inizializza il database e crea le tabelle necessarie
  Input: path, il percorso del file del database
  Return: void
*/
void initializeStorage(char* path);

/*
  Descrizione: apre una connessione al database SQLite
  Input: 
   - filename, il nome del file del database
   - db, ovvero il database utilizzato per memorizzare i dati
  Return: 0 se l'apertura è riuscita, altrimenti un codice di errore
*/
int db_open(const char* filename, sqlite3** db);

/*
  Descrizione: esegue una query SQL sul database
  Input: 
   - db, ovvero il database utilizzato per memorizzare i dati
   - sql: query SQL da eseguire
  Return: il codice di ritorno di SQLite, 0 se l'esecuzione è riuscita
*/
int db_exec(sqlite3* db, const char* sql);

/*
  Descrizione: crea un record nella tabella t1 del database
  Input: 
   - name_sensor: nome del sensore
   - value: valore rilevato
   - data_type: tipo di dato del valore
   - unit_of_measure: unità di misura del valore
   - ts: timestamp della rilevazione
   - synch: flag che indica se il record è stato sincronizzato
   - pr: priorità del record
  Return: void
 */
void createRecordToInsertIntot1(char* name_sensor, const char* value, char* data_type, char* unit_of_measure, long long int ts, int synch, int pr);

/*
  Descrizione: recupera il tempo di storage per un sensore specificato dalla tabella t2
  Input: 
   - db, ovvero il database utilizzato per memorizzare i dati
   - nome_sensore: Nome del sensore di cui si vuole ottenere il tempo di storage
  Return: il tempo di storage del sensore
*/
long long int getTempoStorage(sqlite3* db, const char* nome_sensore);

/*
  Descrizione: recupera i parametri di configurazione per un sensore specificato dalla tabella t2
  Input: 
   - sensorName: Nome del sensore di cui si vogliono ottenere i parametri
   - tempoStorage: Riferimento per memorizzare il tempo di storage del sensore
   - fattoreIncrDecr: Riferimento per memorizzare il fattore di incremento/decremento
   - soglia: Riferimento per memorizzare la soglia
   - tMinStorage: Riferimento per memorizzare il tempo minimo di storage
   - tMaxStorage: Riferimento per memorizzare il tempo massimo di storage
  Return: true se i parametri sono stati recuperati con successo, false altrimenti
*/
bool getSensorParameters(const char* sensorName, int& tempoStorage, int& fattoreIncrDecr, float& soglia, int& tMinStorage, int& tMaxStorage);

/*
  Descrizione: aggiorna il tempo di storage per un sensore specificato
  Input: 
   - sensorName: Nome del sensore di cui si vuole aggiornare il tempo di storage
   - newTempoStorage: Nuovo tempo di storage
  Return: void
 */
void updateTempoStorage(const char* sensorName, int newTempoStorage);

/*
  Descrizione: crea un record nella tabella t3 per la profilazione
  Input: 
   - date: Data della profilazione
   - normalizedNumber: Numero normalizzato della profilazione
   - lastStorage: Ultimo timestamp di storage
   - synchronised: Flag che indica se il record è stato sincronizzato
  Return: void
*/
void createRecordToInsertIntot3(const char* date, const char* normalizedNumber, long long int lastStorage, int synchronised);

/*
  Descrizione: recupera l'ultimo timestamp dalla tabella t3
  Input: 
   - db, ovvero il database utilizzato per memorizzare i dati
  Return: l'ultimo timestamp presente nella tabella t3
*/
long long int getLastTimestampFromt3(sqlite3* db);

/*
  Descrizione: crea un record nella tabella t4 per la segnalazione degli allarmi rossi
  Input: 
   - problem: descrizione del problema
   - date: Data del problema
   - synchronised: flag che indica se il record è stato sincronizzato
 Return: void
*/
void createRecordToInsertIntot4(const char* problem, const char* date, int synchronised);

/*
  Descrizione: Verifica se un problema esiste già nella tabella t4
  Input: 
   - db: Puntatore alla connessione del database
   - problem: Descrizione del problema da verificare
  Return: true se il problema esiste già, false altrimenti
*/
bool alreadyExists(sqlite3* db, const char* problem);

/*
  Descrizione: recupera il valore di una colonna specificata dalla tabella t5 per un determinato problema
  Input: 
   - problem: descrizione del problema
   - column: nome della colonna di cui si vuole ottenere il valore
  Return: il valore della colonna per il problema specificato
*/
int getValueFromT5(const char* problem, const char* column);

/*
 * Descrizione: aggiorna il valore di una colonna specificata nella tabella t5 per un determinato problema
 * Input: 
 *  - problem: descrizione del problema
 *  - column: nome della colonna da aggiornare
 *  - value: nuovo valore per la colonna
 * Return: void
 */
void updateColumnValue(const char* problem, const char* column, int value);

#endif 