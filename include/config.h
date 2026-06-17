#pragma once

// Board pin mapping
#define PIN_BTN 8 // Joystick push button, active low
#define PIN_X 10 // Joystick X-axis analog input
#define PIN_Y 9 // Joystick Y-axis analog input
#define PIN_LED 48 // On-board NeoPixel data pin
#define PIN_BOOT 0 // BOOT button, active low

// MIDI mapping
#define BTN_CHANNEL 2 // MIDI channel for joystick button note messages
#define BTN_NOTE 60 // MIDI note sent when the joystick button is pressed
#define BTN_VELO 100 // Note-on velocity for the joystick button
#define X_CHANNEL 1 // MIDI channel for X-axis CC messages
#define Y_CHANNEL 1 // Y-axis
#define X_CC 20 // MIDI CC number for the X-axis
#define Y_CC 21 // Y-axis

// Runtime settings
#define CURVE_POWER 2.0f // Higher values give finer control near center
#define DEBOUNCE_MS 30 // Button debounce time
#define REFRESH_MS 50 // Moving joystick CC refresh interval
#define CENTER_REFRESH_MS 10000 // At-rest center resend interval
#define LOOP_DELAY_MS 10 // Main loop delay
#define CPU_FREQ_MHZ 80 // Lower CPU clock to reduce heat and power use

// BOOT hold recalibration
#define BOOT_HOLD_MS 5000 // Hold BOOT this long to start calibration
#define BOOT_WAIT_LED_DT 1000 // LED blink interval while BOOT is held

// Calibration sampling and output
#define CAL_MAGIC 0x4D4A4341UL // EEPROM marker for valid calibration data
#define N_IDLE 2400 // Idle samples used for center/jitter calibration
#define IDLE_DT 2 // Delay between idle samples
#define SHOW_DT 150 // Minimum delay between live range print updates
#define CAL_SETTLE_MS 1500 // Settle time before idle samples are captured
#define RANGE_EDGE_BUFFER 48 // ADC counts trimmed from captured range edges
