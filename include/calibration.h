#pragma once

#include <Arduino.h>

struct CalibrationData {
  uint32_t magic;
  int16_t centerX;
  int16_t centerY;
  int16_t deadzone;
  int16_t minX;
  int16_t maxX;
  int16_t minY;
  int16_t maxY;
};

bool loadCalibration(CalibrationData &data);
CalibrationData runCalibration();
