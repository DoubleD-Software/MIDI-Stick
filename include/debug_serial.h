#pragma once

#include <Arduino.h>
#include <USB.h>

class DebugSerial {
 public:
  void begin(unsigned long baud) {
    if (cdc) return;
    cdc = new USBCDC();
    cdc->begin(baud);
  }

  template <typename T>
  void print(const T &value) {
    if (cdc) cdc->print(value);
  }

  template <typename T>
  void println(const T &value) {
    if (cdc) cdc->println(value);
  }

  void println() {
    if (cdc) cdc->println();
  }

 private:
  USBCDC *cdc = nullptr;
};

extern DebugSerial MidiSerial;
