#pragma once

#include <Arduino.h>

struct LedColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

extern const LedColor LED_OFF;
extern const LedColor LED_CALIBRATING;
extern const LedColor LED_OK;
extern const LedColor LED_ERROR;

bool readJoystickBtn();
bool readBootBtn();

void setLed(LedColor color);
void ledOff();
void blinkLed(LedColor color, int count, int onMs, int offMs);
void tickLed(LedColor color, unsigned long periodMs);

void waitJoystickRelease();
void waitBootRelease();
