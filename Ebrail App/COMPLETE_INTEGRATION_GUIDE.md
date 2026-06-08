# E-Brail BLE Braille Printer - Complete Integration Guide

## 📋 Overview

This guide walks you through setting up the complete E-Brail system:
- **Android App** (client that sends braille commands)
- **ESP32 Firmware** (receiver that controls solenoid printer)
- **Full end-to-end testing**

---

## 🚀 Quick Start (5 minutes)

### Phase 1: Android App
```bash
1. Open Android project in Android Studio
2. Build and run on Android device
3. Grant Bluetooth & Location permissions (Android 12+)
4. App should start with ScanActivity
```

### Phase 2: ESP32 Setup
```bash
1. Upload ESP32_BLE_Braille_Printer_UART.ino to ESP32
2. Open Serial Monitor @ 115200 baud
3. Should show:
   ╔═════════════════════════════════════════╗
   ║  ESP32 BLE BRAILLE PRINTER               ║
   ║  Nordic UART Edition v2.0                ║
   ╚═════════════════════════════════════════╝
```

### Phase 3: Connection Test
```bash
1. In Android app: Click "Scan/Refresh"
2. Look for "EZConnect-Braille01" in device list
3. Click on it
4. ESP32 Serial Monitor should show:
   ╔════════════════════════════════════════╗
   ║  ✓ ANDROID DEVICE CONNECTED            ║
   ╚════════════════════════════════════════╝
```

---

## 📱 Android App Setup

### 1. Prerequisites
- Android Studio 2022.1+
- Android SDK 31+
- Physical Android device (Android 8+)

### 2. File Structure
```
app/src/main/
├── java/com/example/ebrail/
│   ├── MainActivity.java
│   ├── ScanActivity.java
│   ├── BleManager.java
│   ├── BrailleParser.java
│   ├── Device.java
│   ├── DeviceAdapter.java
│   └── PermissionUtil.java
├── res/layout/
│   ├── activity_main.xml
│   ├── activity_scan.xml
│   └── item_device.xml
└── AndroidManifest.xml
```

### 3. Key UUID in Android (BleManager.java)
```java
// Line 211-213 in BleManager.java
UUID UART_SERVICE_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
UUID CHAR_RX_UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");  // Write
UUID CHAR_TX_UUID = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");  // Notify
```

### 4. Building the App
```bash
# In Android Studio terminal
./gradlew build
./gradlew installDebug

# Or using the Run button in Android Studio
```

### 5. First Run
- App opens with ScanActivity
- Click "Refresh" to scan for BLE devices
- Should see device list updating
- Look for device name starting with "EZConnect-"

---

## 💻 ESP32 Firmware Setup

### 1. Prerequisites
- Arduino IDE 1.8.19+
- ESP32 Board Manager installed
- USB Cable for ESP32

### 2. Arduino IDE Configuration

**Step 1: Add ESP32 Board Manager**
```
File → Preferences
Paste into "Additional Boards Manager URLs":
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

**Step 2: Install ESP32 Core**
```
Tools → Board → Boards Manager
Search: "ESP32"
Install: "esp32 by Espressif Systems" (latest)
```

**Step 3: Board Settings**
```
Tools → Board: "ESP32 Dev Module" (or your variant)
Tools → CPU Frequency: "240 MHz"
Tools → Upload Speed: "115200"
Tools → Port: (select COM port where ESP32 is connected)
```

### 3. Upload the Firmware

**File to use:** `ESP32_BLE_Braille_Printer_UART.ino`

```
1. Open in Arduino IDE
2. Click Upload (⬆️ button)
3. Wait for "Leaving... Hard resetting via RTS pin"
4. ESP32 will boot and start advertising
```

### 4. Verify Upload
```
Tools → Serial Monitor (Ctrl+Shift+M)
Baud: 115200
```

You should see:
```
╔═════════════════════════════════════════╗
║  ESP32 BLE BRAILLE PRINTER               ║
║  Nordic UART Edition v2.0                ║
╚═════════════════════════════════════════╝

[GPIO] Initializing solenoid pins...
  Pin 12 → Dot 1
  Pin 13 → Dot 2
  ...
  Pin 32 → Dot 8
✓ GPIO ready!

[BLE] Initializing BLE device...
[BLE] ✓ BLE Server initialized!
[BLE] Device Name: EZConnect-Braille01
[BLE] UART Service UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
[BLE] RX UUID (Write): 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
[BLE] TX UUID (Notify): 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
```

---

## 🔗 Hardware Connections

### Wiring Diagram
```
ESP32 Board
├── GPIO 12 ──→ Solenoid 1 (Dot 1)
├── GPIO 13 ──→ Solenoid 2 (Dot 2)  
├── GPIO 14 ──→ Solenoid 3 (Dot 3)
├── GPIO 27 ──→ Solenoid 4 (Dot 4)
├── GPIO 26 ──→ Solenoid 5 (Dot 5)
├── GPIO 25 ──→ Solenoid 6 (Dot 6)
├── GPIO 33 ──→ Solenoid 7 (Dot 7)
├── GPIO 32 ──→ Solenoid 8 (Dot 8)
└── GND ──────→ Solenoid Ground Rail
```

### Solenoid Driver Circuit (Simplified)
Each GPIO pin needs a driver circuit (transistor/relay):
```
ESP32 GPIO
    ↓
 [100Ω resistor]
    ↓
 [NPN Transistor Base]
    Collector ──→ Solenoid +12V
    Emitter ───→ GND
    ↓
