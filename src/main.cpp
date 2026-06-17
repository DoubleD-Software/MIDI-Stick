#include <Arduino.h>
#include <EEPROM.h>
#include <USB.h>
#include <esp32-hal-tinyusb.h>

#include "board.h"
#include "calibration.h"
#include "config.h"
#include "debug_serial.h"

DebugSerial MidiSerial;

CalibrationData cal;

int lastBtnRaw = HIGH;
int btnStable = HIGH;
unsigned long lastDebounceAt = 0;
unsigned long lastSendAt = 0;
unsigned long lastCenterSendAt = 0;
unsigned long bootHoldStartedAt = 0;

uint16_t midiLoadDescriptor(uint8_t *dst, uint8_t *itf) {
  uint8_t strIndex = tinyusb_add_string_descriptor("DoubleD MIDI Joystick");
  uint8_t epOut = tinyusb_get_free_out_endpoint();
  uint8_t epIn = tinyusb_get_free_in_endpoint();
  TU_VERIFY(epOut != 0);
  TU_VERIFY(epIn != 0);

  uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
    TUD_MIDI_DESCRIPTOR(*itf, strIndex, epOut, (uint8_t)(0x80 | epIn), 64)
  };
  *itf += 2;
  memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
  return TUD_MIDI_DESC_LEN;
}

void beginMidiInterface() {
  tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, midiLoadDescriptor);
  USB.manufacturerName("DoubleD Software");
  USB.productName("DoubleD MIDI Joystick");
  USB.begin();
}

uint8_t midiChannel(uint8_t channel) {
  if (channel < 1) return 0;
  if (channel > 16) return 15;
  return channel - 1;
}

void midiPacket(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2) {
  uint8_t packet[4] = {cin, status, data1, data2};
  tud_midi_packet_write(packet);
}

void midiControlChange(uint8_t cc, uint8_t value, uint8_t channel) {
  midiPacket(MIDI_CIN_CONTROL_CHANGE, (uint8_t)(0xB0 | midiChannel(channel)), cc, value);
}

void midiNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
  midiPacket(MIDI_CIN_NOTE_ON, (uint8_t)(0x90 | midiChannel(channel)), note, velocity);
}

void midiNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
  midiPacket(MIDI_CIN_NOTE_OFF, (uint8_t)(0x80 | midiChannel(channel)), note, velocity);
}

bool updateBootHold(unsigned long &startedAt) {
  if (!readBootBtn()) {
    startedAt = 0;
    return false;
  }

  unsigned long now = millis();
  if (startedAt == 0) {
    startedAt = now;
  }

  tickLed(LED_CALIBRATING, BOOT_WAIT_LED_DT);
  return now - startedAt >= BOOT_HOLD_MS;
}

bool bootHeldForCalibration() {
  return updateBootHold(bootHoldStartedAt);
}

uint8_t mapAsym(int v, int vmin, int vcenter, int vmax, int dz) {
  if (v >= vcenter + dz) {
    int lo = vcenter + dz;
    int hi = vmax;
    if (hi <= lo) return 127;
    float norm = float(v - lo) / float(hi - lo);
    float delta = powf(norm, CURVE_POWER) * 63.0f;
    int cc = 64 + int(delta + 0.5f);
    if (cc > 127) cc = 127;
    return (uint8_t)cc;
  }
  if (v <= vcenter - dz) {
    int lo = vmin;
    int hi = vcenter - dz;
    if (hi <= lo) return 0;
    float norm = float(hi - v) / float(hi - lo);
    float delta = powf(norm, CURVE_POWER) * 63.0f;
    int cc = 64 - int(delta + 0.5f);
    if (cc < 0) cc = 0;
    return (uint8_t)cc;
  }
  return 64;
}

void sendCenterAtRest(uint8_t xCc, uint8_t yCc) {
  unsigned long now = millis();
  if (lastCenterSendAt != 0 && now - lastCenterSendAt < CENTER_REFRESH_MS) return;

  midiControlChange(X_CC, xCc, X_CHANNEL);
  midiControlChange(Y_CC, yCc, Y_CHANNEL);
  lastCenterSendAt = now;
}

void loadStartupCalibration() {
  if (!loadCalibration(cal)) {
    cal = runCalibration();
    return;
  }
  blinkLed(LED_OK, 1, 500, 300);
}

void handleRuntimeCalibration() {
  if (!bootHeldForCalibration()) return;

  blinkLed(LED_OK, 2, 300, 300);
  waitBootRelease();
  cal = runCalibration();
  lastSendAt = 0;
  lastCenterSendAt = 0;
  MidiSerial.println("Calibration updated. MIDI output resumed.");
}

void handleButton() {
  int btnReading = digitalRead(PIN_BTN);
  if (btnReading != lastBtnRaw) {
    lastDebounceAt = millis();
    lastBtnRaw = btnReading;
  }
  if (millis() - lastDebounceAt <= DEBOUNCE_MS || btnReading == btnStable) return;

  btnStable = btnReading;
  if (btnStable == LOW) {
    midiNoteOn(BTN_NOTE, BTN_VELO, BTN_CHANNEL);
  } else {
    midiNoteOff(BTN_NOTE, 0, BTN_CHANNEL);
  }
}

void sendMovingControls(uint8_t xCc, uint8_t yCc) {
  lastCenterSendAt = 0;
  unsigned long now = millis();
  if (now - lastSendAt < REFRESH_MS) return;

  midiControlChange(X_CC, xCc, X_CHANNEL);
  midiControlChange(Y_CC, yCc, Y_CHANNEL);
  lastSendAt = now;
}

void handleJoystick() {
  uint8_t xCc = mapAsym(analogRead(PIN_X), cal.minX, cal.centerX, cal.maxX, cal.deadzone);
  uint8_t yCc = mapAsym(analogRead(PIN_Y), cal.minY, cal.centerY, cal.maxY, cal.deadzone);

  if (xCc == 64 && yCc == 64) {
    sendCenterAtRest(xCc, yCc);
  } else {
    sendMovingControls(xCc, yCc);
  }
}

void setup() {
  setCpuFrequencyMhz(CPU_FREQ_MHZ);

  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  ledOff();

  EEPROM.begin(sizeof(CalibrationData));
  bool serialEnabled = readJoystickBtn();
  if (serialEnabled) {
    MidiSerial.begin(115200);
  }

  beginMidiInterface();

  delay(300);
  MidiSerial.println();
  MidiSerial.println("DoubleD MIDI Joystick starting...");

  loadStartupCalibration();

  lastSendAt = 0;
  lastCenterSendAt = 0;
  ledOff();
  MidiSerial.println("USB MIDI interface started.");
  MidiSerial.println("Hold BOOT for 5 seconds any time to recalibrate.");
}

void loop() {
  handleRuntimeCalibration();
  handleButton();
  handleJoystick();
  delay(LOOP_DELAY_MS);
}
