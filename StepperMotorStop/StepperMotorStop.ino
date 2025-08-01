#include <Servo.h>

const int stop_switch_pin_x = A0;
const int stop_switch_pin_y = A1;

// const int step_pin_x = 10;
// const int dir_pin_x = 11;

const int step_pin_x = 3;
const int dir_pin_x = 9;

const int step_pin_y = 5;
const int dir_pin_y = 6;

const int stepDelay = 800;    // microseconds (speed)
const int stepsPerTile = 80;  // steps for one tile distance


const int servoWhitePin = 11;
const int servoBlackPin = 10;

const int SERVO_MAX = 179;
const int SERVO_MIN = 0;

const int resume_pin = A4;

int currentX = 0;
int currentY = 0;
int maxX = 5;  // number of tiles horizontally
int maxY = 5;  // number of tiles vertically


Servo servoWhite;
Servo servoBlack;

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(stop_switch_pin_x, INPUT_PULLUP);
  pinMode(stop_switch_pin_y, INPUT_PULLUP);

  pinMode(step_pin_x, OUTPUT);
  pinMode(dir_pin_x, OUTPUT);

  pinMode(step_pin_y, OUTPUT);
  pinMode(dir_pin_y, OUTPUT);

  pinMode(resume_pin, INPUT);


  servoWhite.attach(servoWhitePin);
  servoBlack.attach(servoBlackPin);

  servoWhite.write(SERVO_MAX);
  servoBlack.write(SERVO_MIN);

  delay(500);  // wait for servos to initialize

  Serial.println("System Ready. Send commands: HOME, X3Y2, ROW");
}

void dispenseWhite() {
  servoWhite.write(SERVO_MIN);
  delay(1000);
  servoWhite.write(SERVO_MAX);
  delay(1000);
}


void dispenseBlack() {
  servoBlack.write(SERVO_MAX);
  delay(1000);
  servoBlack.write(SERVO_MIN);
  delay(1000);
}

String commands[] = {
  "HOME", "X5Y0", "PAUSE", "X10Y0", "X8Y0", "X12Y5", "X5Y5", "X5Y0", "HOME"
};
int commandCount = sizeof(commands) / sizeof(commands[0]);
int commandIndex = 0;

bool runCommands = false;

void loop() {

  // Serial.println(digitalRead(resume_pin) == LOW ? "PRESSED" : "NOT PRESSED");

  if (!runCommands && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    if (input == "START") {
      Serial.println("Starting command sequence...");
      runCommands = true;
    }
  }

    // Check A4 Button START
  if (!runCommands && digitalRead(resume_pin) == LOW) {
    Serial.println("Starting command sequence (A4 pressed)...");
    // Wait for button release to avoid immediate PAUSE trigger later
    while (digitalRead(resume_pin) == LOW) delay(10);
    runCommands = true;
  }


  if (runCommands && commandIndex < commandCount) {
    String command = commands[commandIndex];
    Serial.print("Executing: ");
    Serial.println(command);
    runCommand(command);
    commandIndex++;
    delay(1000); // wait a bit between commands
  } else if (runCommands && commandIndex >= commandCount) {
    Serial.println("All commands done.");

    // reseet
    runCommands = false;
    commandIndex = 0;
  }
}


// ==================== MOVEMENT ====================
void moveX(int steps, bool direction) {
  digitalWrite(dir_pin_x, direction);
  for (int i = 0; i < steps; i++) {
    digitalWrite(step_pin_x, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(step_pin_x, LOW);
    delayMicroseconds(stepDelay);
  }
}

void moveY(int steps, bool direction) {
  digitalWrite(dir_pin_y, direction);
  for (int i = 0; i < steps; i++) {
    digitalWrite(step_pin_y, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(step_pin_y, LOW);
    delayMicroseconds(stepDelay);
  }
}

void moveToTile(int x, int y) {
  int dx = x - currentX;
  int dy = y - currentY;

  if (dx > 0) moveX(dx * stepsPerTile, HIGH);
  else if (dx < 0) moveX(-dx * stepsPerTile, LOW);

  if (dy > 0) moveY(dy * stepsPerTile, HIGH);
  else if (dy < 0) moveY(-dy * stepsPerTile, LOW);

  currentX = x;
  currentY = y;

  Serial.print("Moved to tile: ");
  Serial.print(currentX);
  Serial.print(",");
  Serial.println(currentY);
}

// ==================== HOMING ====================
void homeAll() {
  Serial.println("Homing...");

  unsigned long startTime = millis();
  // Home X
  digitalWrite(dir_pin_x, LOW);                     // move left
  while (digitalRead(stop_switch_pin_x) == HIGH) {  // HIGH = not pressed (because INPUT_PULLUP)
    moveX(1, LOW);
    if (millis() - startTime > 5000) {
      Serial.println("X Homing timeout!");
      break;
    }
  }

  startTime = millis();
  // Home Y
  // digitalWrite(dir_pin_y, LOW); // move down
  // while (digitalRead(stop_switch_pin_y) == HIGH) {
  //   moveY(1, LOW);
  //   if (millis() - startTime > 5000) {
  //     Serial.println("Y Homing timeout!");
  //     break;
  //   }
  // }

  currentX = 0;
  currentY = 0;
  Serial.println("Home complete!");
}

void runCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command == "HOME") {
    homeAll();
  } else if (command == "PAUSE") {
    waitForResume();
  } else if (command.startsWith("X") && command.indexOf("Y") != -1) {
    int xIndex = 1;
    int yIndex = command.indexOf("Y") + 1;
    int x = command.substring(xIndex, command.indexOf("Y")).toInt();
    int y = command.substring(yIndex).toInt();
    moveToTile(x, y);
  } else if (command == "ROW") {
    for (int y = 0; y < maxY; y++) {
      for (int x = 0; x < maxX; x++) {
        moveToTile(x, y);
        delay(500);  // simulate placing tile
      }
    }
    Serial.println("Completed ROW pattern.");
  } else {
    Serial.println("Unknown command!");
  }
}

void waitForResume() {
  Serial.println("Paused. Press A4 to continue...");
  while (digitalRead(resume_pin) == HIGH) {
    delay(10);  // debounce wait
  }
  while (digitalRead(resume_pin) == LOW) {
    delay(10);  // wait until released
  }
  Serial.println("Resumed!");
}


// ==================== SERIAL COMMANDS ====================
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    runCommand(command);
  }