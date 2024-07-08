#ifndef FILESYSTEM_H
#define FILESYSTEM_H

/*
  Descrizione: monta il file system LittleFS sulla scheda di memoria del dispositivo. Questa funzione è essenziale per abilitare l'accesso alla memorizzazione interna del dispositivo
  Input: s, un puntatore a una stringa di caratteri che rappresenta il percorso del file system
  Return: ritorna `true` se il montaggio del file system è avvenuto con successo, `false` in caso contrario
*/
bool mountingFS(char* s);

/*
  Descrizione: apre il file system montato e visualizza la struttura delle directory e dei file presenti nella memoria.
  Questa funzione permette di esplorare il contenuto della memoria interna del dispositivo e di rimuovere file specificati
  Input: nessun parametro
  Return: void
*/
void openFS();

#endif