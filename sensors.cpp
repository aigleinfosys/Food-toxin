#include "sensors.h"

// ============================================================
// MATTERA - ESP32-S3 + MiCS-6814
// ============================================================

// MiCS-6814 analog connections
static const uint8_t CO_PIN  = 1;
static const uint8_t NH3_PIN = 2;
static const uint8_t NO2_PIN = 3;

// Number of ADC samples used per reading (trimmed-mean filtering)
static const uint16_t ADC_SAMPLES = 50;


// ============================================================
// Read filtered ADC value — averages 50 samples, dropping the
// single highest and lowest reading to reject outlier noise.
// (Matches the project's recalibrated reference implementation.)
// ============================================================

static float readGasADC(uint8_t pin) {

  long total = 0;
  int minValue = 4095;
  int maxValue = 0;

  for (uint16_t i = 0; i < ADC_SAMPLES; i++) {

    int value = analogRead(pin);

    total += value;

    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;

    delay(5);
  }

  // Remove the single highest and lowest reading before averaging
  return (total - minValue - maxValue) / (float)(ADC_SAMPLES - 2);
}


// ============================================================
// Sensor setup
// ============================================================

void setupSensors() {

  // 12-bit ADC
  analogReadResolution(12);

  // ADC attenuation
  analogSetPinAttenuation(CO_PIN, ADC_11db);
  analogSetPinAttenuation(NH3_PIN, ADC_11db);
  analogSetPinAttenuation(NO2_PIN, ADC_11db);

  // Configure ADC pins
  pinMode(CO_PIN, INPUT);
  pinMode(NH3_PIN, INPUT);
  pinMode(NO2_PIN, INPUT);

  Serial.println("MiCS-6814 ADC configured");

  Serial.println("CO  -> GPIO 1");
  Serial.println("NH3 -> GPIO 2");
  Serial.println("NO2 -> GPIO 3");
}


// ============================================================
// Read all sensors
// ============================================================

RawSensorData readAllSensors() {

  RawSensorData data = {};


  // ----------------------------------------------------------
  // 1. Read averaged RAW ADC values
  // ----------------------------------------------------------

  float coADC  = readGasADC(CO_PIN);
  float nh3ADC = readGasADC(NH3_PIN);
  float no2ADC = readGasADC(NO2_PIN);


  // ----------------------------------------------------------
  // 2. Keep RAW ADC values for the existing software pipeline
  // ----------------------------------------------------------
  //
  // IMPORTANT:
  // These values remain exactly the same type of values that
  // your BLE / signal-processing / ML code was already using.
  //
  // We are NOT replacing ADC counts with millivolts.
  //

  data.co  = (float)coADC;
  data.nh3 = (float)nh3ADC;
  data.no2 = (float)no2ADC;


  // ----------------------------------------------------------
  // 3. Read ADC voltage in millivolts
  // ----------------------------------------------------------
  //
  // These values are ONLY for debugging/calibration.
  // They are NOT sent into the existing ML pipeline.
  //

  uint32_t co_mV  = analogReadMilliVolts(CO_PIN);
  uint32_t nh3_mV = analogReadMilliVolts(NH3_PIN);
  uint32_t no2_mV = analogReadMilliVolts(NO2_PIN);


  // ----------------------------------------------------------
  // 4. Print RAW ADC + voltage
  // ----------------------------------------------------------

  Serial.println("----------------------------------------");

  Serial.print("CO  : ");
  Serial.print(coADC);
  Serial.print(" ADC / ");
  Serial.print(co_mV);
  Serial.println(" mV");

  Serial.print("NH3 : ");
  Serial.print(nh3ADC);
  Serial.print(" ADC / ");
  Serial.print(nh3_mV);
  Serial.println(" mV");

  Serial.print("NO2 : ");
  Serial.print(no2ADC);
  Serial.print(" ADC / ");
  Serial.print(no2_mV);
  Serial.println(" mV");


  // ----------------------------------------------------------
  // 5. Sensors not connected yet
  // ----------------------------------------------------------

  data.so2  = 0.0f;
  data.o3   = 0.0f;
  data.h2s  = 0.0f;
  data.co2  = 0.0f;

  data.pm1  = 0.0f;
  data.pm25 = 0.0f;
  data.pm10 = 0.0f;


  // ----------------------------------------------------------
  // 6. ESP32-S3 internal temperature
  // ----------------------------------------------------------
  //
  // This is BOARD temperature, not ambient temperature.
  // SHT40 will replace this later.
  //

  data.temperature = temperatureRead();

  data.humidity = 0.0f;


  // ----------------------------------------------------------
  // Return complete sensor structure
  // ----------------------------------------------------------

  return data;
}