#include "calibration.h"

#include <EEPROM.h>
#include <USB.h>

#include "board.h"
#include "config.h"
#include "debug_serial.h"

struct IdleCapture {
  int centerX;
  int centerY;
  int jitterX;
  int jitterY;
  int deadzoneX;
  int deadzoneY;
  int deadzone;
};

struct RangeCapture {
  int minX;
  int maxX;
  int minY;
  int maxY;
};

static bool calibrationIntegrity(const CalibrationData &data) {
  if (data.magic != CAL_MAGIC) return false;
  if (data.minX < 0 || data.minY < 0 || data.maxX > 4095 || data.maxY > 4095) return false;
  if (data.minX >= data.centerX || data.centerX >= data.maxX) return false;
  if (data.minY >= data.centerY || data.centerY >= data.maxY) return false;
  if (data.deadzone < 1 || data.deadzone > 1024) return false;
  return true;
}

bool loadCalibration(CalibrationData &data) {
  EEPROM.get(0, data);
  return calibrationIntegrity(data);
}

static void saveCalibration(const CalibrationData &data) {
  EEPROM.put(0, data);
  EEPROM.commit();
}

void printCalibrationResults(const CalibrationData &data, const IdleCapture &idle) {
  MidiSerial.println();
  MidiSerial.println("=== RESULTS ===");
  MidiSerial.print("centerX=");
  MidiSerial.print(data.centerX);
  MidiSerial.print("  centerY=");
  MidiSerial.println(data.centerY);
  MidiSerial.print("jitterX=");
  MidiSerial.print(idle.jitterX);
  MidiSerial.print("  jitterY=");
  MidiSerial.println(idle.jitterY);
  MidiSerial.print("deadzoneX=");
  MidiSerial.print(idle.deadzoneX);
  MidiSerial.print("  deadzoneY=");
  MidiSerial.print(idle.deadzoneY);
  MidiSerial.print("  deadzoneStored=");
  MidiSerial.println(data.deadzone);
  MidiSerial.print("minX=");
  MidiSerial.print(data.minX);
  MidiSerial.print("  maxX=");
  MidiSerial.print(data.maxX);
  MidiSerial.print("  minY=");
  MidiSerial.print(data.minY);
  MidiSerial.print("  maxY=");
  MidiSerial.println(data.maxY);
}

IdleCapture captureIdle() {
  MidiSerial.println("Leave the stick untouched. Waiting for it to settle...");
  unsigned long settleStartedAt = millis();
  while (millis() - settleStartedAt < CAL_SETTLE_MS) {
    tickLed(LED_CALIBRATING, 750);
    delay(5);
  }
  MidiSerial.println("Capturing idle jitter automatically...");

  long sumX = 0;
  long sumY = 0;
  int minX = 4095;
  int maxX = 0;
  int minY = 4095;
  int maxY = 0;
  for (int i = 0; i < N_IDLE; ++i) {
    int x = analogRead(PIN_X);
    int y = analogRead(PIN_Y);
    sumX += x;
    sumY += y;
    if (x < minX) minX = x;
    if (x > maxX) maxX = x;
    if (y < minY) minY = y;
    if (y > maxY) maxY = y;
    tickLed(LED_CALIBRATING, 750);
    delay(IDLE_DT);
  }

  IdleCapture idle = {};
  idle.centerX = (int)((sumX + N_IDLE / 2) / N_IDLE);
  idle.centerY = (int)((sumY + N_IDLE / 2) / N_IDLE);
  idle.jitterX = max(idle.centerX - minX, maxX - idle.centerX);
  idle.jitterY = max(idle.centerY - minY, maxY - idle.centerY);
  idle.deadzoneX = max(8, idle.jitterX * 2);
  idle.deadzoneY = max(8, idle.jitterY * 2);
  idle.deadzone = max(idle.deadzoneX, idle.deadzoneY);
  return idle;
}

void printRangeProgress(const RangeCapture &range) {
  MidiSerial.print("RANGE  X[");
  MidiSerial.print(range.minX);
  MidiSerial.print("..");
  MidiSerial.print(range.maxX);
  MidiSerial.print("]  Y[");
  MidiSerial.print(range.minY);
  MidiSerial.print("..");
  MidiSerial.print(range.maxY);
  MidiSerial.println("]  (press joystick BTN when done)");
}

void updateRange(RangeCapture &range, int x, int y) {
  if (x < range.minX) {
    range.minX = x;
  }
  if (x > range.maxX) {
    range.maxX = x;
  }
  if (y < range.minY) {
    range.minY = y;
  }
  if (y > range.maxY) {
    range.maxY = y;
  }
}

RangeCapture captureRange() {
  ledOff();
  MidiSerial.println("Idle OK. Range capture started.");
  MidiSerial.println("Move the stick to ALL edges, then press joystick BTN when done.");

  RangeCapture range = {4095, 0, 4095, 0};
  RangeCapture lastPrinted = range;
  unsigned long lastPrintAt = 0;

  while (!readJoystickBtn()) {
    updateRange(range, analogRead(PIN_X), analogRead(PIN_Y));
    bool printChanged = range.minX != lastPrinted.minX || range.maxX != lastPrinted.maxX ||
                        range.minY != lastPrinted.minY || range.maxY != lastPrinted.maxY;
    if (printChanged && millis() - lastPrintAt > SHOW_DT) {
      printRangeProgress(range);
      lastPrinted = range;
      lastPrintAt = millis();
    }
    tickLed(LED_CALIBRATING, 750);
    delay(2);
  }
  waitJoystickRelease();
  return range;
}

CalibrationData makeCalibrationData(const IdleCapture &idle, const RangeCapture &range) {
  CalibrationData data = {};
  data.magic = CAL_MAGIC;
  data.centerX = idle.centerX;
  data.centerY = idle.centerY;
  data.deadzone = idle.deadzone;
  data.minX = min(range.minX + RANGE_EDGE_BUFFER, idle.centerX - 1);
  data.maxX = max(range.maxX - RANGE_EDGE_BUFFER, idle.centerX + 1);
  data.minY = min(range.minY + RANGE_EDGE_BUFFER, idle.centerY - 1);
  data.maxY = max(range.maxY - RANGE_EDGE_BUFFER, idle.centerY + 1);
  return data;
}

void saveOrHalt(const CalibrationData &data) {
  if (calibrationIntegrity(data)) {
    saveCalibration(data);
    MidiSerial.println("Calibration saved to EEPROM.");
    blinkLed(LED_OK, 2, 300, 300);
    return;
  }

  MidiSerial.println("Calibration values are invalid. Reset and try again.");
  while (true) {
    blinkLed(LED_ERROR, 1, 500, 1000);
  }
}

CalibrationData runCalibration() {
  MidiSerial.println();
  MidiSerial.println("=== DoubleD Joystick Calibrator ===");

  IdleCapture idle = captureIdle();
  blinkLed(LED_OK, 2, 300, 300);
  RangeCapture range = captureRange();
  CalibrationData data = makeCalibrationData(idle, range);

  printCalibrationResults(data, idle);
  saveOrHalt(data);
  ledOff();
  return data;
}
