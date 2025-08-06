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

#define STEP_DELAY 900

#define STEPS_PER_TILE 47

#define SERVO_PIN_WHITE 11
#define SERVO_PIN_BLACK 10

#define SERVO_MAX_BLACK 39
#define SERVO_MIN_BLACK 149

#define SERVO_MAX_WHITE 30
#define SERVO_MIN_WHITE 140

#define RETRACT_AMOUNT 3 // tiles to retract before dispensing

#define B 0
#define W 1

// Enable to track remaining tiles and pause if they run out
#define TRACK_REMAINING_TILES true

#define TILE_CAPACITY 10
// #define TILE_CAPACITY 19

bool isHomed = false;

int currentX = 0;
int currentY = 0;

int maxX = 925;
int maxY = 844;

// Tile dispenser counters
int blackTilesRemaining = TILE_CAPACITY; // Initialize with starting count
int whiteTilesRemaining = TILE_CAPACITY; // Initialize with starting count

Servo servoBlack;
Servo servoWhite;

// 3x3 test image
// const int IMAGE_TO_DRAW[1][1] = {
//     {W}};

// Checkerpattern
// const int IMAGE_TO_DRAW[4][4] = {
//     {B, W, B, W},
//     {W, B, W, B},
//     {B, W, B, W},
//     {W, B, W, B}};

// Smiley face image
const int IMAGE_TO_DRAW[6][6] = {
    {W, W, W, W, W, W},
    {W, B, W, W, B, W},
    {W, W, W, W, W, W},
    {B, W, W, W, W, B},
    {W, B, B, B, B, W},
    {W, W, W, W, W, W}};

// For precision test
// String commands[] = {
//     "HOME",
//     "X10Y10",
//     "PAUSE",
//     "X1Y01",

//     "X10Y10",
//     "PAUSE",
//     "X1Y01",

//     "X10Y10",
//     "PAUSE",
//     "X1Y01",

//     "X10Y10",
//     "PAUSE",
//     "X1Y01",

//     "X10Y10",
//     "PAUSE",
//     "X1Y01",
// };

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
  Serial.println("Ready. Press blue button to run predefined commands: " + cmds);
}

void dispenseBlack(int nextTileColor)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return;
  }
  if (TRACK_REMAINING_TILES)
  {
    if (blackTilesRemaining <= 0)
    {
      Serial.println("Please refill tiles!");
      runCommand("PAUSE");
      blackTilesRemaining = TILE_CAPACITY;
      whiteTilesRemaining = TILE_CAPACITY;
    }
    blackTilesRemaining--;
  }
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
  if (TRACK_REMAINING_TILES)
  {
    if (whiteTilesRemaining <= 0)
    {
      Serial.println("Please refill tiles!");
      runCommand("PAUSE");
      blackTilesRemaining = TILE_CAPACITY;
      whiteTilesRemaining = TILE_CAPACITY;
    }
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

  // Use Bresenham-like algorithm for smooth diagonal movement
  int maxSteps = max(stepsX, stepsY);
  int errorX = maxSteps / 2;
  int errorY = maxSteps / 2;
  int stepsRemainingX = stepsX;
  int stepsRemainingY = stepsY;

  for (int i = 0; i < maxSteps; i++)
  {
    bool stepX = false;
    bool stepY = false;

    // Determine which motors should step this iteration
    if (stepsRemainingX > 0)
    {
      errorX -= stepsX;
      if (errorX < 0)
      {
        errorX += maxSteps;
        stepX = true;
        stepsRemainingX--;
      }
    }

    if (stepsRemainingY > 0)
    {
      errorY -= stepsY;
      if (errorY < 0)
      {
        errorY += maxSteps;
        stepY = true;
        stepsRemainingY--;
      }
    }

    // Check limit switches before stepping
    if (stepX && digitalRead(directionX == LOW ? STOP_SWITCH_PIN_X_MIN : STOP_SWITCH_PIN_X_MAX) == LOW)
    {
      return HIGH;
    }
    if (stepY && digitalRead(directionY == LOW ? STOP_SWITCH_PIN_Y_MIN : STOP_SWITCH_PIN_Y_MAX) == LOW)
    {
      return HIGH;
    }

    // Pulse both motors simultaneously
    if (stepX)
      digitalWrite(STEP_PIN_X, HIGH);
    if (stepY)
      digitalWrite(STEP_PIN_Y, HIGH);

    delayMicroseconds(STEP_DELAY);

    if (stepX)
      digitalWrite(STEP_PIN_X, LOW);
    if (stepY)
      digitalWrite(STEP_PIN_Y, LOW);

    delayMicroseconds(STEP_DELAY);

    // Update position counters
    if (stepX)
      currentX += (directionX == LOW ? -1 : 1);
    if (stepY)
      currentY += (directionY == LOW ? -1 : 1);
  }

  return LOW;
}

