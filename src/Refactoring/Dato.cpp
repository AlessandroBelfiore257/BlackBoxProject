#include "Dato.h"
#include <cmath> // Per la funzione pow e sqrt

Dato::Dato(char* nome, int maxReadings) : lastStoredTS(0), currentIndex(0), name(nome), maxReadings(maxReadings) {
    readings = new float[maxReadings];  // Allocazione dinamica dell'array
    for (int i = 0; i < maxReadings; i++) {
        readings[i] = 0.0;
    }
}

// Distruttore per liberare la memoria allocata dinamicamente
Dato::~Dato() {
    delete[] readings;
}

void Dato::addReading(float value) {
    if (currentIndex < maxReadings) {
        readings[currentIndex] = value;
        currentIndex++;
    } else {
        for (int i = 1; i < maxReadings; i++) {
            readings[i - 1] = readings[i];
        }
        readings[maxReadings - 1] = value;
    }
}

float Dato::calculateMean() {
    float sum = 0;
    for (int i = 0; i < currentIndex; i++) {
        sum += readings[i];
    }
    return sum / currentIndex;
}

float Dato::calculateStdDev() {
    float mean = calculateMean();
    float sum = 0;
    for (int i = 0; i < currentIndex; i++) {
        sum += pow(readings[i] - mean, 2);
    }
    return sqrt(sum / currentIndex);
}

