#include "feature_extraction.h"

// Signature unchanged — still called the same way from AMB8.ino.
// Internals simplified: classification now depends only on the three
// calibrated gas concentrations, so we just carry them straight through
// from the (already-calibrated, in ppm) processed data.
ExtractedFeatures extractFeatures(RawSensorData processed) {
  ExtractedFeatures features;

  features.co  = processed.co;
  features.nh3 = processed.nh3;
  features.no2 = processed.no2;

  return features;
}
