// CNC Shield Test - X & Y motors only
// Pin map matches Brail_Arduino_Code.ino

const int StepX  = 2;   // X_STEP_PIN  (CNC shield X-STEP)
const int DirX   = 5;   // X_DIR_PIN   (CNC shield X-DIR)
const int StepY  = 3;   // Y_STEP_PIN  (CNC shield Y-STEP)
const int DirY   = 6;   // Y_DIR_PIN   (CNC shield Y-DIR)
const int Enable = 8;   // ENABLE_PIN  (CNC shield EN, active LOW)

// ---------- Tuning ----------
const int  stepsPerRev   = 200;   // NEMA-17 (1.8 deg) full steps
const int  microsteps    = 1;     // 1, 2, 4, 8, 16  (CNC shield jumper setting)
const int  stepsPerCycle = 1;     // microsteps * stepsPerRev / microsteps = 200 here
const long stepsPerSec   = 400;   // step frequency (start low: 200-400)
const long pauseMs       = 1000;  // pause between phases

unsigned long stepInterval = 1000000UL / stepsPerSec;  // microseconds between steps
int  stepCount = stepsPerRev * microsteps;             // 200 with full-step NEMA

void setup() {
  Serial.begin(115200);
  Serial.println("CNC Shield X+Y Test");
  Serial.print("steps/rev: "); Serial.println(stepsPerRev);
  Serial.print("microsteps: "); Serial.println(microsteps);
  Serial.print("steps per phase: "); Serial.println(stepCount);
  Serial.print("step freq Hz: "); Serial.println(stepsPerSec);

  pinMode(StepX, OUTPUT);
  pinMode(DirX,  OUTPUT);
  pinMode(StepY, OUTPUT);
  pinMode(DirY,  OUTPUT);
  pinMode(Enable, OUTPUT);

  // Driver boards OFF until we are ready to move
  digitalWrite(Enable, HIGH);
  digitalWrite(StepX, LOW);
  digitalWrite(StepY, LOW);
}

// One direction move: 200 steps in target direction
void runPhase(int dirXPin, int dirYPin, int dirX, int dirY, const char* label) {
  Serial.print(label); Serial.print("  X="); Serial.print(dirX ? "+" : "-");
  Serial.print("  Y="); Serial.print(dirY ? "+" : "-");
  Serial.print("  ("); Serial.print(stepCount); Serial.println(" steps)");

  digitalWrite(DirX, dirX ? HIGH : LOW);
  digitalWrite(DirY, dirY ? HIGH : LOW);
  delay(50);  // let DIR settle (>200 ns is enough, 50 ms is safe)

  digitalWrite(Enable, LOW);  // energize coils
  delay(20);

  unsigned long nextStep = micros();
  for (int i = 0; i < stepCount; i++) {
    digitalWrite(StepX, HIGH);
    digitalWrite(StepY, HIGH);
    delayMicroseconds(5);
    digitalWrite(StepX, LOW);
    digitalWrite(StepY, LOW);

    nextStep += stepInterval;
    unsigned long now = micros();
    if (nextStep > now) {
      delayMicroseconds((unsigned int)(nextStep - now));
    } else {
      nextStep = now;  // fell behind, resync
    }
  }
  digitalWrite(Enable, HIGH);  // de-energize
  delay(pauseMs);
}

void loop() {
  // Diagonal out: X+ Y+
  runPhase(DirX, DirY, 1, 1, "PHASE 1: X+ Y+");
  // Square: X- Y+
  runPhase(DirX, DirY, 0, 1, "PHASE 2: X- Y+");
  // Diagonal in: X- Y-
  runPhase(DirX, DirY, 0, 0, "PHASE 3: X- Y-");
  // Square: X+ Y-
  runPhase(DirX, DirY, 1, 0, "PHASE 4: X+ Y-");

  Serial.println("---- cycle done, resting 3 s ----");
  delay(3000);
}
