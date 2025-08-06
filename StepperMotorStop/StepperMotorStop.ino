#include <Servo.h>

#define RESUME_SWITCH_PIN A0

#define STOP_SWITCH_PIN_Y_MIN A1
#define STOP_SWITCH_PIN_X_MIN A2

#define STOP_SWITCH_PIN_Y_MAX A3
#define STOP_SWITCH_PIN_X_MAX A4

// GREEN WIRE AWAY
#define DIR_PIN_X 6
#define STEP_PIN_X 7

// RED WIRE UP
#define DIR_PIN_Y 4
#define STEP_PIN_Y 5

#define STEP_DELAY 1000 // microseconds (movement speed
// #define step_delay 800 // microseconds (movement speed

// TODO: Calibrate this:
#define STEPS_PER_TILE 46 // steps for one tile distanc

#define SERVO_PIN_WHITE 11
#define SERVO_PIN_BLACK 10

#define SERVO_MAX_BLACK 39
#define SERVO_MIN_BLACK 149

#define SERVO_MAX_WHITE 30
#define SERVO_MIN_WHITE 140

#define RETRACT_AMOUNT 3 // tiles to retract before dispensing

#define B 0
#define W 1

bool isHomed = LOW;

int currentX = 0;
int currentY = 0;

int maxX = 925;
int maxY = 844;

// Tile dispenser counters
int blackTilesRemaining = 19; // Initialize with starting count
int whiteTilesRemaining = 19; // Initialize with starting count

Servo servoBlack;
Servo servoWhite;

// 3x3 test image
const int IMAGE_TO_DRAW[3][3] = {
    {W, W, W},
    {W, B, B},
    {W, B, B}};

// Smiley face image
// const int IMAGE_TO_DRAW[8][8] = {
//     {B, B, B, B, B, B, B, B},
//     {B, B, W, B, B, W, B, B},
//     {B, B, W, B, B, W, B, B},
//     {B, B, W, B, B, W, B, B},
//     {W, B, B, B, B, B, B, W},
//     {W, W, B, B, B, B, W, W},
//     {B, W, W, B, B, W, W, B},
//     {B, B, W, W, W, W, B, B}};

String commands[] = {
    "DRAW"};

int commandCount = sizeof(commands) / sizeof(commands[0]);
int commandIndex = 0;

bool shouldExecuteCommandsAtLaunch = false;

void setup()
{
  Serial.begin(9600);

  // Switch pins
  pinMode(RESUME_SWITCH_PIN, INPUT);

  pinMode(STOP_SWITCH_PIN_X_MIN, INPUT_PULLUP);
  pinMode(STOP_SWITCH_PIN_Y_MIN, INPUT_PULLUP);

  pinMode(STOP_SWITCH_PIN_X_MAX, INPUT_PULLUP);
  pinMode(STOP_SWITCH_PIN_Y_MAX, INPUT_PULLUP);

  // Stepper pins
  pinMode(STEP_PIN_X, OUTPUT);
  pinMode(DIR_PIN_X, OUTPUT);

  pinMode(STEP_PIN_Y, OUTPUT);
  pinMode(DIR_PIN_Y, OUTPUT);

  // Initialize servos
  servoWhite.attach(SERVO_PIN_WHITE);
  servoBlack.attach(SERVO_PIN_BLACK);

  servoWhite.write(SERVO_MAX_WHITE);
  servoBlack.write(SERVO_MIN_BLACK);

  String cmds = "";
  for (int i = 0; i < commandCount; i++)
  {
    cmds += commands[i];
    if (i < commandCount - 1)
      cmds += ", ";
  }
  Serial.println("System Ready. Send commands: " + cmds);
}

