/*
 * ESP32 Solenoid Test Utility
 *
 * Use this sketch to test solenoid pins individually.
 * Enter commands in Serial Monitor to activate dots.
 *
 * Commands:
 * - "1" to "8": Activate dot 1-8
 * - "FF": Activate all dots (0xFF = 11111111 binary)
 * - "00": Deactivate all
 * - "01": Activate dot 1 only (0x01 = 00000001 binary)
 * - "03": Activate dots 1,2 (0x03 = 00000011 binary)
 * - "TEST": Run automatic test sequence
 */

// GPIO pins for solenoid drivers (controlling 8 braille dots)
#define PIN_DOT1 12
#define PIN_DOT2 13
#define PIN_DOT3 14
#define PIN_DOT4 27
#define PIN_DOT5 26
#define PIN_DOT6 25
#define PIN_DOT7 33
#define PIN_DOT8 32

int solenoidPins[] = {PIN_DOT1, PIN_DOT2, PIN_DOT3, PIN_DOT4,
                       PIN_DOT5, PIN_DOT6, PIN_DOT7, PIN_DOT8};

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================");
    Serial.println("ESP32 SOLENOID TEST UTILITY");
    Serial.println("========================================\n");

    // Initialize pins
    for (int i = 0; i < 8; i++) {
        pinMode(solenoidPins[i], OUTPUT);
        digitalWrite(solenoidPins[i], LOW);
        Serial.print("Pin ");
        Serial.print(solenoidPins[i]);
        Serial.print(" (Dot ");
        Serial.print(i + 1);
        Serial.println(") -> OUTPUT");
    }

    Serial.println("\nCommands:");
    Serial.println("  1-8  : Activate dot (1=Dot 1, 2=Dot 2, ..., 8=Dot 8)");
    Serial.println("  FF   : Activate ALL dots");
    Serial.println("  00   : Deactivate ALL dots");
    Serial.println("  01   : Hex pattern (01=Dot1, 03=Dot1+2, FF=All)");
    Serial.println("  TEST : Run automatic test sequence");
    Serial.println("  ?\n");
}

void activateDotPattern(uint8_t pattern) {
    Serial.print("Activating pattern 0x");
    Serial.print(pattern, HEX);
    Serial.print(" (binary: ");

    // Print binary
    for (int i = 7; i >= 0; i--) {
        Serial.print(bitRead(pattern, i));
    }
    Serial.print(") -> Dots: ");

    // Activate pins
    for (int i = 0; i < 8; i++) {
        digitalWrite(solenoidPins[i], bitRead(pattern, i) ? HIGH : LOW);
        if (bitRead(pattern, i)) {
            Serial.print(i + 1);
            Serial.print(" ");
        }
    }
    Serial.println("\n");
}

void testSequence() {
    Serial.println("\n========== RUNNING TEST SEQUENCE ==========\n");

    // Test individual dots
    Serial.println("Testing individual dots (1 second each):");
    for (int i = 0; i < 8; i++) {
        Serial.print("Dot ");
        Serial.println(i + 1);
        activateDotPattern(1 << i);
        delay(1000);
        digitalWrite(solenoidPins[i], LOW);
    }

    // Test all dots
    Serial.println("\nActivating ALL dots:");
    activateDotPattern(0xFF);
    delay(2000);

    // Deactivate all
    Serial.println("Deactivating all:");
    activateDotPattern(0x00);
    delay(500);

    // Test patterns
    Serial.println("\nTesting common patterns:");
    uint8_t patterns[] = {0x01, 0x03, 0x0F, 0x3F, 0xFF};
    String names[] = {"Single", "Two dots", "Half", "Three-quarter", "All"};

    for (int i = 0; i < 5; i++) {
        Serial.print(names[i]);
        Serial.print(": ");
        activateDotPattern(patterns[i]);
        delay(1000);
    }

    // Deactivate
    activateDotPattern(0x00);
    Serial.println("========== TEST COMPLETE ==========\n");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        if (input.length() == 0) {
            return;
        }

        Serial.print("Input: ");
        Serial.println(input);
        Serial.println();

        if (input == "TEST") {
            testSequence();
        }
        else if (input.length() == 1 && input[0] >= '1' && input[0] <= '8') {
            int dot = input[0] - '0';
            activateDotPattern(1 << (dot - 1));
            delay(500);
            activateDotPattern(0x00);
        }
        else if (input.length() == 2) {
            // Parse as hex
            uint8_t pattern = (uint8_t)strtol(input.c_str(), nullptr, 16);
            activateDotPattern(pattern);
            delay(500);
            activateDotPattern(0x00);
        }
        else if (input == "?") {
            Serial.println("Commands:");
            Serial.println("  1-8  : Activate dot");
            Serial.println("  FF   : All dots");
            Serial.println("  00   : No dots");
            Serial.println("  TEST : Test sequence\n");
        }
        else {
            Serial.println("Unknown command. Type '?' for help.\n");
        }
    }
}

