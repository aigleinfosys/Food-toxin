#include "BLEDevice.h"
#include <Adafruit_NeoPixel.h>
#include "data_types.h"
#include "sensors.h"
#include "signal_processing.h"
#include "feature_extraction.h"
#include "ml_model.h"

// Same UUIDs as the ESP32 version -- unchanged so the existing app can
// connect to this board without any app-side changes.
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define LED_PIN 48
#define NUM_PIXELS 1

Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Ameba's BLE library (BLEDevice.h) uses global BLEService/BLECharacteristic
// objects plus a singleton `BLE` device, instead of ESP32's heap-allocated
// BLEServer/BLECharacteristic pointers + BLE2902 descriptor. Functionally
// equivalent -- same service, same characteristic, same notify behavior.
BLEService enviroService(SERVICE_UUID);
BLECharacteristic reportCharacteristic(CHARACTERISTIC_UUID);
BLEAdvertData advData;
BLEAdvertData scanRspData;

bool notifyEnabled = false; // client has enabled notifications (CCCD)
bool wasConnected = false;  // used to edge-detect connect/disconnect in loop()

#pragma pack(push, 1)
struct SensorReport {
  float temperature;
  float humidity;
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
  uint8_t classification;
  uint8_t confidenceScore;
};
#pragma pack(pop)

void showIdleLED() {
  pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // green
  pixel.show();
}

void showConnectedLED() {
  pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // blue
  pixel.show();
}

// Fires when a client enables/disables notifications on reportCharacteristic.
// Ameba's BLEDevice has no server-side onConnect/onDisconnect callback like
// ESP32's BLEServerCallbacks, so connect/disconnect LED + advertising-restart
// handling is done by polling BLE.connected(0) in loop() instead (below).
void notifCB(BLECharacteristic* chr, uint8_t connID, uint16_t cccd) {
  notifyEnabled = (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY) != 0;
}

void setup() {
  Serial.begin(115200);
  setupSensors();
  setupModel();

  pixel.begin();
  pixel.setBrightness(50); // 0–255, keep it low so it's not blinding
  showIdleLED();

  reportCharacteristic.setReadProperty(true);
  reportCharacteristic.setReadPermissions(GATT_PERM_READ);
  reportCharacteristic.setNotifyProperty(true);
  reportCharacteristic.setCCCDCallback(notifCB);
  reportCharacteristic.setBufferLen(sizeof(SensorReport));

  enviroService.addCharacteristic(reportCharacteristic);

  advData.addFlags(GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED);
  advData.addCompleteName("EnviroMonitor-Test");
  scanRspData.addCompleteServices(BLEUUID(SERVICE_UUID));

  BLE.init();
  BLE.configAdvert()->setAdvData(advData);
  BLE.configAdvert()->setScanRspData(scanRspData);
  BLE.configServer(1);
  BLE.addService(enviroService);
  BLE.beginPeripheral();

  Serial.println("BLE advertising started...");
  Serial.print("sizeof(SensorReport) = ");
  Serial.println(sizeof(SensorReport));
}

void sendReport(RawSensorData raw, float temp, ClassificationResult result) {
  SensorReport report;
  report.temperature = temp;
  report.humidity = raw.humidity;
  report.co = raw.co;
  report.no2 = raw.no2;
  report.so2 = raw.so2;
  report.nh3 = raw.nh3;
  report.o3 = raw.o3;
  report.h2s = raw.h2s;
  report.co2 = raw.co2;
  report.pm1 = raw.pm1;
  report.pm25 = raw.pm25;
  report.pm10 = raw.pm10;
  report.classification = result.classification;
  report.confidenceScore = result.confidenceScore;

  reportCharacteristic.setData((uint8_t*)&report, sizeof(report));
  if (BLE.connected(0) && notifyEnabled) {
    reportCharacteristic.notify(0);
  }
}

void loop() {
  // Connect/disconnect edge detection -- mirrors the ESP32 code's
  // MyServerCallbacks::onConnect()/onDisconnect(), done here via polling
  // since Ameba's BLEDevice doesn't expose those as callbacks.
  bool isConnected = BLE.connected(0);
  if (isConnected && !wasConnected) {
    Serial.println("Device connected — LED -> blue");
    showConnectedLED();
  }
  if (!isConnected && wasConnected) {
    Serial.println("Device disconnected — LED -> green, restarting advertising");
    showIdleLED();
    BLE.configAdvert()->startAdv();
  }
  wasConnected = isConnected;

  RawSensorData raw = readAllSensors();
  RawSensorData processed = processSignal(raw);
  ExtractedFeatures features = extractFeatures(processed);
  ClassificationResult result = classify(features);

  // Ambient temperature from the DHT22 sensor.
  float realTemp = raw.temperature;

  // Debug output: raw MiCS ADC counts (unchanged from before).
  Serial.print("CO ADC: ");
  Serial.print(raw.co, 0);
  Serial.print(" | NH3 ADC: ");
  Serial.print(raw.nh3, 0);
  Serial.print(" | NO2 ADC: ");
  Serial.println(raw.no2, 0);

  // Debug output: calibrated ppm values + classification actually being sent.
  Serial.print("CO ppm: ");
  Serial.print(processed.co, 2);
  Serial.print(" | NH3 ppm: ");
  Serial.print(processed.nh3, 2);
  Serial.print(" | NO2 ppm: ");
  Serial.print(processed.no2, 2);
  Serial.print(" | class: ");
  Serial.print(result.classification);
  Serial.print(" | confidence: ");
  Serial.println(result.confidenceScore);

  sendReport(processed, realTemp, result);

  delay(2000);
}
