MATTERA — Integrated MiCS-6814 + BLE firmware

WHAT CHANGED
1. Removed simulated CO/NH3/NO2 values from sensors.cpp.
2. Added real 12-bit ESP32-S3 ADC acquisition with 32-sample averaging.
3. CO = GPIO1, NH3 = GPIO2, NO2 = GPIO3.
4. Existing BLE SensorReport layout and UUIDs were preserved so the friend's receiver/UI protocol is not broken.
5. Unconnected sensors now transmit 0 instead of fake values.
6. Added Serial Monitor output for CO/NH3/NO2 ADC values.

IMPORTANT
- CO/NH3/NO2 values are RAW ADC COUNTS (0..4095), NOT ppm.
- Do not label them ppm in the UI yet.
- Use the finalized MiCS-6814 load-resistor/interface before trusting readings.
- The existing ML model is retained only so the original demo pipeline still builds/runs. Its classification is NOT scientifically valid with the new raw ADC inputs because it was not trained on calibrated MATTERA data.
- SHT40 is not yet connected; humidity is 0. ESP32 internal temperature is only a board-temperature diagnostic.

BLE PROTOCOL (UNCHANGED)
Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
Characteristic UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8
Packed SensorReport fields, in order:
float temperature
float humidity
float co
float no2
float so2
float nh3
float o3
float h2s
float co2
float pm1
float pm25
float pm10
uint8_t classification
uint8_t confidenceScore

UI MAPPING
CO column  <- report.co  (ADC)
NH3 column <- report.nh3 (ADC)
NO2 column <- report.no2 (ADC)

NEXT HARDWARE STEP
Finalize the exact CJMCU-6814 resistor/ADC interface and verify each sensor-node voltage with a multimeter. Then integrate SHT40. Only after calibration should ADC fields be converted to Rs/R0 or ppm.
