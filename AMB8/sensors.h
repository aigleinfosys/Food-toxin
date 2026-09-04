#ifndef SENSORS_H
#define SENSORS_H

#include "data_types.h"

// MATTERA hardware: CJMCU/MiCS-6814 analog channels on the Realtek
// AMB82-Mini (RTL8735B). Values in co/nh3/no2 are RAW ADC counts,
// NOT ppm (see signal_processing.cpp for the ADC-bit-depth caveat).
void setupSensors();
RawSensorData readAllSensors();

#endif