void dispenseBlack(int nextTileColor)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return;
  }
  if (blackTilesRemaining <= 0)
  {
    Serial.println("Please refill white tiles!");
    Serial.println("Press button to continue!");
    runCommand("PAUSE");
    return;
  }
  blackTilesRemaining--;
  int blackVsWhiteDist = 1.45 * STEPS_PER_TILE;
  moveX(blackVsWhiteDist, HIGH);
  Serial.println("Dispensing white");
  servoWhite.write(SERVO_MIN_WHITE);
  delay(1000);
  servoWhite.write(SERVO_MAX_WHITE);
  delay(1000);
  // TODO: Optimize this, if two white tiles are placed consecutively
  moveX(blackVsWhiteDist, LOW);
  delay(300);
}

void dispenseWhite()
{
  if (whiteTilesRemaining <= 0)
  {
    Serial.println("Please refill black tiles!");
    Serial.println("Press button to continue!");
    runCommand("PAUSE");
    return;
  }
  whiteTilesRemaining--;
  Serial.println("Dispensing black");
  servoBlack.write(SERVO_MAX_BLACK);
  delay(1000);
  servoBlack.write(SERVO_MIN_BLACK);
  delay(1000);
}

void loop()
{
  if (shouldExecuteCommandsAtLaunch)
  {
    if (commandIndex < commandCount)
    {
      String command = commands[commandIndex];
      Serial.print("Executing: ");
      Serial.println(command);
      runCommand(command);
      commandIndex++;
      delay(1000); // wait a bit between commands
    }
    else
    {
      Serial.println("All commands done.");
      shouldExecuteCommandsAtLaunch = false;
      commandIndex = 0;
    }
  }
  else
  {
    // If we are not executing commands, check for serial input...
    if (Serial.available() > 0)
    {
      String input = Serial.readStringUntil('\n');
      input.trim();
      input.toUpperCase();
      if (input == "START" || input == "RUN")
      {
        Serial.println("Starting command sequence...");
        shouldExecuteCommandsAtLaunch = true;
      }
      else
      {
        runCommand(input);
      }
    }
    // ...or start button press
    if (digitalRead(RESUME_SWITCH_PIN) == LOW)
    {
      Serial.println("Starting COMMAND sequence (A4 pressed)");

      // Wait for button release to avoid immediate PAUSE trigger later
      while (digitalRead(RESUME_SWITCH_PIN) == LOW)
      {
        delay(10);
      }
      shouldExecuteCommandsAtLaunch = true;
    }
  }
}

// ==================== MOVEMENT ====================
bool moveX(int steps, bool direction)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return false;
  }
  digitalWrite(DIR_PIN_X, direction);

  for (int i = 0; i < steps; i++)
  {
    if (digitalRead(direction == LOW ? STOP_SWITCH_PIN_X_MIN : STOP_SWITCH_PIN_X_MAX) == LOW)
    {
      return HIGH;
    }

    currentX += (direction == LOW ? -1 : 1);
    step(STEP_PIN_X);
  }

  return LOW;
}

bool moveY(int steps, bool direction)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return false;
  }
  digitalWrite(DIR_PIN_Y, direction);

  for (int i = 0; i < steps; i++)
  {
    if (digitalRead(direction == LOW ? STOP_SWITCH_PIN_Y_MIN : STOP_SWITCH_PIN_Y_MAX) == LOW)
    {
      return HIGH;
    }

    currentY += (direction == LOW ? -1 : 1);
    step(STEP_PIN_Y);
  }
  return LOW;
}

bool moveXY(int stepsX, int stepsY, bool directionX, bool directionY)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return false;
  }

  digitalWrite(DIR_PIN_X, directionX);
  digitalWrite(DIR_PIN_Y, directionY);

  // Interleave steps in x and y directions
  int maxSteps = max(stepsX, stepsY);
  for (int i = 0; i < maxSteps; i++)
  {
    if (i < stepsX)
    {
      if (digitalRead(directionX == LOW ? STOP_SWITCH_PIN_X_MIN : STOP_SWITCH_PIN_X_MAX) == LOW)
      {
        return HIGH;
      }
      currentX += (directionX == LOW ? -1 : 1);
      step(STEP_PIN_X);
    }

    if (i < stepsY)
    {
      if (digitalRead(directionY == LOW ? STOP_SWITCH_PIN_Y_MIN : STOP_SWITCH_PIN_Y_MAX) == LOW)
      {
        return HIGH;
      }
      currentY += (directionY == LOW ? -1 : 1);
      step(STEP_PIN_Y);
    }

    return LOW;
  }
}

