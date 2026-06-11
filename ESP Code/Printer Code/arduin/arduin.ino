#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <AccelStepper.h>
#include <soc/rtc_cntl_reg.h>   // RTC_CNTL_WDT* registers (ESP32-S3, IDF v5.5)

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
        for (int i = 0; i < rxValue.length(); i++) {
          char c = rxValue[i];
          if (c == '\n') commandReady = true;
          else bleBuffer += c;
        }
      }
    }
};

void setup() {
  // --- 1. Serial first, so we see every subsequent line ---
  Serial.begin(115200);
  delay(100);

  // --- 2. Kill the hardware RTC watchdog directly (S3 + IDF v5.5 bypass) ---
  // disableCore0WDT() is a no-op on ESP32-S3 in 3.0.x — must use register poke
  REG_WRITE(RTC_CNTL_WDTWPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
  REG_WRITE(RTC_CNTL_WDTCONFIG0_REG, 0);
  REG_WRITE(RTC_CNTL_WDTWPROTECT_REG, 0);

  Serial.println("\n=== BOOT ===");

  // --- 4. Pins ---
  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);
  digitalWrite(ENABLE_PIN, HIGH);
  Serial.println("[1/5] Pins OK");

  // --- 5. Motors (at full 240 MHz - the slow clock was killing BLE init) ---
  setCpuFrequencyMhz(240);
  stepperX.setMaxSpeed(FORWARD_MAX_SPEED);
  stepperX.setAcceleration(FORWARD_ACCEL);
  stepperY.setMaxSpeed(FORWARD_MAX_SPEED);
  stepperY.setAcceleration(FORWARD_ACCEL);
  Serial.println("[2/5] Motors OK");

  // --- 6. BLE Device ---
  Serial.println("[3/5] BLE init...");
  BLEDevice::init("eBrail");
  Serial.println("      BLE device inited");

  // --- 7. BLE Server + Service ---
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  Serial.println("[4/5] Service started");

  // --- 8. Advertising ---
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);

  delay(100);
  BLEDevice::startAdvertising();
  Serial.println("[5/5] Advertising as 'eBrail'");
  Serial.println("READY");
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
            Serial.print("[RX ");
            Serial.print(bleBuffer.length());
            Serial.print("B] \"");
            Serial.print(bleBuffer);
            Serial.println("\"");
            handle_full_command(bleBuffer);
        }
        bleBuffer = "";
        commandReady = false;
    }
}

void handle_full_command(String command) {
    digitalWrite(ENABLE_PIN, LOW); // Enable motors

    if (command.startsWith("HOME")) {
        Serial.println("CMD: HOME");
        move_stepper_with_speed(stepperX, 0, RETURN_SAFE_SPEED, RETURN_ACCEL);
        delay(100);

        current_x = 0;
        current_y = 0;
        stepperX.setCurrentPosition(0);
        stepperY.setCurrentPosition(0);

        stepperX.setMaxSpeed(FORWARD_MAX_SPEED);
        stepperX.setAcceleration(FORWARD_ACCEL);

        if (deviceConnected) {
            pTxCharacteristic->setValue("HOME_COMPLETED");
            pTxCharacteristic->notify();
        }
    }
    else if (command.startsWith("EJECT")) {
        Serial.println("CMD: EJECT");
        move_stepper_with_speed(stepperY, -500, 1000.0, 300.0);
        delay(200);
        move_stepper_with_speed(stepperY, 0, 1000.0, 300.0);
        current_y = 0;
        stepperY.setCurrentPosition(0);

        if (deviceConnected) {
            pTxCharacteristic->setValue("PAPER_EJECTED");
            pTxCharacteristic->notify();
        }
    }
    else if (command.startsWith("STATUS")) {
        Serial.print("STATUS X=");
        Serial.print(current_x);
        Serial.print(" Y=");
        Serial.println(current_y);
    }
    else {
        process_command_sequence(command);
        if (deviceConnected) {
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
    unsigned long jobT0 = millis();

    Serial.print("JOB: ");
    Serial.print(command.length());
    Serial.print(" cmds, ");
    Serial.print(total_prints);
    Serial.print(" punches @ X=");
    Serial.print(current_x);
    Serial.print(" Y=");
    Serial.println(current_y);

    for (int i = 0; i < command.length(); i++) {
        char cmd = command.charAt(i);

        if (cmd == CMD_PRINT) {
            Serial.print("  ["); Serial.print(i); Serial.print("] PUNCH");
            digitalWrite(SOLENOID_PIN, HIGH);
            delay(SOLENOID_PULSE);
            digitalWrite(SOLENOID_PIN, LOW);
            delay(SOLENOID_GAP);

            printed_count++;
            if (deviceConnected) {
                String prog = "PROG:" + String(printed_count) + "/" + String(total_prints);
                pTxCharacteristic->setValue(prog.c_str());
                pTxCharacteristic->notify();
                delay(10);
            }

        } else if (cmd == CMD_DOT_SHIFT_X) {
            Serial.print("  ["); Serial.print(i); Serial.print("] X+"); Serial.print(STEP_COUNT_DOT);
            current_x += STEP_COUNT_DOT;
            move_stepper(stepperX, current_x);

        } else if (cmd == CMD_CELL_SHIFT) {
            Serial.print("  ["); Serial.print(i); Serial.print("] CELL+"); Serial.print(STEP_COUNT_CELL_X);
            current_x += STEP_COUNT_CELL_X;
            move_stepper(stepperX, current_x);

        } else if (cmd == CMD_Y_SHIFT_DOT_ROW) {
            Serial.print("  ["); Serial.print(i); Serial.print("] Y+"); Serial.print(STEP_COUNT_DOT_Y);
            current_y += STEP_COUNT_DOT_Y;
            move_stepper(stepperY, current_y);

        } else if (cmd == CMD_LINE_FEED) {
            Serial.print("  ["); Serial.print(i); Serial.print("] LINE+"); Serial.print(STEP_COUNT_LINE_Y);
            current_y += STEP_COUNT_LINE_Y;
            move_stepper(stepperY, current_y);

        } else if (cmd == CMD_GO_HOME) {
            Serial.print("  ["); Serial.print(i); Serial.print("] HOME");
            current_x = 0;
            move_stepper(stepperX, 0);
        }
    }

    Serial.print("DONE: "); Serial.print(millis() - jobT0);
    Serial.print("ms, end X="); Serial.print(current_x);
    Serial.print(" Y="); Serial.println(current_y);

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
