#ifndef GARBAGE_H
#define GARBAGE_H

#include <sqlite3.h>
#include <vector>
#include "Storage.h"
#include "Config.h"

void garbageCollectorRoutine();
void garbageCollectorMemoryFull();

void eliminazioneVotiProfilazione(sqlite3* db);

#endif