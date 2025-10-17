# 🖱️ ESP32-S3 USB Mouse Jiggler

A simple USB HID "mouse jiggler" firmware for the **ESP32-S3**, designed to keep your computer “awake” and “online” by subtly moving the mouse cursor at random intervals.  
No drivers required, your board will enumerate as a **USB Mouse** on Windows, macOS, and Linux.

---

## ⚙️ Features

- ✅ **Native USB HID** support (no UART-to-USB bridge required)  
- 🎛️ **Smooth, eased motion** using cosine interpolation (natural micro-movements)  
- 🕒 **Configurable runtime** (default: 8 hours, then LED stays ON)  
- 🔄 **Randomized delays** between jiggle bursts (22 – 42 seconds)  
- 💡 **LED blink** after each jiggle to indicate activity  
- 💻 Appears as: `Dell Laser Mouse MS3220` (spoofed VID/PID)

---

## 🧠 How It Works

Every 20–40 seconds, the ESP32-S3 performs a short mouse movement made up of multiple tiny, smoothly-distributed micro-steps.  
This activity prevents your system from sleeping or marking you as idle in chat apps or remote-work software.

---

## 🧩 Hardware Requirements

- **ESP32-S3 development board** (any model with native USB support)
  - Example: *TTGO TDisplay-S3, ESP32-S3-SuperMini, etc.*
- Optional: onboard or external **status LED** (default: `LED_BUILTIN`)

---

## 🔌 Wiring

No special wiring required — the ESP32-S3’s onboard USB port handles both power and USB HID communication.

If your board does **not** expose native USB pins, use the **USB-capable port** (not the UART serial port, if separate).

---

## 🧰 Arduino Setup

1. **Install ESP32 Board Package**
   - In *Arduino IDE → Preferences*, add this URL:  
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Then go to *Boards Manager* and install **esp32** by Espressif Systems.

2. **Select the Board**
   - Choose your ESP32-S3 board, e.g.  
     `Tools → Board → ESP32 Arduino → ESP32S3 Dev Module`

3. **Recommended Settings**
   - `USB CDC On Boot`: **Disabled** → HID-only (appears purely as a mouse)  
   - `USB Mode`: **Hardware CDC and JTAG** (default is fine)  
   - `Upload Speed`: 921600 or 115200  
   - `Partition Scheme`: Minimal SPIFFS or Default

4. **Upload the sketch**  
   Connect via USB-C or Micro-USB and hit *Upload*.

---

## 🖱️ Behavior

| Action | Description |
|:-------|:-------------|
| LED blinks | Each time a jiggle completes |
| Cursor moves slightly | Every 22 – 42 seconds |
| LED stays ON | After 8 hours (runtime complete) |

The movement is gentle and randomized — it won’t interfere with your work or drag windows around.

---

## 🧮 Customization

You can adjust key parameters near the top of the sketch:

```cpp
static constexpr int   kMoveRangePx   = 100;   // max pixels per axis
static constexpr int   kMinPauseMs    = 22000; // min idle (ms)
static constexpr int   kMaxPauseMs    = 42000; // max idle (ms)
static constexpr float kHoursToRun    = 8.0f;  // run time (hours)
```

Set kHoursToRun to 0 for continuous operation.