Solenoid GND
```

---

## 🧪 Testing Steps

### Test 1: Check GPIO Pins
Use: `ESP32_Solenoid_Test.ino`

```
1. Upload ESP32_Solenoid_Test.ino instead
2. Open Serial Monitor
3. Type command: "TEST"
4. You should feel vibration/clicking on solenoids
5. Each solenoid will activate 1 second each
```

**Expected Serial Output:**
```
========================================================
ESP32 SOLENOID TEST UTILITY
========================================================

Pin 12 (Dot 1) -> OUTPUT
Pin 13 (Dot 2) -> OUTPUT
...

========== RUNNING TEST SEQUENCE ==========

Testing individual dots (1 second each):
Dot 1
Activating pattern 0x01 (binary: 00000001) -> Dots: 1

Dot 2
Activating pattern 0x02 (binary: 00000010) -> Dots: 2
```

### Test 2: BLE Connectivity

```
1. Upload ESP32_BLE_Braille_Printer_UART.ino
2. Run Android app
3. Go to ScanActivity
4. Click "Scan/Refresh"
5. Look for "EZConnect-Braille01"
6. Click it
```

**ESP32 Serial Monitor should show:**
```
╔════════════════════════════════════════╗
║  ✓ ANDROID DEVICE CONNECTED            ║
╚════════════════════════════════════════╝
```

### Test 3: Send Test Message

```
1. Navigate to MainActivity
2. Type: "A"
3. Click "Send"
```

**ESP32 Serial Monitor should show:**
```
┌─ COMMAND RECEIVED
│ Length: 5 bytes
│ Content: CMD:01
└

╔═════════════════════════════════════════╗
║  PROCESSING BRAILLE COMMAND             ║
╚═════════════════════════════════════════╝
Format: HEX String
Hex data: 01
Chars to print: 1

  [1/1] Hex: 0x01 (1)
  Pattern (binary): 00000001 | Active dots: 1
  ↗ Sent to Android: PROG:1/1
  ↗ Sent to Android: PROG:DONE

✓ Print completed!
```

**Android MainActivity will show:**
- Progress bar at 100%
- "Print Complete!" message
- Braille preview of the character

### Test 4: Send Longer Message

```
1. Type: "HELLO"
2. Click "Send"
```

**Expect:**
- 5 characters processed (H-E-L-L-O)
- Progress updates: PROG:1/5, PROG:2/5, ... PROG:DONE
- Each solenoid pattern activates in sequence
- Android shows real-time progress

---

## 📊 Data Flow Diagram

```
User Types: "HELLO"
↓
Android BrailleParser
├─ Text → Unicode
├─ Unicode → Braille Cell Patterns [01, 05, 0F, 0F, 1F]
└─ Patterns → Hex String: "01050F0F1F"
↓
wrap in: "CMD:01050F0F1F"
↓
Android BleManager.write()
├─ Convert to bytes
├─ Split into 20-byte BLE chunks (if needed)
└─ Send via BLE Write characteristic
↓
ESP32 RX via BluetoothGattCharacteristic
↓
parseCommand()
├─ Extract hex: "01050F0F1F"
├─ Split: ["01", "05", "0F", "0F", "1F"]
└─ For each hex byte:
    ├─ Convert to pattern: 0x01 = 00000001
    ├─ Activate GPIO pins for active bits
    ├─ Wait 100ms
    ├─ Deactivate all pins
    ├─ Send progress via TX notify: "PROG:i/5"
    └─ Wait 500ms before next
↓
After all:
├─ Send: "PROG:DONE"
└─ Android receives via ReadCharacteristic
↓
Android MainActivity
├─ Receives "PROG:DONE"
├─ Updates UI: hide progress, show completion
└─ User ready for next message
```

---

## 🔍 Serial Monitor Output Reference

### Successful Connection
```
╔════════════════════════════════════════╗
║  ✓ ANDROID DEVICE CONNECTED            ║
╚════════════════════════════════════════╝
```

### Successful Command
```
┌─ COMMAND RECEIVED
│ Length: 5 bytes
│ Content: CMD:01
└

╔═════════════════════════════════════════╗
║  PROCESSING BRAILLE COMMAND             ║
╚═════════════════════════════════════════╝
Format: HEX String
Hex data: 01
Chars to print: 1

  [1/1] Hex: 0x01 (1)
  Pattern (binary): 00000001 | Active dots: 1
  ↗ Sent to Android: PROG:1/1
  ↗ Sent to Android: PROG:DONE

