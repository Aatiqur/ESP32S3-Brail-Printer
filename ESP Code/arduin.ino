#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <AccelStepper.h>

// --- CNC Shield Pins for ESP32 ---
// Make sure these pins match your actual ESP32 wiring to the CNC shield!
#define X_STEP_PIN 12
#define X_DIR_PIN 14
#define Y_STEP_PIN 27
#define Y_DIR_PIN 26
#define SOLENOID_PIN 25
#define ENABLE_PIN 33   // LOW = enable, HIGH = disable

// --- Stepper Setup ---
AccelStepper stepperX(1, X_STEP_PIN, X_DIR_PIN);
AccelStepper stepperY(1, Y_STEP_PIN, Y_DIR_PIN);

// --- Movement Calibration ---
const int STEP_COUNT_DOT = 15;      // 2.5 mm dot-to-dot
const int STEP_COUNT_CELL_X = 22;   // 3.5 mm cell-to-cell
const int STEP_COUNT_DOT_Y = 15;    // 2.5 mm dot-row spacing
const int STEP_COUNT_LINE_Y = 30;   // 5.0 mm line spacing

// --- Printer State ---
long current_x = 0;
long current_y = 0; 

// --- Solenoid Parameters ---
const int SOLENOID_PULSE = 20;  // ms - Reduced for shorter pulse
const int SOLENOID_GAP   = 20;  // ms - Reduced for quicker response

// --- Command Symbols ---
const char CMD_PRINT = 'P';
const char CMD_DOT_SHIFT_X = 'X';
const char CMD_CELL_SHIFT = 'C';
const char CMD_Y_SHIFT_DOT_ROW = 'Y';
const char CMD_LINE_FEED = 'L';
const char CMD_GO_HOME = 'H';
const char CMD_EJECT = 'E';

// --- Speed Profiles ---
const long FORWARD_MAX_SPEED = 6000.0;    // Max speed for forward movement
const long RETURN_SAFE_SPEED = 1500.0;    // Slower speed for return/homing
const long FORWARD_ACCEL = 2000.0;        // Forward acceleration
const long RETURN_ACCEL = 500.0;          // Return acceleration (safer)

// --- BLE UUIDs ---
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

String bleBuffer = "";
bool commandReady = false;

// --- Function Prototypes ---
void move_stepper(AccelStepper& stepper, long target);
void move_stepper_with_speed(AccelStepper& stepper, long target, float speed, float accel);
void process_command_sequence(String command);
void handle_full_command(String command);

// --- BLE Callbacks ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("⚡ CHUNK RECEIVED [");
        Serial.print(rxValue.length());
        Serial.print(" bytes]: ");
        Serial.println(rxValue);
        
        for (int i = 0; i < rxValue.length(); i++) {
          char c = rxValue[i];
          if (c == '\n') {
            commandReady = true;
          } else {
            bleBuffer += c;
          }
        }
      }
    }
};

#include <WiFi.h> // Added for WiFi control

void setup() {
  Serial.begin(115200);
  
  // Safely wait for USB Serial to connect (for ESP32-S3 native USB)
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
    delay(10);
  }

  Serial.println("\n\n=== STARTING BOOT SEQUENCE ===");
  Serial.println("0. Reducing Power Consumption...");
  setCpuFrequencyMhz(80); // Drop CPU from 240MHz to 80MHz to save massive power
  WiFi.mode(WIFI_OFF);    // Completely disable the WiFi radio (saves ~100mA)

  Serial.println("1. Initializing Pins...");
  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);
  digitalWrite(ENABLE_PIN, HIGH); 

  Serial.println("2. Initializing Motors...");
  stepperX.setMaxSpeed(FORWARD_MAX_SPEED);
  stepperX.setAcceleration(FORWARD_ACCEL);
  stepperY.setMaxSpeed(FORWARD_MAX_SPEED);
  stepperY.setAcceleration(FORWARD_ACCEL);  
  
  Serial.println("3. Initializing BLE Device...");
  BLEDevice::init("eBrail-Printer"); 
  
  Serial.println("4. Creating BLE Server...");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											 BLECharacteristic::PROPERTY_WRITE |
                       BLECharacteristic::PROPERTY_WRITE_NR
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  // --- Optimized Advertising Setup ---
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  // Help with iPhone/Android connection issues by setting preferred parameters
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMaxPreferred(0x12); 
  
  // Give the stack a moment to stabilize before starting
  delay(200);
  BLEDevice::startAdvertising();

  Serial.println("==================================================");
  Serial.println("✅ ESP32 BRAILLE PRINTER READY");
  Serial.println("Waiting for connection from Android App...");
  Serial.println("==================================================");
}

void loop() {
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        BLEDevice::startAdvertising(); // restart advertising using standard method
        Serial.println("❌ Disconnected, restarting advertising...");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
        Serial.println("✅ Device connected successfully!");
    }

    if (commandReady) {
        bleBuffer.trim();
        if (bleBuffer.length() > 0) {
            Serial.println("\n📥 RECEIVED FULL BLE COMMAND:");
            Serial.println(bleBuffer);
            Serial.println("------------------------------------------------");
            handle_full_command(bleBuffer);
        }
        bleBuffer = "";
        commandReady = false;
    }
}

