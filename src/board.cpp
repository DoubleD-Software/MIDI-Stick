#include "board.h"
#include "config.h"

const LedColor LED_OFF = {0, 0, 0};
const LedColor LED_CALIBRATING = {48, 24, 0};
const LedColor LED_OK = {0, 48, 0};
const LedColor LED_ERROR = {48, 0, 0};

bool readJoystickBtn() {
  return digitalRead(PIN_BTN) == LOW;
}

bool readBootBtn() {
  return digitalRead(PIN_BOOT) == LOW;
}

void setLed(LedColor color) {
  neopixelWrite(PIN_LED, color.r, color.g, color.b);
}

void ledOff() {
  setLed(LED_OFF);
}

void blinkLed(LedColor color, int count, int onMs, int offMs) {
  for (int i = 0; i < count; ++i) {
    setLed(color);
    delay(onMs);
    ledOff();
    delay(offMs);
  }
}

void tickLed(LedColor color, unsigned long periodMs) {
  static unsigned long lastToggleAt = 0;
  static bool state = false;
  static LedColor lastColor = LED_OFF;
  static unsigned long lastPeriodMs = 0;
  unsigned long now = millis();
  if (color.r != lastColor.r || color.g != lastColor.g || color.b != lastColor.b || periodMs != lastPeriodMs) {
    state = false;
    lastToggleAt = now;
    lastColor = color;
    lastPeriodMs = periodMs;
    ledOff();
    return;
  }
  if (now - lastToggleAt >= periodMs) {
    state = !state;
    if (state) {
      setLed(color);
    } else {
      ledOff();
    }
    lastToggleAt = now;
  }
}

void waitJoystickRelease() {
  while (readJoystickBtn()) {
    tickLed(LED_CALIBRATING, 500);
    delay(5);
  }
  delay(30);
}

void waitBootRelease() {
  while (readBootBtn()) {
    ledOff();
    delay(5);
  }
  delay(30);
}
