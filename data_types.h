#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>

// Raw sensor readings — placeholder shape for now, expand as real sensors come online
#pragma pack(push, 1)
struct RawSensorData {
  float co;
  float no2;
  float so2;
  float nh3;
  float o3;
  float h2s;
  float co2;
  float pm1;
  float pm25;
  float pm10;
  float temperature;
  float humidity;
};
#pragma pack(pop)

// Extracted features — fed into classify().
// Classification now depends ONLY on these three calibrated gas
// concentrations (ppm), per the project's Gas Safety Ranges doc.
// avgGasLevel / pm25to10Ratio removed — no longer used for classification.
struct ExtractedFeatures {
  float co;   // calibrated CO, ppm
  float nh3;  // calibrated NH3, ppm
  float no2;  // calibrated NO2, ppm
};

// Final classification result
struct ClassificationResult {
  uint8_t classification;   // 0=Good, 1=Moderate, 2=Poor, 3=Hazardous
  uint8_t confidenceScore;  // 0-100
};

#endif
