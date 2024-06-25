#ifndef DATO_H
#define DATO_H

#include <Arduino.h>  // Aggiungi questa linea per usare la classe String

class Dato {
public:
    long long int lastStoredTS;
    float* readings;  // Puntatore a un array di float
    int maxReadings;
    int currentIndex;
    char* name;

    // Costruttore
    Dato(char* nome, int maxReadings);

    // Distruttore
    ~Dato();

    // Metodo per aggiungere una lettura
    void addReading(float value);

    // Metodo per calcolare la deviazione standard
    float calculateStdDev();

private:
    // Metodo per calcolare la media
    float calculateMean();
};

#endif // DATO_H
