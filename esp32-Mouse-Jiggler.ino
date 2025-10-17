/*
  ESP32-S3 “Mouse Jiggler” — HID over native USB (TinyUSB via Arduino-ESP32 3.x)

  What this sketch does:
  • Enumerates as a USB mouse and performs a short eased mouse movement every
    ~22–42 seconds for up to `kHoursToRun`.
  • LED blinks after each jiggle burst; stays ON once time is up.

  Board setup (Arduino IDE, ESP32 core 3.x):
  • Select your ESP32-S3 board (e.g., “ESP32S3 Dev Module”).
  • USB CDC On Boot: Disabled  (for clean HID-only “USB Mouse” name)
      – or Enabled if you want a serial console (it becomes a composite device).

  VID/PID note:
  • usbdev.VID()/PID() may not override on all core versions at runtime.
    If you need an exact VID/PID, you might have to hard-code it in the core’s USB.cpp.
*/

#include <Arduino.h>
#include "USB.h"
#include "USBHIDMouse.h"

// ---------- USB device ----------
ESPUSB usbdev;
USBHIDMouse Mouse;

// ---------- Tunables ----------
static constexpr int   kMoveRangePx   = 100;   // |dx|, |dy| upper bound (pixels)
static constexpr int   kMinPauseMs    = 22000; // idle window between bursts
static constexpr int   kMaxPauseMs    = 42000;
static constexpr int   kMinSteps      = 20;    // eased micro-steps per burst
static constexpr int   kMaxSteps      = 30;
static constexpr int   kStepInterval  = 5;     // ms between micro-steps
static constexpr float kHoursToRun    = 8.0f;  // after this, LED stays ON

// ---------- LED pin (fallback if LED_BUILTIN isn't defined on your S3 board) ----------
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Many S3 dev boards have a simple GPIO LED on 2; adjust if needed
#endif
static constexpr int kLedPin = LED_BUILTIN;

// ---------- State machine ----------
enum class JiggleState : uint8_t { IDLE, MOVING };

struct MovementSession {
  int  totalDx = 0;
  int  totalDy = 0;
  int  steps   = 0;

  int  prevX   = 0;
  int  prevY   = 0;
  int  i       = 0;

  unsigned long lastStepMs = 0;
};

static JiggleState     gState        = JiggleState::IDLE;
static MovementSession gSession;
static unsigned long   gStartMs      = 0;
static unsigned long   gNextActionMs = 0;

// ---------- Helpers ----------
static float easeCosine01(float t) {
  // 0.5 * (1 - cos(pi * t))
  return 0.5f * (1.0f - cosf(PI * t));
}

static void seedRng() {
#if defined(ESP_PLATFORM)
  randomSeed(esp_random());
#else
  randomSeed(analogRead(A0));
#endif
}

static void scheduleNextIdleWindow() {
  gNextActionMs = millis() + (unsigned long)random(kMinPauseMs, kMaxPauseMs);
}

static void beginSession() {
  gSession.totalDx = random(-kMoveRangePx, kMoveRangePx + 1);
  gSession.totalDy = random(-kMoveRangePx, kMoveRangePx + 1);
  gSession.steps   = random(kMinSteps, kMaxSteps + 1);

  gSession.prevX = 0;
  gSession.prevY = 0;
  gSession.i     = 0;
  gSession.lastStepMs = 0;

  gState = JiggleState::MOVING;
}

static bool stepSessionIfDue() {
  const unsigned long now = millis();

  if (gSession.i == 0) {
    gSession.lastStepMs = now; // start immediately
  }

  if (now - gSession.lastStepMs < (unsigned long)kStepInterval) {
    return true; // not time yet
  }

  gSession.lastStepMs = now;

  const int nextIndex = gSession.i + 1;
  const float t       = float(nextIndex) / float(gSession.steps);
  const float e       = easeCosine01(t);

  const int xPos = lroundf(gSession.totalDx * e);
  const int yPos = lroundf(gSession.totalDy * e);

  const int dx = xPos - gSession.prevX;
  const int dy = yPos - gSession.prevY;

  if (dx != 0 || dy != 0) {
    Mouse.move(dx, dy, 0, 0);
  }

  gSession.prevX = xPos;
  gSession.prevY = yPos;
  gSession.i     = nextIndex;

  if (gSession.i >= gSession.steps) {
    // Burst finished — quick LED blink
    digitalWrite(kLedPin, HIGH);
    delay(100);
    digitalWrite(kLedPin, LOW);

    gState = JiggleState::IDLE;
    scheduleNextIdleWindow();
    return false;
  }

  return true;
}

// ---------- Arduino lifecycle ----------
void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  // (Optional) Spoof as a Dell Laser Mouse MS3220
  usbdev.VID(0x413C); // May require core edit to truly take effect
  usbdev.PID(0x250E);
  usbdev.productName("Dell Laser Mouse MS3220");
  usbdev.manufacturerName("Dell");

  usbdev.begin();   // bring up TinyUSB device stack
  Mouse.begin();    // start HID mouse

  seedRng();

  gStartMs = millis();
  scheduleNextIdleWindow();
}

void loop() {
  const unsigned long elapsedMs = millis() - gStartMs;
  const unsigned long limitMs   = (unsigned long)(kHoursToRun * 60.0f * 60.0f * 1000.0f);

  if (elapsedMs >= limitMs) {
    digitalWrite(kLedPin, HIGH); // done — hold LED ON
    delay(1000);
    return;
  }

  switch (gState) {
    case JiggleState::IDLE:
      if (millis() >= gNextActionMs) {
        beginSession();
      } else {
        delay(5);
      }
      break;

    case JiggleState::MOVING:
      stepSessionIfDue();
      delay(1);
      break;
  }
}
