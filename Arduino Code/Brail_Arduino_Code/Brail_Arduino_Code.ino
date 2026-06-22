#include <AccelStepper.h>

// --- CNC Shield Pins ---
#define X_STEP_PIN 2
#define X_DIR_PIN 5
#define Y_STEP_PIN 3
#define Y_DIR_PIN 6
#define SOLENOID_PIN 9
#define ENABLE_PIN 8   // LOW = enable, HIGH = disable

// --- Stepper Setup ---
AccelStepper stepperX(1, X_STEP_PIN, X_DIR_PIN);
AccelStepper stepperY(1, Y_STEP_PIN, Y_DIR_PIN);

// --- Movement Calibration ---

const int STEP_COUNT_DOT = 10;      // 2.5 mm dot-to-dot
const int STEP_COUNT_CELL_X = 15;   // 3.5 mm cell-to-cell
const int STEP_COUNT_DOT_Y = 10;    // 2.5 mm dot-row spacing
const int STEP_COUNT_LINE_Y = 20;   // 5.0 mm line spacing

// --- Printer State ---
long current_x = 0;
long current_y = 0; 

// --- Solenoid Parameters ---
const int SOLENOID_PULSE = 120; // ms
const int SOLENOID_GAP   = 50;  // ms

// --- Command Symbols ---
const char CMD_PRINT = 'P';
const char CMD_DOT_SHIFT_X = 'X';
const char CMD_CELL_SHIFT = 'C';
const char CMD_Y_SHIFT_DOT_ROW = 'Y';
const char CMD_LINE_FEED = 'L';
const char CMD_GO_HOME = 'H';

// --- Function Prototypes ---
void move_stepper(AccelStepper& stepper, long target);
void process_command_sequence(String command);

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(5000); // Wait up to 5s for long strings

  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);
  digitalWrite(ENABLE_PIN, HIGH); 

  // --- SPEED SETTINGS ---
  // Lower speed to prevent stalling when returning home
  stepperX.setMaxSpeed(2000.0);      
  stepperX.setAcceleration(1000.0);  

  stepperY.setMaxSpeed(2000.0);      
  stepperY.setAcceleration(1000.0);  
  
  Serial.println("✅ PRINTER_READY 115200");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.length() == 0) return;

    digitalWrite(ENABLE_PIN, LOW); // Enable motors

    if (command.startsWith("HOME")) {
        current_x = 0;
        current_y = 0;
        stepperX.setCurrentPosition(0);
        stepperY.setCurrentPosition(0);
        Serial.println("✅ HOME_COMPLETED");
    } 
    else if (command.startsWith("STATUS")) {
        Serial.print("STATUS X:");
        Serial.print(current_x);
        Serial.print(" Y:");
        Serial.println(current_y);
    } 
    else {
        process_command_sequence(command);
    }

    digitalWrite(ENABLE_PIN, HIGH); // Disable motors
    Serial.println("OK"); 
  }
}

void process_command_sequence(String command) {
    for (int i = 0; i < command.length(); i++) {
        char cmd = command.charAt(i);
        
        if (cmd == CMD_PRINT) {
            digitalWrite(SOLENOID_PIN, HIGH);
            delay(SOLENOID_PULSE);
            digitalWrite(SOLENOID_PIN, LOW);
            delay(SOLENOID_GAP);

        } else if (cmd == CMD_DOT_SHIFT_X) {
            current_x += STEP_COUNT_DOT;
            move_stepper(stepperX, current_x);

        } else if (cmd == CMD_CELL_SHIFT) {
            current_x += STEP_COUNT_CELL_X;
            move_stepper(stepperX, current_x);

        } else if (cmd == CMD_Y_SHIFT_DOT_ROW) {
            current_y += STEP_COUNT_DOT_Y;
            move_stepper(stepperY, current_y);

        } else if (cmd == CMD_LINE_FEED) {
            current_y += STEP_COUNT_LINE_Y;
            move_stepper(stepperY, current_y);

        } else if (cmd == CMD_GO_HOME) {
            // H: Go to (0,0) position on X axis
            current_x = 0;
            move_stepper(stepperX, 0); 
        }
    }
}

void move_stepper(AccelStepper& stepper, long target) {
    stepper.moveTo(target);
    stepper.runToPosition();
}