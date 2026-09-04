#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "data_types.h"

// Existing pipeline entry point — UNCHANGED signature, still called exactly
// the same way from AMB8.ino's loop(). Internally it now runs the real
// MiCS-6814 calibration (ADC -> Vout -> Rs -> Rs/R0 -> PPM) on co/nh3/no2
// before passing the struct along.
RawSensorData processSignal(RawSensorData raw);

#endif