void moveToTile(int x, int y)
{
  if (!isHomed)
  {
    Serial.println("Not homed! Please home first.");
    return;
  }

  // Calculate actual steps per tile based on measured dimensions
  // Assuming the work area has a certain number of tiles that fit in maxX/maxY
  // If you know how many tiles fit, adjust these divisors accordingly
  int tilesInX = maxX / STEPS_PER_TILE; // Number of tiles that fit in X direction
  int tilesInY = maxY / STEPS_PER_TILE; // Number of tiles that fit in Y direction

  // Calculate actual steps per tile
  int actualStepsPerTileX = (tilesInX > 0) ? maxX / tilesInX : STEPS_PER_TILE;
  int actualStepsPerTileY = (tilesInY > 0) ? maxY / tilesInY : STEPS_PER_TILE;

  // Calculate target position in steps
  int targetStepsX = x * actualStepsPerTileX;
  int targetStepsY = y * actualStepsPerTileY;

  // Calculate movement needed
  int dx = targetStepsX - currentX;
  int dy = targetStepsY - currentY;
  int stepsX = abs(dx);
  int stepsY = abs(dy);
  bool directionX = (dx > 0) ? HIGH : LOW;
  bool directionY = (dy > 0) ? HIGH : LOW;

  moveXY(stepsX, stepsY, directionX, directionY);

  // Update current position to actual target (not tile position)
  currentX = targetStepsX;
  currentY = targetStepsY;

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

  Serial.print("DX=");
  Serial.print(dx);
  Serial.print(", DY=");
  Serial.println(dy);

  // Calculate actual steps per tile based on measured dimensions
  int tilesInX = maxX / STEPS_PER_TILE;
  int tilesInY = maxY / STEPS_PER_TILE;

  int actualStepsPerTileX = (tilesInX > 0) ? maxX / tilesInX : STEPS_PER_TILE;
  int actualStepsPerTileY = (tilesInY > 0) ? maxY / tilesInY : STEPS_PER_TILE;

  int stepsX = abs(dx) * actualStepsPerTileX;
  int stepsY = abs(dy) * actualStepsPerTileY;
  bool directionX = (dx > 0) ? HIGH : LOW;
  bool directionY = (dy > 0) ? HIGH : LOW;

  moveXY(stepsX, stepsY, directionX, directionY);

  currentX += (dx > 0 ? stepsX : -stepsX);
  currentY += (dy > 0 ? stepsY : -stepsY);

  printCurrentPos();
}

// ==================== HOMING ====================
void homeAll()
{
  isHomed = true;
  maxX = 0;
  maxY = 0;
  Serial.println("HOME_X...");
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

  delay(PAUSE_DELAY);
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

  Serial.println("HOME_X complete!");

  // Y_MAX
  delay(PAUSE_DELAY);
  Serial.println("HOME_Y...");
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

  delay(100);

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

  if (command == "PAUSE" || command == "P")
  {
    waitForResume();
  }
  else if (command == "HOME" || command == "H")
  {
    homeAll();
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

  int startTime = millis();
  moveToTile(0, 0);
  int x = 0;
  int y = 0;
  int TILE_WIDTH = sizeof(IMAGE_TO_DRAW[0]) / sizeof(IMAGE_TO_DRAW[0][0]);
  int TILE_HEIGHT = sizeof(IMAGE_TO_DRAW) / sizeof(IMAGE_TO_DRAW[0]);
  for (y = 0; y < TILE_HEIGHT; y++)
  {
    for (x = 0; x < TILE_WIDTH; x++)
    {
      if (digitalRead(RESUME_SWITCH_PIN) == LOW)
      {
        runCommand("PAUSE");
      }
      else
      {
        Serial.println("No pause, just continuing");
      }
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
  unsigned long elapsedMillis = millis() - startTime;
  unsigned long totalSeconds = elapsedMillis / 1000;

  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds % 3600) / 60;
  int seconds = totalSeconds % 60;

  char buffer[12];
  sprintf(buffer, "%02d.%02d.%02d", hours, minutes, seconds);

  Serial.print("Image drawn in ");
  Serial.println(buffer);

  moveToTile(50, 50);
}
