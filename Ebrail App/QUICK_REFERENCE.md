# E-Brail System - Quick Reference Guide

## Command Flow: Android → ESP32 → Braille Printer

```
Android App (MainActivity)
    ↓
User types: "Hello"
    ↓
BrailleParser.translateToBrailleCells("Hello")
    Returns: [[01], [03], [07], [0F], [1F]]  (byte patterns)
    ↓
BrailleParser.translateToCommands(cells)
    Returns: "CMD:0103070F1F"
    ↓
BleManager.write(utf8_bytes)
    Sends via BLE to ESP32
    ↓
ESP32 (BLE GATT Server)
    ↓
WriteCharacteristicCallbacks.onWrite()
    Receives: "CMD:0103070F1F"
    ↓
processBrailleCommand(command)
    Parses hex: [01, 03, 07, 0F, 1F]
    ↓
For each byte (pattern):
    activateBrailleDots(pattern)
        Activates GPIO pins based on bit pattern
        Prints to Serial Monitor
        ↓
        sendProgressUpdate(1, 5, false)  // 1/5 characters done
    ↓
Serial Monitor Output:
    [DOTS] Pattern (binary): 00000001 | Activating dots: 1
    [NOTIFY] Sent to Android: PROG:1/5
    ↓
After all characters:
    sendProgressUpdate(5, 5, true)  // Done!
    ↓
Android receives notification on ReadCharacteristic
    Updates progress bar in MainActivity
```

---

## Hex Pattern to Dot Mapping

Each hex byte = 8 dots

### Binary Position to Dot Number
```
Binary bit position: 7 6 5 4 3 2 1 0
Dot number:         8 7 6 5 4 3 2 1
```

### Examples
| Hex | Binary | Active Dots | Used For |
|-----|--------|-------------|----------|
| `00` | 00000000 | None | Space/pause |
| `01` | 00000001 | 1 | Letter 'A' |
| `03` | 00000011 | 1,2 | Letter 'B' |
| `07` | 00000111 | 1,2,3 | Letter 'C' |
| `0F` | 00001111 | 1,2,3,4 | Letter 'D' |
| `1F` | 00011111 | 1,2,3,4,5 | Letter 'E' |
| `3F` | 00111111 | 1,2,3,4,5,6 | Comma |
| `FF` | 11111111 | 1-8 (All) | Dense character |

---

## Serial Monitor Debug Output Examples

### Startup
```
===============================================
  ESP32 BLE BRAILLE PRINTER FIRMWARE v1.0
===============================================
[GPIO] Initializing solenoid pins...
[GPIO] Pin 12 set as OUTPUT (LOW)
[GPIO] Pin 13 set as OUTPUT (LOW)
...
[BLE] ✓ BLE Server initialized and advertising...
[BLE] Device Name: EZConnect-Braille01
[BLE] Service UUID: 12345678-1234-1234-1234-123456789012
```

### Android Connection
```
[BLE] ✓ Android device CONNECTED
[BLE] Client Address: A4:C1:38:XX:XX:XX
```

### Command Reception
```
[COMMAND RECEIVED] Length: 10 bytes | Content: CMD:0103070F1F
```

### Command Processing
```
========== PROCESSING BRAILLE COMMAND ==========
[DATA] Hex string: 0103070F1F
[INFO] Number of characters to print: 5
[CHAR 1/5] Hex: 01 | [DOTS] Pattern (binary): 00000001 | Activating dots: 1
[NOTIFY] Sent to Android: PROG:1/5
[CHAR 2/5] Hex: 03 | [DOTS] Pattern (binary): 00000011 | Activating dots: 1 2
[NOTIFY] Sent to Android: PROG:2/5
[CHAR 3/5] Hex: 07 | [DOTS] Pattern (binary): 00000111 | Activating dots: 1 2 3
[NOTIFY] Sent to Android: PROG:3/5
[CHAR 4/5] Hex: 0F | [DOTS] Pattern (binary): 00001111 | Activating dots: 1 2 3 4
[NOTIFY] Sent to Android: PROG:4/5
[CHAR 5/5] Hex: 1F | [DOTS] Pattern (binary): 00011111 | Activating dots: 1 2 3 4 5
[NOTIFY] Sent to Android: PROG:5/5
[SUCCESS] Braille print completed!
[NOTIFY] Sent to Android: PROG:DONE
```

---

## Key Files

