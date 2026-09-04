#include "sensors.h"
#include "DHT.h"

// ============================================================
// MATTERA - AMB82-Mini (RTL8735B) + MiCS-6814
// ============================================================

// MiCS-6814 analog connections.
// AMB82-Mini exposes exactly 3 ADC-capable pins (A0/A1/A2), which
// matches this project's 3-channel gas sensor array.
static const uint8_t CO_PIN  = A0;
static const uint8_t NH3_PIN = A1;
static const uint8_t NO2_PIN = A2;

// Number of ADC samples used per reading (trimmed-mean filtering)
static const uint16_t ADC_SAMPLES = 50;

// DHT22 ambient temperature/humidity sensor.
// NOTE: GPIO 15 carried over from the ESP32 pin number -- AMB82-Mini has
// a different GPIO map, verify this pin is free (not reserved for the
// camera/flash interface) before wiring it up.
static const int DHT_PIN = 15;
static DHT dhtSensor(DHT_PIN, DHT22);


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

  // NOTE: analogReadResolution() / analogSetPinAttenuation() were
  // ESP32-Arduino-core-only calls with no Ameba equivalent -- AMB82-Mini's
  // ADC runs at its native resolution/range. See the ADC_MAX comment in
  // signal_processing.cpp.

  Serial.println("MiCS-6814 ADC configured");

  Serial.println("CO  -> A0");
  Serial.println("NH3 -> A1");
  Serial.println("NO2 -> A2");

  dhtSensor.begin();
  Serial.println("DHT22 -> GPIO 15");
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
  // 3. Print RAW ADC
  // ----------------------------------------------------------
  //
  // NOTE: analogReadMilliVolts() was an ESP32-only helper with no Ameba
  // equivalent -- dropped from this debug print. Raw ADC counts (still
  // used by the pipeline) are unaffected.

  Serial.println("----------------------------------------");

  Serial.print("CO  : ");
  Serial.print(coADC);
  Serial.println(" ADC");

  Serial.print("NH3 : ");
  Serial.print(nh3ADC);
  Serial.println(" ADC");

  Serial.print("NO2 : ");
  Serial.print(no2ADC);
  Serial.println(" ADC");


  // ----------------------------------------------------------
  // 4. Sensors not connected yet
  // ----------------------------------------------------------

  data.so2  = 0.0f;
  data.o3   = 0.0f;
  data.h2s  = 0.0f;
  data.co2  = 0.0f;

  data.pm1  = 0.0f;
  data.pm25 = 0.0f;
  data.pm10 = 0.0f;


  // ----------------------------------------------------------
  // 5. DHT22 ambient temperature + humidity
  // ----------------------------------------------------------

  data.temperature = dhtSensor.readTemperature();
  data.humidity = dhtSensor.readHumidity();

  Serial.print("DHT22 Temp: ");
  Serial.print(data.temperature, 2);
  Serial.print(" C | Humidity: ");
  Serial.print(data.humidity, 1);
  Serial.println(" %");


  // ----------------------------------------------------------
  // Return complete sensor structure
  // ----------------------------------------------------------

  return data;
}
