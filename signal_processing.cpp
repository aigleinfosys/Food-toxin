#include "signal_processing.h"
#include <math.h>

// ============================================================
// MiCS-6814 calibration — recalibrated reference values.
// Source: recaliberated_ppm.ino (latest project calibration reference)
// All outputs below are in PPM.
// ============================================================

static const float VCC = 3.3f;
static const float ADC_MAX = 4095.0f;
static const float RL = 47000.0f; // 47 kOhm load resistor (shared across channels)

// New baseline R0 values (ohms), from the recalibrated reference
static const float R0_CO  = 119800.0f; // 119.8 kOhm
static const float R0_NH3 = 408000.0f; // derived from last live ADC=3672 reading -- NOT a dedicated clean-air calibration, refine when you can
static const float R0_NO2 = 1410.0f;   // 1.41 kOhm


// ============================================================
// ADC -> Voltage
// ============================================================

static float adcToVoltage(float adc) {
  return (adc / ADC_MAX) * VCC;
}


// ============================================================
// Voltage -> Sensor resistance (Rs)
//
// NOTE: this formula's orientation is Rs = RL * V / (VCC - V) —
// the INVERSE of a simple pull-down divider. This matches the
// project's actual circuit topology as confirmed in the recalibrated
// reference. Do not swap this back to RL*(VCC-V)/V.
// ============================================================

static float calculateRs(float voltage) {
  if (voltage >= (VCC - 0.001f)) {
    return -1.0f; // invalid reading, avoid divide-by-zero
  }
  return RL * voltage / (VCC - voltage);
}


// ============================================================
// Gas-specific PPM curves (from the recalibrated reference)
// ============================================================

static float calculateCOPPM(float ratio) {
  if (ratio <= 0.0f) return 0.0f;
  return 4.48f * powf(ratio, -1.45f);
}

static float calculateNH3PPM(float ratio) {
  if (ratio <= 0.0f) return 0.0f;
  // Below the calibrated range (ratio > 1.8), the reference reports
  // "< 1 ppm" rather than a precise number. We can't send text over
  // BLE, so this is reported as 0 ppm — correctly falls into the
  // "Good" classification bucket either way.
  if (ratio > 1.8f) return 0.0f;
  return 2.2f * powf(ratio, -1.5f);
}

static float calculateNO2PPM(float ratio) {
  if (ratio <= 0.0f) return 0.0f;
  return ratio / 6.0f; // linear, not power-law, per the reference
}


// ============================================================
// Per-channel calibration pipeline: ADC -> Vout -> Rs -> Rs/R0 -> PPM
// ============================================================

static float calibrateCO(float rawAdc) {
  float v  = adcToVoltage(rawAdc);
  float Rs = calculateRs(v);
  if (Rs <= 0.0f) return 0.0f;
  return calculateCOPPM(Rs / R0_CO);
}

static float calibrateNH3(float rawAdc) {
  float v  = adcToVoltage(rawAdc);
  float Rs = calculateRs(v);
  if (Rs <= 0.0f) return 0.0f;
  return calculateNH3PPM(Rs / R0_NH3);
}

static float calibrateNO2(float rawAdc) {
  float v  = adcToVoltage(rawAdc);
  float Rs = calculateRs(v);
  if (Rs <= 0.0f) return 0.0f;
  return calculateNO2PPM(Rs / R0_NO2);
}


// ============================================================
// Existing pipeline entry point — signature unchanged
// ============================================================

RawSensorData processSignal(RawSensorData raw) {
  RawSensorData processed = raw;

  // raw.co / raw.nh3 / raw.no2 arrive as RAW ADC counts.
  // All three are converted to PPM here (not ppb).
  processed.co  = calibrateCO(raw.co);
  processed.nh3 = calibrateNH3(raw.nh3);
  processed.no2 = calibrateNO2(raw.no2);

  // Everything else (so2, o3, h2s, co2, pm1/25/10, temperature, humidity)
  // passes through untouched — no sensor exists for these yet, stays 0.

  return processed;
}
