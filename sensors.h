#ifndef SENSORS_H
#define SENSORS_H

#include "data_types.h"

// MATTERA current hardware: CJMCU/MiCS-6814 analog channels on ESP32-S3.
// Values in co/nh3/no2 are RAW 12-bit ADC counts (0..4095), NOT ppm.
void setupSensors();
RawSensorData readAllSensors();

#endif
