#include "ml_model.h"
#include "model_data.h"
#include <ArduTFLite.h>

// ============================================================
// TFLite Micro model — kept initialized (harmless), classify()
// below does not use it. Classification is rule-based, per explicit
// request: it depends ONLY on CO concentration now.
// ============================================================
constexpr int kTensorArenaSize = 4 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

void setupModel() {
  if (!modelInit(air_classifier_tflite, tensorArena, kTensorArenaSize)) {
    Serial.println("Model initialization failed!");
    while (true) delay(1000);
  }
  Serial.println("Model initialized successfully.");
}


// ============================================================
// CO-only classification bands (explicit request):
//   < 6.5 ppm       -> Good
//   6.5 - 15 ppm     -> Poor
//   > 15 ppm         -> Hazardous
//
// No "Moderate" tier is produced — only 3 bands were specified.
// The classification value 1 (Moderate) is left defined below for
// compatibility with the existing 4-level ClassificationResult /
// app UI, but classify() will never output it under this logic.
// NH3 and NO2 are still calibrated and sent (unchanged elsewhere),
// they just no longer influence this classification decision.
// ============================================================

static uint8_t levelForCO(float ppm) {
  if (ppm < 6.5f)  return 0; // Good
  if (ppm <= 15.0f) return 2; // Poor
  return 3;                   // Hazardous
}

static uint8_t confidenceForClass(uint8_t classification) {
  switch (classification) {
    case 0: return 90;
    case 1: return 75; // unused (no Moderate band defined for CO)
    case 2: return 65;
    case 3: return 80;
    default: return 50;
  }
}

ClassificationResult classify(ExtractedFeatures features) {
  ClassificationResult result;

  result.classification  = levelForCO(features.co);
  result.confidenceScore = confidenceForClass(result.classification);

  return result;
}
