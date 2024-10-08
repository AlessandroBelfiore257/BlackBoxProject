#ifndef DATO_H
#define DATO_H

#include <Arduino.h>

class Dato {
public:
  long long int lastStoredTS;
  float* readings;
  int maxReadings;
  int currentIndex;
  char* name;

  Dato(char* nome, int maxReadings);
  void addReading(float value);
  float calculateStdDev();
  void cleanParameters();

private:
  float calculateMean();
};

#endif