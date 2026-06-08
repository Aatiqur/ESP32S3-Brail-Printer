# ESP32 BLE Braille Printer - Setup Guide

## Overview
This is the firmware for the ESP32 microcontroller that:
- Acts as a BLE GATT Server
- Receives braille print commands from the Android app
- Controls solenoid drivers to activate braille pins
- Prints debug information to Serial Monitor
- Sends progress updates back to the Android app

---

## Hardware Requirements

### Components
- **ESP32 Development Board** (any variant: DevKit v1, C3, S3, etc.)
- **8 Solenoid Drivers** (or relay modules for pin activation)
- **Power Supply** (suitable for solenoids - typically 12V or 24V)
- **USB Cable** (for uploading firmware)

### Wiring Diagram
```
ESP32 GPIO Pins → Solenoid Driver Inputs (via transistor/relay circuit)

ESP32 Pin 12 ──→ Dot 1 Solenoid
ESP32 Pin 13 ──→ Dot 2 Solenoid
ESP32 Pin 14 ──→ Dot 3 Solenoid
ESP32 Pin 27 ──→ Dot 4 Solenoid
ESP32 Pin 26 ──→ Dot 5 Solenoid
ESP32 Pin 25 ──→ Dot 6 Solenoid
ESP32 Pin 33 ──→ Dot 7 Solenoid
ESP32 Pin 32 ──→ Dot 8 Solenoid

GND ──→ Solenoid Control Circuit GND
```

### Braille Dot Layout
```
1 | 4
2 | 5
3 | 6
---
7 | 8
```

---

## Setup Instructions

### 1. Arduino IDE Configuration

1. Open **Arduino IDE**
2. Go to **File → Preferences**
3. Add this URL to "Additional Boards Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "ESP32" and install "esp32 by Espressif Systems"

### 2. Install Required Libraries

In Arduino IDE, go to **Sketch → Include Library → Manage Libraries**:

- Search for **"BLE"** and install **"BLE Arduino"** by Arduino
  - OR ensure your ESP32 core has BLE support

### 3. Board Settings

**Tools Menu** settings:
```
Board: ESP32 Dev Module (or your ESP32 variant)
Upload Speed: 115200
CPU Frequency: 240 MHz
Core Debug Level: Info (for debugging)
Partition Scheme: Default 4MB with spiffs
```

### 4. Upload the Firmware

1. Connect ESP32 to your computer via USB
2. Open `ESP32_BLE_Braille_Printer.ino` in Arduino IDE
3. Select the correct **COM port** under **Tools → Port**
4. Click **Upload** (⬆️ button)
5. Wait for upload to complete

### 5. Open Serial Monitor

1. In Arduino IDE, go to **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You should see startup messages:
   ```
   ===============================================
     ESP32 BLE BRAILLE PRINTER FIRMWARE v1.0
   ===============================================
   [GPIO] Initializing solenoid pins...
   [GPIO] Pin 12 set as OUTPUT (LOW)
   ...
   [BLE] ✓ BLE Server initialized and advertising...
   [BLE] Device Name: EZConnect-Braille01
   ```

---

## Usage

### From Android App

1. **Open the Android app** and navigate to **ScanActivity**
2. **Scan for devices** - you should see `EZConnect-Braille01`
3. **Click to connect** - the ESP32 will show:
   ```
   [BLE] ✓ Android device CONNECTED
   ```

4. **In MainActivity**, type a message and click **Send**
5. The Android app converts text to braille and sends it to ESP32
6. **Serial Monitor output** will show:
   ```
   [COMMAND RECEIVED] Length: 22 bytes | Content: CMD:010203...
   ========== PROCESSING BRAILLE COMMAND ==========
   [DOTS] Pattern (binary): 00000001 | Activating dots: 1
   [CHAR 1/11] Hex: 01 | ...
   [SUCCESS] Braille print completed!
   ```

---

## Serial Monitor Output Explanation

### Connection Status
```
[BLE] ✓ Android device CONNECTED
[BLE] Client Address: XX:XX:XX:XX:XX:XX
```

### Command Reception
```
[COMMAND RECEIVED] Length: 22 bytes | Content: CMD:010203...
```

### Dot Activation
```
[DOTS] Pattern (binary): 00000011 | Activating dots: 1 2
[CHAR 1/5] Hex: 03 | ...
```

### Progress Updates
```
[NOTIFY] Sent to Android: PROG:1/5
[NOTIFY] Sent to Android: PROG:2/5
[NOTIFY] Sent to Android: PROG:DONE
```

---

## Command Format

### Format 1: Hex String (Recommended)
```
CMD:010203040506070809
```
- Each 2 hex digits = 1 braille character
- Example: `01` = Dot 1 only, `3F` = All dots

### Format 2: Decimal Stream (Alternative)
```
PRINT:1,2,3,4,5,6,7,8,9
```
- Each number (0-255) = 1 braille character dot pattern

---

## Troubleshooting

### Issue: ESP32 not found in Android scan
**Solution:**
1. Check Serial Monitor - should show "Waiting for Android connection..."
2. Verify device name matches (should be `EZConnect-Braille01`)
3. Check Bluetooth permissions on Android (Android 12+)
4. Restart ESP32 and Android Bluetooth

### Issue: No Serial output after upload
**Solution:**
1. Check COM port - make sure it's selected correctly
2. Verify baud rate is 115200
3. Click Serial Monitor **refresh button** (⟳)
4. Press ESP32 reset button

### Issue: Connected but no data received
**Solution:**
1. Verify the Android app sends correct UUID (check BleManager.java)
2. Check UUID in ESP32 code matches Android app
3. Verify Android app sends data in correct format (`CMD:...`)

### Issue: Solenoids not activating
**Solution:**
1. Verify GPIO pins are correctly wired to drivers
2. Check if pins are HIGH in Serial Monitor (should show dots being activated)
3. Verify power supply to solenoid drivers
4. Test pins individually with logic analyzer

---

## Configuration

Edit these constants at the top of the `.ino` file:

```cpp
#define DEVICE_NAME "EZConnect-Braille01"     // BLE device name
#define SERVICE_UUID "12345678-..."           // Must match Android app
#define DOT_ACTIVATION_TIME 100               // How long to activate (ms)
#define CHAR_DELAY 500                         // Delay between characters (ms)
#define LINE_DELAY 1000                        // Delay between lines (ms)
```

---

## API Reference

### Receiving Commands
The `WriteCharacteristicCallbacks` class handles incoming data:
```cpp
onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    // Process command...
}
```

### Sending Updates to Android
```cpp
sendProgressUpdate(int current, int total, bool done = false);
// Sends "PROG:current/total" or "PROG:DONE"
```

### Activating Solenoids
```cpp
activateBrailleDots(uint8_t pattern);
// pattern: 8-bit number where each bit controls a dot
// Example: 0x03 = 00000011 = activate dots 1,2
```

---

## Performance

- **Command Processing**: ~10-50ms per character
- **BLE Latency**: 5-20ms
- **Max throughput**: ~50 characters/sec
- **Memory**: ~50KB program, ~100KB heap

---

## License

This firmware is part of the E-Brail project. Use freely for educational and non-commercial purposes.

---

## Support

For issues or questions:
1. Check Serial Monitor output
2. Enable **Core Debug Level: Verbose** in Arduino IDE
3. Capture the full Serial output and check logs

