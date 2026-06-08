/*
 * ESP32 BLE Braille Printer Firmware
 *
 * Features:
 * - BLE GATT Server to receive braille print commands from Android app
 * - Processes braille dot patterns
 * - Controls solenoid drivers for braille pin activation
 * - Prints detailed debug info to Serial Monitor
 * - Sends progress updates back to Android app
 *
 * Setup:
 * - Upload to ESP32 using Arduino IDE
 * - Open Serial Monitor at 115200 baud
 * - Check BLE device pairing in Android app
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
#define LINE_DELAY 1000          // Delay between lines

// ============ UUIDs ============
#define SERVICE_UUID           "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_READ_UUID  "87654321-4321-4321-4321-210987654321"  // For status/progress
#define CHARACTERISTIC_WRITE_UUID "abcdefab-cdef-abcd-ef12-345678901234"  // For receiving commands

// ============ Global variables ============
BLEServer *pServer = nullptr;
BLEService *pService = nullptr;
BLECharacteristic *pReadCharacteristic = nullptr;
BLECharacteristic *pWriteCharacteristic = nullptr;
bool deviceConnected = false;
String receivedCommand = "";

// Solenoid pins array for easy access
int solenoidPins[] = {PIN_DOT1, PIN_DOT2, PIN_DOT3, PIN_DOT4,
                       PIN_DOT5, PIN_DOT6, PIN_DOT7, PIN_DOT8};

// ============ BLE Server Callbacks ============
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        deviceConnected = true;
        Serial.println("\n[BLE] ✓ Android device CONNECTED");
        Serial.println("[BLE] Client Address: " + String(BLEDevice::getAddress().toString().c_str()));
    }

    void onDisconnect(BLEServer *pServer) override {
        deviceConnected = false;
        Serial.println("[BLE] ✗ Android device DISCONNECTED");
        Serial.println("[BLE] Waiting for reconnection...\n");

        // Restart advertising
        BLEDevice::startAdvertising();
    }
};

// ============ Characteristic Callbacks ============
class WriteCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        String rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.print("\n[COMMAND RECEIVED] Length: ");
            Serial.print(rxValue.length());
            Serial.print(" bytes | Content: ");
            Serial.println(rxValue);

            receivedCommand = rxValue;
            processBrailleCommand(rxValue);
        }
    }
};

// ============ Solenoid Control Functions ============

// Activate specific dots based on braille pattern
// Pattern: 8-bit number where each bit = dot (1=active, 0=inactive)
void activateBrailleDots(uint8_t pattern) {
    Serial.print("[DOTS] Pattern (binary): ");

    // Print binary representation
    for (int i = 7; i >= 0; i--) {
        Serial.print(bitRead(pattern, i));
    }
    Serial.print(" | Activating dots: ");

    // Activate pins based on pattern
    for (int i = 0; i < 8; i++) {
        if (bitRead(pattern, i)) {
            digitalWrite(solenoidPins[i], HIGH);
            Serial.print((i + 1));
            Serial.print(" ");
        }
    }
    Serial.println();

    // Keep activated for DOT_ACTIVATION_TIME
    delay(DOT_ACTIVATION_TIME);

    // Deactivate all
    for (int i = 0; i < 8; i++) {
        digitalWrite(solenoidPins[i], LOW);
    }
}

// Send status update back to Android app
void sendProgressUpdate(int current, int total, bool done = false) {
    if (!deviceConnected || pReadCharacteristic == nullptr) return;

    String status;
    if (done) {
        status = "PROG:DONE";
    } else {
        status = "PROG:" + String(current) + "/" + String(total);
    }

    pReadCharacteristic->setValue((uint8_t*)status.c_str(), status.length());
    pReadCharacteristic->notify();

    Serial.print("[NOTIFY] Sent to Android: ");
    Serial.println(status);
}

// ============ Braille Command Processing ============

void processBrailleCommand(String command) {
    Serial.println("\n========== PROCESSING BRAILLE COMMAND ==========");

    // Expected format: "CMD:<hex_data>"
    if (!command.startsWith("CMD:")) {
        Serial.println("[ERROR] Invalid command format. Expected: CMD:<hex_data>");
        return;
    }

    String hexData = command.substring(4);  // Remove "CMD:" prefix

    Serial.print("[DATA] Hex string length: ");
    Serial.println(hexData.length());
    Serial.print("[DATA] Hex string: ");
    Serial.println(hexData);

    // Parse hex data into byte array
    int charCount = hexData.length() / 2;
    Serial.print("[INFO] Number of characters to print: ");
    Serial.println(charCount);

    if (charCount == 0) {
        Serial.println("[ERROR] No valid data to process");
        return;
    }

    // Process each character
    for (int i = 0; i < charCount; i++) {
        String hex = hexData.substring(i * 2, (i + 1) * 2);
        uint8_t dotPattern = strtol(hex.c_str(), nullptr, 16);

        Serial.print("[CHAR ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(charCount);
        Serial.print("] Hex: ");
        Serial.print(hex);
        Serial.print(" | ");

        // Activate the dots
        activateBrailleDots(dotPattern);

        // Send progress update
        sendProgressUpdate(i + 1, charCount, false);

        // Delay between characters
        delay(CHAR_DELAY);
    }

    // Send completion signal
    Serial.println("\n[SUCCESS] Braille print completed!");
    sendProgressUpdate(charCount, charCount, true);

    Serial.println("============================================\n");
}

// ============ Alternative Command Format: Direct Byte Stream ============

void processBrailleCommandStream(String command) {
    // Format: "PRINT:<byte1>,<byte2>,<byte3>..."

    if (!command.startsWith("PRINT:")) {
        return;
    }

    String data = command.substring(6);  // Remove "PRINT:" prefix
    int charCount = 0;
    int lastIndex = 0;

    Serial.println("\n========== PROCESSING PRINT COMMAND ==========");

    // Count commas to determine total characters
    for (int i = 0; i < data.length(); i++) {
        if (data[i] == ',') charCount++;
    }
    charCount++;  // Add 1 for last character

    Serial.print("[INFO] Character count: ");
    Serial.println(charCount);

    int charIndex = 1;
    for (int i = 0; i <= data.length(); i++) {
        if (data[i] == ',' || i == data.length()) {
            String byteStr = data.substring(lastIndex, i);

            if (byteStr.length() > 0) {
                uint8_t dotPattern = (uint8_t)strtol(byteStr.c_str(), nullptr, 10);

                Serial.print("[CHAR ");
                Serial.print(charIndex);
                Serial.print("/");
                Serial.print(charCount);
                Serial.print("] Decimal: ");
                Serial.print(dotPattern);
                Serial.print(" | ");

                activateBrailleDots(dotPattern);
                sendProgressUpdate(charIndex, charCount, false);

                delay(CHAR_DELAY);
                charIndex++;
            }

            lastIndex = i + 1;
        }
    }

    Serial.println("[SUCCESS] Print completed!");
    sendProgressUpdate(charCount, charCount, true);
    Serial.println("============================================\n");
}

// ============ Setup ============

void setupGPIO() {
    Serial.println("[GPIO] Initializing solenoid pins...");

    for (int i = 0; i < 8; i++) {
        pinMode(solenoidPins[i], OUTPUT);
        digitalWrite(solenoidPins[i], LOW);
        Serial.print("[GPIO] Pin ");
        Serial.print(solenoidPins[i]);
        Serial.println(" set as OUTPUT (LOW)");
    }

    Serial.println("[GPIO] All solenoid pins ready!\n");
}

void setupBLE() {
    Serial.println("[BLE] Initializing BLE...");

    // Create BLE device
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create service
    pService = pServer->createService(SERVICE_UUID);

    // Create READ characteristic (for status updates)
    pReadCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_READ_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pReadCharacteristic->addDescriptor(new BLE2902());
    pReadCharacteristic->setValue("Ready");

    // Create WRITE characteristic (for receiving commands)
    pWriteCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pWriteCharacteristic->setCallbacks(new WriteCharacteristicCallbacks());

    // Start service
    pService->start();

    // Setup advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("[BLE] ✓ BLE Server initialized and advertising...");
    Serial.print("[BLE] Device Name: ");
    Serial.println(DEVICE_NAME);
    Serial.print("[BLE] Service UUID: ");
    Serial.println(SERVICE_UUID);
    Serial.print("[BLE] Write Characteristic UUID: ");
    Serial.println(CHARACTERISTIC_WRITE_UUID);
    Serial.print("[BLE] Read Characteristic UUID: ");
    Serial.println(CHARACTERISTIC_READ_UUID);
    Serial.println("[BLE] Waiting for Android connection...\n");
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    Serial.println("\n\n");
    Serial.println("===============================================");
    Serial.println("  ESP32 BLE BRAILLE PRINTER FIRMWARE v1.0");
    Serial.println("===============================================");
    Serial.print("Startup Time: ");
    Serial.print(millis());
    Serial.println(" ms\n");

    setupGPIO();
    setupBLE();

    Serial.println("Setup complete! Ready to receive braille commands.\n");
}

// ============ Loop ============

void loop() {
    // Keep-alive: monitor connection status
    delay(1000);

    // Optional: Monitor connection status periodically
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 10000) {  // Print every 10 seconds
        lastStatusPrint = millis();

        Serial.print("[STATUS] Time: ");
        Serial.print(millis() / 1000);
        Serial.print("s | Connected: ");
        Serial.println(deviceConnected ? "YES" : "NO");
    }
}

/*
 * ============ USAGE GUIDE ============
 *
 * 1. Upload this code to ESP32 using Arduino IDE
 * 2. Open Serial Monitor (115200 baud)
 * 3. You should see:
 *    - GPIO initialization
 *    - BLE Server initialization
 *    - "Waiting for Android connection..." message
 *
 * 4. From Android app (using BLE):
 *    Send commands in format: "CMD:<HEX_DATA>"
 *    Example: "CMD:01020304050607080910"
 *    Each 2 hex characters = 1 braille character
 *
 * 5. ESP32 will:
 *    - Print received command info
 *    - Activate solenoids for each dot pattern
 *    - Print progress to Serial
 *    - Send progress updates back to Android
 *
 * ============ SOLENOID PIN LAYOUT ============
 * Pin 12 -> Dot 1
 * Pin 13 -> Dot 2
 * Pin 14 -> Dot 3
 * Pin 27 -> Dot 4
 * Pin 26 -> Dot 5
 * Pin 25 -> Dot 6
 * Pin 33 -> Dot 7
 * Pin 32 -> Dot 8
 *
 * Dot layout:
 * 1 | 4
 * 2 | 5
 * 3 | 6
 *  ---
 * 7 | 8
 *
 */