void moveToTile(int x, int y)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return;
  }
  int dx = x - currentX;
  int dy = y - currentY;
  int stepsX = abs(dx) * STEPS_PER_TILE;
  int stepsY = abs(dy) * STEPS_PER_TILE;
  bool directionX = (dx > 0) ? HIGH : LOW;
  bool directionY = (dy > 0) ? HIGH : LOW;

  moveXY(stepsX, stepsY, directionX, directionY);

  currentX = x;
  currentY = y;

  printCurrentPos();
  delay(500);
}
void moveRelativeTile(int dx, int dy)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return;
  }

  Serial.print("Moving relative: dx=");
  Serial.print(dx);
  Serial.print(", dy=");
  Serial.println(dy);

  int stepsX = abs(dx) * STEPS_PER_TILE;
  int stepsY = abs(dy) * STEPS_PER_TILE;
  bool directionX = (dx > 0) ? HIGH : LOW;
  bool directionY = (dy > 0) ? HIGH : LOW;

  moveXY(stepsX, stepsY, directionX, directionY);

  currentX += dx;
  currentY += dy;

  printCurrentPos();
}

// ==================== HOMING ====================
void homeAll()
{
  isHomed = true;
  maxX = 0;
  maxY = 0;
  Serial.println("HOME_X_MAX...");
  unsigned long startTime = millis();

  int HOME_TIMEOUT = 5000;
  int PAUSE_DELAY = 100;

  // X_MAX
  startTime = millis();

  while (digitalRead(STOP_SWITCH_PIN_X_MAX) == HIGH)
  {
    moveX(1, HIGH);
    if (millis() - startTime > HOME_TIMEOUT)
    {
      Serial.println("HOME_X_MAX timeout!");
      return;
    }
  }

  Serial.println("HOME_X_MAX complete!");
  delay(PAUSE_DELAY);
  Serial.println("HOME_X_MIN...");
  startTime = millis();

  while (digitalRead(STOP_SWITCH_PIN_X_MIN) == HIGH)
  {
    maxX += 1; // Increment maxX for each step until we hit the switch
    moveX(1, LOW);
    if (millis() - startTime > HOME_TIMEOUT)
    {
      Serial.println("HOME_X_MIN timeout!");
      break;
    }
  }
  currentX = 0;

  Serial.println("HOME_X_MIN complete!");

  // Y_MAX
  delay(PAUSE_DELAY);
  Serial.println("HOME_Y_MAX...");
  startTime = millis();

  while (digitalRead(STOP_SWITCH_PIN_Y_MAX) == HIGH)
  {
    moveY(1, HIGH);
    if (millis() - startTime > HOME_TIMEOUT)
    {
      Serial.println("HOME_Y_MAX timeout!");
      break;
    }
  }

  Serial.println("HOME_Y_MAX complete!");
  delay(100);
  Serial.println("HOME_Y_MIN...");

  startTime = millis();
  while (digitalRead(STOP_SWITCH_PIN_Y_MIN) == HIGH)
  {
    maxY += 1; // Increment maxY for each step until we hit the switch
    moveY(1, LOW);
    if (millis() - startTime > HOME_TIMEOUT)
    {
      Serial.println("HOME_Y_MIN timeout!");
      return;
      break;
    }
  }

  currentY = 0;

  Serial.println("HOME_Y complete");
  delay(PAUSE_DELAY);
  Serial.println("Homing succesful!");

  Serial.println("X_MAX: " + String(maxX));
  Serial.println("Y_MAX: " + String(maxY));
  Serial.println("currentX: " + String(currentX));
  Serial.println("currentY: " + String(currentY));
}