### Android App
- `MainActivity.java` - Main UI, message input, progress display
- `ScanActivity.java` - BLE device scanning
- `BleManager.java` - BLE communication (GATT client)
- `BrailleParser.java` - Text-to-braille conversion
- `DeviceAdapter.java` - Device list UI

### ESP32 Firmware
- `ESP32_BLE_Braille_Printer.ino` - Main firmware (BLE server)
- `ESP32_Solenoid_Test.ino` - Testing utility

### Resources
- `activity_main.xml` - MainActivity layout
- `activity_scan.xml` - ScanActivity layout
- `item_device.xml` - Device card layout

---

## UUID Configuration

These MUST match between Android and ESP32:

```cpp
// In Android (BleManager.java)
private static final String SERVICE_UUID = "12345678-1234-1234-1234-123456789012";
private static final String WRITE_CHAR_UUID = "abcdefab-cdef-abcd-ef12-345678901234";
private static final String READ_CHAR_UUID = "87654321-4321-4321-4321-210987654321";

// In ESP32 (.ino file)
#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_WRITE_UUID "abcdefab-cdef-abcd-ef12-345678901234"
#define CHARACTERISTIC_READ_UUID "87654321-4321-4321-4321-210987654321"
```

**If they don't match, the app won't find the characteristics!**

---

## Testing Checklist

### Hardware
- [ ] All 8 solenoid pins wired correctly (GPIO 12-14, 25-27, 32-33)
- [ ] Solenoid drivers receiving power
- [ ] GND connected between ESP32 and solenoid circuit

### ESP32
- [ ] Upload `ESP32_BLE_Braille_Printer.ino`
- [ ] Serial Monitor shows startup messages
- [ ] Device name visible as "EZConnect-Braille01"
- [ ] BLE advertising active

### Android
- [ ] Permissions granted (BLUETOOTH, LOCATION)
- [ ] ScanActivity successfully finds "EZConnect-Braille01"
- [ ] Connect button works (no exceptions in logcat)
- [ ] MainActivity shows "Connected: [MAC]"

### Full Flow
- [ ] Type message in Android app
- [ ] Click Send
- [ ] ESP32 Serial Monitor shows command received
- [ ] Solenoids activate (check pins with multimeter)
- [ ] Progress updates in Android UI
- [ ] Message "Print Complete!" appears after

---

## Common BLE Characteristics

- **WRITE**: Android → ESP32 (receive commands)
- **READ + NOTIFY**: ESP32 → Android (send status/progress)

Write Characteristic:
- Properties: WRITE, WRITE_NR
- Purpose: Receive braille commands from Android

Read Characteristic:
- Properties: READ, NOTIFY
- Purpose: Send progress updates to Android
- Example values: "PROG:1/5", "PROG:DONE", "Ready"

---

## Performance Tuning

If you want to speed up or slow down printing:

```cpp
#define DOT_ACTIVATION_TIME 100  // How long to hold dots (ms)
#define CHAR_DELAY 500           // Time between characters (ms)
#define LINE_DELAY 1000          // Time between lines (ms)
```

**Faster printing**: Reduce these values
**Slower printing**: Increase these values

---

## Troubleshooting Flowchart

```
Problem: App can't find ESP32
├─ Check: Is ESP32 powered on?
├─ Check: Serial Monitor showing BLE init messages?
├─ Check: Device name is "EZConnect-Braille01"?
└─ Fix: Restart ESP32, re-scan in Android

Problem: App connects but no data sent
├─ Check: UUID matches in both codes?
├─ Check: Android app sends "CMD:..." format?
├─ Check: No BLE errors in logcat?
└─ Fix: Verify UUID, check BleManager.java

Problem: Data sent but solenoids don't activate
├─ Check: GPIO pins correct (12,13,14,25,26,27,32,33)?
├─ Check: Solenoid power supply active?
├─ Check: Serial Monitor shows "Activating dots"?
└─ Fix: Check wiring, test with ESP32_Solenoid_Test.ino

Problem: Solenoids activate but print is wrong
├─ Check: Hex patterns in Serial Monitor correct?
├─ Check: BrailleParser output correct?
├─ Check: Dot numbering matches hardware layout?
└─ Fix: Verify braille mapping, re-map if needed
```

---

## Emergency Reset

If the ESP32 gets stuck:

1. Press the **RESET** button on the ESP32 board
2. Watch Serial Monitor (it will restart)
3. Wait for "Waiting for Android connection..." message
4. Reconnect Android app

---

## License & Credits

E-Brail Project - Open Source
For educational and accessibility purposes