void handle_full_command(String command) {
    digitalWrite(ENABLE_PIN, LOW); // Enable motors

    if (command.startsWith("HOME")) {
        move_stepper_with_speed(stepperX, 0, RETURN_SAFE_SPEED, RETURN_ACCEL);
        delay(100);
        
        current_x = 0;
        current_y = 0;
        stepperX.setCurrentPosition(0);
        stepperY.setCurrentPosition(0);
        
        stepperX.setMaxSpeed(FORWARD_MAX_SPEED);
        stepperX.setAcceleration(FORWARD_ACCEL);
        
        Serial.println("✅ HOME_COMPLETED");
        if(deviceConnected) {
            pTxCharacteristic->setValue("HOME_COMPLETED");
            pTxCharacteristic->notify();
        }
    }
    else if (command.startsWith("EJECT")) {
        Serial.println("⏸️ Ejecting paper...");
        
        move_stepper_with_speed(stepperY, -500, 1000.0, 300.0);
        delay(200);
        
        move_stepper_with_speed(stepperY, 0, 1000.0, 300.0);
        current_y = 0;
        stepperY.setCurrentPosition(0);
        
        Serial.println("✅ PAPER_EJECTED");
        if(deviceConnected) {
            pTxCharacteristic->setValue("PAPER_EJECTED");
            pTxCharacteristic->notify();
        }
    } 
    else if (command.startsWith("STATUS")) {
        Serial.print("STATUS X:");
        Serial.print(current_x);
        Serial.print(" Y:");
        Serial.println(current_y);
    } 
    else {
        process_command_sequence(command);
        if(deviceConnected) {
            pTxCharacteristic->setValue("OK");
            pTxCharacteristic->notify();
        }
    }

    digitalWrite(ENABLE_PIN, HIGH); // Disable motors
}

void process_command_sequence(String command) {
    int total_prints = 0;
    for (int i = 0; i < command.length(); i++) {
        if (command.charAt(i) == CMD_PRINT) total_prints++;
    }

    int printed_count = 0;
    Serial.println("--- Starting Print Job ---");
    Serial.print("Total commands: "); Serial.println(command.length());
    Serial.print("Total dots to print: "); Serial.println(total_prints);

    for (int i = 0; i < command.length(); i++) {
        char cmd = command.charAt(i);
        
        if (cmd == CMD_PRINT) {
            Serial.println("Action: Print Dot (Solenoid Pulse)");
            digitalWrite(SOLENOID_PIN, HIGH);
            delay(SOLENOID_PULSE);
            digitalWrite(SOLENOID_PIN, LOW);
            delay(SOLENOID_GAP);
            
            printed_count++;
            if (deviceConnected) {
                String prog = "PROG:" + String(printed_count) + "/" + String(total_prints);
                pTxCharacteristic->setValue(prog.c_str());
                pTxCharacteristic->notify();
                delay(10); // Small delay to prevent BLE congestion
            }

        } else if (cmd == CMD_DOT_SHIFT_X) {
            current_x += STEP_COUNT_DOT;
            Serial.print("Action: Shift X by Dot -> New X: "); Serial.println(current_x);
            move_stepper(stepperX, current_x);

        } else if (cmd == CMD_CELL_SHIFT) {
            current_x += STEP_COUNT_CELL_X;
            Serial.print("Action: Shift X by Cell -> New X: "); Serial.println(current_x);
            move_stepper(stepperX, current_x);

        } else if (cmd == CMD_Y_SHIFT_DOT_ROW) {
            current_y += STEP_COUNT_DOT_Y;
            Serial.print("Action: Shift Y by Dot Row -> New Y: "); Serial.println(current_y);
            move_stepper(stepperY, current_y);

        } else if (cmd == CMD_LINE_FEED) {
            current_y += STEP_COUNT_LINE_Y;
            Serial.print("Action: Line Feed -> New Y: "); Serial.println(current_y);
            move_stepper(stepperY, current_y);

        } else if (cmd == CMD_GO_HOME) {
            Serial.println("Action: X Home");
            current_x = 0;
            move_stepper(stepperX, 0); 
        }
    }
    
    Serial.println("--- Print Job Completed ---");
    if (deviceConnected) {
        pTxCharacteristic->setValue("PROG:DONE");
        pTxCharacteristic->notify();
    }
}

void move_stepper(AccelStepper& stepper, long target) {
    long current_pos = stepper.currentPosition();
    
    if (target < current_pos) {
        stepper.setMaxSpeed(RETURN_SAFE_SPEED);
        stepper.setAcceleration(RETURN_ACCEL);
    } else {
        stepper.setMaxSpeed(FORWARD_MAX_SPEED);
        stepper.setAcceleration(FORWARD_ACCEL);
    }
    
    stepper.moveTo(target);
    stepper.runToPosition();
    delayMicroseconds(100);
}

void move_stepper_with_speed(AccelStepper& stepper, long target, float speed, float accel) {
    stepper.setMaxSpeed(speed);
    stepper.setAcceleration(accel);
    stepper.moveTo(target);
    stepper.runToPosition();
    delayMicroseconds(100);
}
