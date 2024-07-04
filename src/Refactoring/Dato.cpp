#include "Dato.h"
#include <cmath>

Dato::Dato(char* nome, int maxReadings)
  : lastStoredTS(0), currentIndex(0), name(nome), maxReadings(maxReadings) {
  readings = new float[maxReadings];
  for (int i = 0; i < maxReadings; i++) {
    readings[i] = 0.0;
  }
}

Dato::~Dato() {
  delete[] readings;
}

void Dato::addReading(float value) {
  if (currentIndex < maxReadings) {
    readings[currentIndex] = value;
    currentIndex++;
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