void runCommand(String command)
{
  command.trim();
  command.toUpperCase();

  if (command == "HOME" || command == "H")
  {
    homeAll();
  }
  else if (command == "PAUSE" || command == "P")
  {
    waitForResume();
  }
  else if (command.startsWith("RX") && command.indexOf("Y") != -1)
  {
    // Relative movement command: RX{int}Y{int}
    int xIndex = 2; // Skip "RX"
    int yIndex = command.indexOf("Y") + 1;
    int dx = command.substring(xIndex, command.indexOf("Y")).toInt();
    int dy = command.substring(yIndex).toInt();
    moveRelativeTile(dx, dy);
  }
  else if (command.startsWith("X") && command.indexOf("Y") != -1)
  {
    // Absolute movement command: X{int}Y{int}
    int xIndex = 1;
    int yIndex = command.indexOf("Y") + 1;
    int x = command.substring(xIndex, command.indexOf("Y")).toInt();
    int y = command.substring(yIndex).toInt();
    moveToTile(x, y);
  }
  else if (command == "WHITE" || command == "W")
  {
    dispenseWhite();
  }
  else if (command == "BLACK" || command == "B")
  {
    dispenseBlack(W);
  }
  else if (command == "DRAW" || command == "D")
  {
    drawImage();
  }
  else
  {
    Serial.println("Unknown command!");
  }
}

void waitForResume()
{
  Serial.println("Paused. Press resume button..");
  while (digitalRead(RESUME_SWITCH_PIN) == HIGH)
  {
    delay(10); // debounce wait
  }
  while (digitalRead(RESUME_SWITCH_PIN) == LOW)
  {
    delay(10); // wait until released
  }
  Serial.println("Resumed!");
}

// ==================== SERIAL COMMANDS ====================
void H(int pin)
{
  digitalWrite(pin, HIGH);
}

void L(int pin)
{
  digitalWrite(pin, LOW);
}

void printCurrentPos()
{
  Serial.println("X: " + String(currentX) + ", Y: " + String(currentY));
}

void step(int pin)
{
  H(pin);
  delayMicroseconds(STEP_DELAY);
  L(pin);
  delayMicroseconds(STEP_DELAY);
}

void drawImage()
{
  if (!isHomed)
  {
    runCommand("HOME");
  }
  else
  {
    moveToTile(0, 0);
  }
  int x = 0;
  int y = 0;
  int TILE_WIDTH = sizeof(IMAGE_TO_DRAW[0]) / sizeof(IMAGE_TO_DRAW[0][0]);
  int TILE_HEIGHT = sizeof(IMAGE_TO_DRAW) / sizeof(IMAGE_TO_DRAW[0]);
  for (y = 0; y < TILE_HEIGHT; y++)
  {
    for (x = 0; x < TILE_WIDTH; x++)
    {
      int currentTileColor = IMAGE_TO_DRAW[x][y];
      int nextTileColor = (x + 1 < TILE_WIDTH)
                              ? IMAGE_TO_DRAW[x + 1][y]
                              // If we are at the last tile in the row, assume white (no movement needed)
                              : W;

      // Retract and dispense
      moveToTile(x + RETRACT_AMOUNT, y + RETRACT_AMOUNT);
      if (currentTileColor == B)
      {
        dispenseBlack(nextTileColor);
      }
      else
      {
        dispenseWhite();
      }
      // Push into place
      moveToTile(x, y);
      moveToTile(x + RETRACT_AMOUNT, y + RETRACT_AMOUNT);
    }
    // After each row, retract to avoid pushing the tiles out of place
    moveToTile(x + RETRACT_AMOUNT, y + RETRACT_AMOUNT);
    // Move to next row, ready for the next iteration
    moveToTile(RETRACT_AMOUNT, y + RETRACT_AMOUNT);
  }
}