✓ Print completed!
╚═════════════════════════════════════════╝
```

### Error: Command Received
```
✗ Device not connected - commands ignored
```

### Error: Bad Format
```
ERROR: Unknown command format!
Expected: CMD:... or PRINT:...
```

---

## ⚠️ Common Issues & Fixes

### Issue: ESP32 Not Found in Scan
```
Symptoms:
  - ScanActivity shows empty list
  - No "EZConnect-Braille01" device

Fix:
  1. Check ESP32 Serial Monitor for init messages
  2. Verify bluetooth is enabled on Android
  3. Grant Bluetooth permission (Settings)
  4. Restart app
  5. Restart ESP32
```

### Issue: Connected but No Data
```
Symptoms:
  - Android shows "Connected: XX:XX:..."
  - ESP32 shows "CONNECTED"
  - But nothing happens when "Send" is clicked

Fix:
  1. Check Android logcat for BLE errors
  2. Verify UUID matches (see Key UUIDs section)
  3. Check BleManager.java line 211-213
  4. Check ESP32 code UART UUIDs match
  5. Restart both devices
```

### Issue: Solenoids Don't Activate
```
Symptoms:
  - Data received (Serial shows PROG notifications)
  - But no physical solenoid movement

Fix:
  1. Check GPIO wiring (pins 12-14, 25-27, 32-33)
  2. Test with ESP32_Solenoid_Test.ino
  3. Check solenoid power supply
  4. Use multimeter to check pin voltage
  5. Check transistor/relay circuit
```

### Issue: Random Connection Drops
```
Symptoms:
  - Connected then disconnected suddenly
  - BLE seems unstable

Fix:
  1. Keep esp32 and phone close (<1m)
  2. Reduce WiFi interference (turn off WiFi if not needed)
  3. Update ESP32 board manager to latest version
  4. Reduce BLE scan interval in Android
```

---

## 🎯 Advanced Configuration

### Adjusting Solenoid Timing

In `ESP32_BLE_Braille_Printer_UART.ino`, line 19-22:

```cpp
#define DOT_ACTIVATION_TIME 100  // Solenoid hold time (ms)
#define CHAR_DELAY 500           // Time between characters (ms)
```

- **Faster printing**: Reduce both values (e.g., 50, 200)
- **Slower printing**: Increase values (e.g., 150, 1000)
- **Test different values** to find what works for your solenoids

### Adding Custom Braille Mappings

In `BrailleParser.java`:
```java
// Add to character mapping table
characterToBraille.put('A', (byte) 0x01);  // Example
```

---

## 📝 Debugging Tips

### 1. Enable Verbose Logging
In Arduino IDE:
```
Tools → Core Debug Level → Verbose
```

### 2. Check BLE Events in Logcat
In Android Studio:
```
Logcat filter: "BleManager"
```

### 3. Use Logic Analyzer
Connect logic analyzer to GPIO pins to verify timing:
- CH1: GPIO 12 (Dot 1)
- CH2: GPIO 13 (Dot 2)
- etc.

### 4. Test Individual Pins
```bash
# Use ESP32_Solenoid_Test.ino
# Type in Serial Monitor: "1" to activate Dot 1
# Type: "FF" to activate all dots
```

---

## 📚 Files Reference

| File | Purpose |
|------|---------|
| `MainActivity.java` | Main UI, messaging, braille preview |
| `ScanActivity.java` | BLE device scanning |
| `BleManager.java` | BLE client (GATT) communication |
| `BrailleParser.java` | Text to braille conversion |
| `ESP32_BLE_Braille_Printer_UART.ino` | **Use this** - Main ESP32 firmware |
| `ESP32_Solenoid_Test.ino` | GPIO/solenoid testing utility |

---

## ✅ Success Checklist

- [ ] Android app builds and runs without errors
- [ ] ESP32 firmware uploads successfully
- [ ] Serial Monitor shows startup banner
- [ ] Android app scans and finds "EZConnect-Braille01"
- [ ] Connection message appears in Serial Monitor
- [ ] Can type in MainActivity without crashes
- [ ] Send button triggers command transmission
- [ ] Serial Monitor shows PROG_XX updates
- [ ] Solenoids activate (visible/audible clicks)
- [ ] Progress updates in Android UI
- [ ] "Print Complete" message appears

Once all are checked, your E-Brail system is ready!

---

## 🆘 Support

If you encounter issues:

1. **Check Serial Monitor** - Most issues are visible here
2. **Verify UUIDs** - Most connection issues are UUID mismatch
3. **Test GPIO** - Use ESP32_Solenoid_Test.ino
4. **Check Logcat** - Android errors appear here
5. **Reduce Distance** - Move phone closer to ESP32

---

## 📄 License

E-Brail Project - Open Source for Educational Purposes

Good luck! 🎉

