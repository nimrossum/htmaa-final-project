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
    {1, 1, 1},
    {1, 0, 0},
    {1, 0, 0}};

// Smiley face image
// const int IMAGE_TO_DRAW[8][8] = {
//     {0, 0, 0, 0, 0, 0, 0, 0},
//     {0, 0, 1, 0, 0, 1, 0, 0},
//     {0, 0, 1, 0, 0, 1, 0, 0},
//     {0, 0, 1, 0, 0, 1, 0, 0},
//     {1, 0, 0, 0, 0, 0, 0, 1},
//     {1, 1, 0, 0, 0, 0, 1, 1},
//     {0, 1, 1, 0, 0, 1, 1, 0},
//     {0, 0, 1, 1, 1, 1, 0, 0}};

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
  pinMode(step_pin_x, OUTPUT);
  pinMode(dir_pin_x, OUTPUT);

  pinMode(STEP_PIN_Y, OUTPUT);
  pinMode(dir_pin_y, OUTPUT);

  // Initialize servos
  servoWhite.attach(servo_pin_white);
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

void dispenseBlack()
{
  if (blackTilesRemaining <= 0)
  {
    Serial.println("Please refill white tiles!");
    Serial.println("Press button to continue!");
    runCommand("PAUSE");
    return;
  }
  blackTilesRemaining--;
  int blackVsWhiteDist = 1.45 * STEPS_PER_TILE;
  moveY(blackVsWhiteDist, HIGH);
  Serial.println("Dispensing white");
  servoWhite.write(SERVO_MIN_WHITE);
  delay(1000);
  servoWhite.write(SERVO_MAX_WHITE);
  delay(1000);
  // TODO: Optimize this, if two white tiles are placed consecutively
  moveY(blackVsWhiteDist, LOW);
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
  digitalWrite(dir_pin_x, direction);

  for (int i = 0; i < steps; i++)
  {
    if (digitalRead(direction == LOW ? stop_switch_pin_x_min : stop_switch_pin_x_max) == LOW)
    {
      return HIGH;
    }

    currentX += (direction == LOW ? -1 : 1);
    step(step_pin_x);
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
  digitalWrite(dir_pin_y, direction);

  for (int i = 0; i < steps; i++)
  {
    if (digitalRead(direction == LOW ? stop_switch_pin_y_min : stop_switch_pin_y_max) == LOW)
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

  digitalWrite(dir_pin_x, directionX);
  digitalWrite(dir_pin_y, directionY);

  // Interleave steps in x and y directions
  int maxSteps = max(stepsX, stepsY);
  for (int i = 0; i < maxSteps; i++)
  {
    if (i < stepsX)
    {
      if (digitalRead(directionX == LOW ? stop_switch_pin_x_min : stop_switch_pin_x_max) == LOW)
      {
        return HIGH;
      }
      currentX += (directionX == LOW ? -1 : 1);
      step(step_pin_x);
    }

    if (i < stepsY)
    {
      if (digitalRead(directionY == LOW ? stop_switch_pin_y_min : stop_switch_pin_y_max) == LOW)
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

  if (dx > 0)
    moveX(dx * STEPS_PER_TILE, HIGH);
  else if (dx < 0)
    moveX(-dx * STEPS_PER_TILE, LOW);

  if (dy > 0
    moveY(dy * STEPS_PER_TILE, HIGH);
  else if (dy < 0)
    moveY(-dy * STEPS_PER_TILE, LOW);

  currentX += dx
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

  int home_timeout = 5000;
  int pause_delay = 100;

  // X_MAX
  startTime = millis();

  while (digitalRead(stop_switch_pin_x_max) == HIGH)
  {
    moveX(1, HIGH);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_X_MAX timeout!");
      return;
    }
  }

  Serial.println("HOME_X_MAX complete!");
  delay(pause_delay);
  Serial.println("HOME_X_MIN...");
  startTime = millis();

  while (digitalRead(stop_switch_pin_x_min) == HIGH)
  {
    maxX += 1; // Increment maxX for each step until we hit the switch
    moveX(1, LOW);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_X_MIN timeout!");
      break;
    }
  }
  currentX = 0;

  Serial.println("HOME_X_MIN complete!");

  // Y_MAX
  delay(pause_delay);
  Serial.println("HOME_Y_MAX...");
  startTime = millis();

  while (digitalRead(stop_switch_pin_y_max) == HIGH)
  {
    moveY(1, HIGH);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_Y_MAX timeout!");
      break;
    }
  }

  Serial.println("HOME_Y_MAX complete!");
  delay(100);
  Serial.println("HOME_Y_MIN...");

  startTime = millis();
  while (digitalRead(stop_switch_pin_y_min) == HIGH)
  {
    maxY += 1; // Increment maxY for each step until we hit the switch
    moveY(1, LOW);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_Y_MIN timeout!");
      return;
      break;
    }
  }

  currentY = 0;

  Serial.println("HOME_Y complete");
  delay(pause_delay);
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
  else if (command == "BLACK" || command == "B")
  {
    dispenseWhite();
  }
  else if (command == "WHITE" || command == "W")
  {
    dispenseBlack();
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
  delayMicroseconds(step_delay);
  L(pin);
  delayMicroseconds(step_delay);
}

void drawImage()
{
  if (!isHomed)
  {
    runCommand("HOME");
    delay(1000);
  }
  else
  {
    moveToTile(0, 0);
  }
  int y = 0;
  int TILE_WIDTH = sizeof(IMAGE_TO_DRAW[0]) / sizeof(IMAGE_TO_DRAW[0][0]);
  int TILE_HEIGHT = sizeof(IMAGE_TO_DRAW) / sizeof(IMAGE_TO_DRAW[0]);
  for (y = 0; y < TILE_HEIGHT; y++)
  {
    for (int x = 0; x < TILE_WIDTH; x++)
    {
      int currentTileColor = IMAGE_TO_DRAW[x][y];
      int nextTileColor = IMAGE_TO_DRAW[x + 1][y];

      moveToTile(x + 3, y + 3);
      if (currentTileColor == 0)
      {
        dispenseBlack(nextTileColor);
      }
      else
      {
        dispenseWhite();
      }
      moveToTile(x, y);
    }
    delay(1000);

    moveToTile(500, 500);
    runCommand("PAUSE");

    moveToTile(x, y + 2);
    delay(500);
    moveToTile(x + 2, y + 2);
    delay(500);
    moveToTile(x + 2, 0);
    delay(500);
  }
}
