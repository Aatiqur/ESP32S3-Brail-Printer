/*
 * ESP32 BLE Braille Printer Firmware (Nordic UART Edition)
 *
 * ⚠️ IMPORTANT: This firmware uses Nordic Semiconductor's standard UART service UUIDs
 * to match the Android app's BleManager.java expectations.
 *
 * Features:
 * - BLE GATT Server with Nordic UART service
 * - Receives braille print commands from Android app
 * - Processes braille dot patterns via solenoids
 * - Prints detailed debug info to Serial Monitor
 * - Sends progress updates back to Android app
 *
 * Upload:
 * - Arduino IDE → Select ESP32 board
 * - Tools → Serial Monitor @ 115200 baud
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ============ Configuration ============
#define DEVICE_NAME "EZConnect-Braille01"
#define SERIAL_BAUD 115200

// GPIO pins for solenoid drivers (controlling 8 braille dots)
#define PIN_DOT1 12
#define PIN_DOT2 13
#define PIN_DOT3 14
#define PIN_DOT4 27
#define PIN_DOT5 26
#define PIN_DOT6 25
#define PIN_DOT7 33
#define PIN_DOT8 32

// Timing (in milliseconds)
#define DOT_ACTIVATION_TIME 100  // How long to activate pins
#define CHAR_DELAY 500           // Delay between characters

// ============ NORDIC UART Service UUIDs ============
// MUST match Android BleManager.java exactly!
#define UART_SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Write from Android
#define CHARACTERISTIC_TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Notify to Android

// ============ Global variables ============
BLEServer *pServer = nullptr;
BLEService *pUartService = nullptr;
BLECharacteristic *pRxCharacteristic = nullptr;   // Receive commands (WRITE)
BLECharacteristic *pTxCharacteristic = nullptr;   // Send status (NOTIFY)
bool deviceConnected = false;
std::string receivedCommand = "";

// Solenoid pins array
int solenoidPins[] = {PIN_DOT1, PIN_DOT2, PIN_DOT3, PIN_DOT4,
                       PIN_DOT5, PIN_DOT6, PIN_DOT7, PIN_DOT8};

// ============ BLE Server Callbacks ============
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        deviceConnected = true;
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║  ✓ ANDROID DEVICE CONNECTED            ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.println();
    }

    void onDisconnect(BLEServer *pServer) override {
        deviceConnected = false;
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║  ✗ ANDROID DEVICE DISCONNECTED         ║");
        Serial.println("╚════════════════════════════════════════╝");

        // Restart advertising
        BLEDevice::startAdvertising();
        Serial.println("Waiting for new connection...\n");
    }
};

// ============ RX Characteristic Callbacks (receiving from Android) ============
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.println("\n┌─ COMMAND RECEIVED");
            Serial.print("│ Length: ");
            Serial.print(rxValue.length());
            Serial.print(" bytes\n│ Content: ");
            Serial.println(rxValue.c_str());
            Serial.println("└");

            receivedCommand = rxValue;
            processBrailleCommand(rxValue);
        }
    }
};

// ============ Solenoid Control ============

void activateBrailleDots(uint8_t pattern) {
    // Print binary representation
    Serial.print("  Pattern (binary): ");
    for (int i = 7; i >= 0; i--) {
        Serial.print(bitRead(pattern, i));
    }
    Serial.print(" | Active dots: ");

    // Activate pins
    for (int i = 0; i < 8; i++) {
        if (bitRead(pattern, i)) {
            digitalWrite(solenoidPins[i], HIGH);
            Serial.print(i + 1);
            Serial.print(" ");
        }
    }
    Serial.println();

    // Hold activation
    delay(DOT_ACTIVATION_TIME);

    // Deactivate all
    for (int i = 0; i < 8; i++) {
        digitalWrite(solenoidPins[i], LOW);
    }
}

// Send progress/status to Android via NOTIFY
void sendStatusToAndroid(const std::string &status) {
    if (!deviceConnected || pTxCharacteristic == nullptr) {
        return;
    }

    pTxCharacteristic->setValue(status);
    pTxCharacteristic->notify();

    Serial.print("  ↗ Sent to Android: ");
    Serial.println(status.c_str());
}

// ============ Braille Command Processing ============

void processBrailleCommand(std::string command) {
    Serial.println("\n╔═════════════════════════════════════════╗");
    Serial.println("║  PROCESSING BRAILLE COMMAND             ║");
    Serial.println("╚═════════════════════════════════════════╝");

    // Expected format examples:
    // "CMD:0103070F1F"  - Hex pairs (this is most common from Android)
    // "PRINT:1,2,3,4,5" - Decimal values

    // ========== Format 1: HEX String (CMD:...) ==========
    if (command.find("CMD:") == 0) {
        std::string hexData = command.substr(4);  // Remove "CMD:" prefix

        Serial.print("Format: HEX String\n");
        Serial.print("Hex data: ");
        Serial.println(hexData.c_str());

        int charCount = hexData.length() / 2;
        Serial.print("Chars to print: ");
        Serial.println(charCount);

        if (charCount == 0) {
            Serial.println("ERROR: No valid hex data!");
            return;
        }

        // Process each character
        for (int i = 0; i < charCount; i++) {
            std::string hex = hexData.substr(i * 2, 2);
            uint8_t dotPattern = (uint8_t)strtol(hex.c_str(), nullptr, 16);

            Serial.print("\n  [");
            Serial.print(i + 1);
            Serial.print("/");
            Serial.print(charCount);
            Serial.print("] Hex: 0x");
            Serial.print(hex.c_str());
            Serial.print(" (");
            Serial.print((int)dotPattern);
            Serial.println(")");

            // Activate dots
            activateBrailleDots(dotPattern);

            // Send progress
            std::string progress = "PROG:" + std::to_string(i + 1) + "/" + std::to_string(charCount);
            sendStatusToAndroid(progress);

            delay(CHAR_DELAY);
        }

        // Send completion
        Serial.println("\n✓ Print completed!");
        sendStatusToAndroid("PROG:DONE");
    }

    // ========== Format 2: Decimal String (PRINT:...) ==========
    else if (command.find("PRINT:") == 0) {
        std::string data = command.substr(6);  // Remove "PRINT:" prefix

        Serial.print("Format: Decimal Stream\n");
        Serial.print("Data: ");
        Serial.println(data.c_str());

        std::vector<uint8_t> patterns;
        size_t start = 0, end = 0;

        // Parse comma-separated values
        while ((end = data.find(',', start)) != std::string::npos) {
            std::string token = data.substr(start, end - start);
            patterns.push_back((uint8_t)strtol(token.c_str(), nullptr, 10));
            start = end + 1;
        }
        // Last value
        patterns.push_back((uint8_t)strtol(data.substr(start).c_str(), nullptr, 10));

        Serial.print("Chars to print: ");
        Serial.println(patterns.size());

        // Process each pattern
        for (size_t i = 0; i < patterns.size(); i++) {
            Serial.print("\n  [");
            Serial.print(i + 1);
            Serial.print("/");
            Serial.print(patterns.size());
            Serial.print("] Value: ");
            Serial.println(patterns[i]);

            activateBrailleDots(patterns[i]);

            std::string progress = "PROG:" + std::to_string(i + 1) + "/" + std::to_string(patterns.size());
            sendStatusToAndroid(progress);

            delay(CHAR_DELAY);
        }

        Serial.println("\n✓ Print completed!");
        sendStatusToAndroid("PROG:DONE");
    }

    // Unknown format
    else {
        Serial.println("ERROR: Unknown command format!");
        Serial.println("Expected: CMD:... or PRINT:...");
    }

    Serial.println("╚═════════════════════════════════════════╝\n");
}

// ============ GPIO Setup ============

void setupGPIO() {
    Serial.println("[GPIO] Initializing solenoid pins...");

    for (int i = 0; i < 8; i++) {
        pinMode(solenoidPins[i], OUTPUT);
        digitalWrite(solenoidPins[i], LOW);

        Serial.print("  Pin ");
        Serial.print(solenoidPins[i]);
        Serial.print(" → Dot ");
        Serial.println(i + 1);
    }

    Serial.println("✓ GPIO ready!\n");
}

// ============ BLE Setup ============

void setupBLE() {
    Serial.println("[BLE] Initializing BLE device...");

    // Create BLE device
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create UART Service
    pUartService = pServer->createService(UART_SERVICE_UUID);

    // Create RX Characteristic (receive from Android)
    pRxCharacteristic = pUartService->createCharacteristic(
        CHARACTERISTIC_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    // Create TX Characteristic (send to Android)
    pTxCharacteristic = pUartService->createCharacteristic(
        CHARACTERISTIC_TX_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());
    pTxCharacteristic->setValue("Ready");

    // Start service
    pUartService->start();

    // Setup advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(UART_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    BLEDevice::startAdvertising();

    Serial.println("[BLE] ✓ BLE Server initialized!");
    Serial.print("[BLE] Device Name: ");
    Serial.println(DEVICE_NAME);
    Serial.print("[BLE] UART Service UUID: ");
    Serial.println(UART_SERVICE_UUID);
    Serial.print("[BLE] RX UUID (Write): ");
    Serial.println(CHARACTERISTIC_RX_UUID);
    Serial.print("[BLE] TX UUID (Notify): ");
    Serial.println(CHARACTERISTIC_TX_UUID);
    Serial.println("[BLE] Advertising started...\n");
}

// ============ Setup ============

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("╔═════════════════════════════════════════╗");
    Serial.println("║  ESP32 BLE BRAILLE PRINTER               ║");
    Serial.println("║  Nordic UART Edition v2.0                ║");
    Serial.println("╚═════════════════════════════════════════╝");
    Serial.println();

    setupGPIO();
    setupBLE();

    Serial.println("✓ Setup complete! Ready for connections.\n");
}

// ============ Loop ============

void loop() {
    delay(1000);

    // Optional: print connection status every 15 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 15000) {
        lastStatus = millis();

        Serial.print("[STATUS] Uptime: ");
        Serial.print(millis() / 1000);
        Serial.print("s | Connected: ");
        Serial.println(deviceConnected ? "YES" : "NO");
    }
}

/*
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                       USAGE GUIDE                              ║
 * ╚════════════════════════════════════════════════════════════════╝
 *
 * 1. UPLOAD TO ESP32
 *    - Arduino IDE → Select ESP32 board
 *    - Click Upload
 *    - Wait for "Uploading..." to complete
 *
 * 2. OPEN SERIAL MONITOR
 *    - Tools → Serial Monitor
 *    - Baud rate: 115200
 *    - You should see startup banner
 *
 * 3. ANDROID CONNECTION
 *    - Open Android app → ScanActivity
 *    - Click "Refresh/Scan"
 *    - Find "EZConnect-Braille01"
 *    - Click to connect
 *    - Serial Monitor shows: "✓ ANDROID DEVICE CONNECTED"
 *
 * 4. SEND MESSAGE
 *    - In MainActivity: type message
 *    - Click "Send"
 *    - Watch Serial Monitor for debug output
 *    - Solenoids activate based on braille patterns
 *
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                    TROUBLESHOOTING                             ║
 * ╚════════════════════════════════════════════════════════════════╝
 *
 * Issue: "APP not found in scan"
 *   → Check: Is Serial Monitor showing BLE init?
 *   → Fix: Reset ESP32, restart app, rescan
 *
 * Issue: "Connected but no data"
 *   → Check: Android logcat for BLE errors
 *   → Fix: Verify UUID matches in both files
 *
 * Issue: "Data received but solenoids don't activate"
 *   → Check: Are GPIO pins wired correctly?
 *   → Fix: Use ESP32_Solenoid_Test.ino to test pins
 *
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                    SOLENOID PIN MAP                            ║
 * ╚════════════════════════════════════════════════════════════════╝
 *
 * ESP32 GPIO → Dot Number
 * Pin 12    → Dot 1
 * Pin 13    → Dot 2
 * Pin 14    → Dot 3
 * Pin 27    → Dot 4
 * Pin 26    → Dot 5
 * Pin 25    → Dot 6
 * Pin 33    → Dot 7
 * Pin 32    → Dot 8
 *
 * Layout:     Binary Pattern Example:
 * 1 | 4       01010101 = Dots 1,3,5,7
 * 2 | 5
 * 3 | 6
 * ---
 * 7 | 8
 */

