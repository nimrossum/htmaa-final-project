#include <Servo.h>

#define resume_switch_pin A0

#define stop_switch_pin_x_min A1
#define stop_switch_pin_y_min A2

#define stop_switch_pin_x_max A3
#define stop_switch_pin_y_max A4

// GREEN WIRE AWAY
#define dir_pin_x 4
#define step_pin_x 5

// RED WIRE UP
#define dir_pin_y 6
#define step_pin_y 7

#define stepDelay 800 // microseconds (movement speed

// TODO: Calibrate this:
#define stepsPerTile 46 // steps for one tile distanc

#define servoWhitePin 11
#define servoBlackPin 10

#define SERVO_MAX_BLACK 0
#define SERVO_MIN_BLACK 178

#define SERVO_MAX_WHITE 0
#define SERVO_MIN_WHITE 178

int currentX = 0;
int currentY = 0;

// int x_max = 0;
// int y_max = 0;
int x_max = 985;
int y_max = 846;

Servo servoWhite;
Servo servoBlack;

String commands[] = {
    "HOME",
    "X1Y1",
    // "HOME",
    // "PAUSE",
    // "HOME",
    // "PAUSE",
    // "BLACK",
    // "WHITE"
    // "HOME",
    // "PAUSE",
    // "X10Y0",
    // "PAUSE",
    // "BLACK",
    // "PAUSE",
    // "X8Y0",
    // "X12Y5",
    // "X5Y5",
    // "X5Y0",
    // "HOME"
};

int commandCount = sizeof(commands) / sizeof(commands[0]);
int commandIndex = 0;

bool shouldExecuteCommandsAtLaunch = false;

void setup()
{
  Serial.begin(9600);

  // Switch pins
  pinMode(resume_switch_pin, INPUT);

  pinMode(stop_switch_pin_x_min, INPUT_PULLUP);
  pinMode(stop_switch_pin_y_min, INPUT_PULLUP);

  pinMode(stop_switch_pin_x_max, INPUT_PULLUP);
  pinMode(stop_switch_pin_y_max, INPUT_PULLUP);

  // Stepper pins
  pinMode(step_pin_x, OUTPUT);
  pinMode(dir_pin_x, OUTPUT);

  pinMode(step_pin_y, OUTPUT);
  pinMode(dir_pin_y, OUTPUT);

  // Initialize servos
  servoWhite.attach(servoWhitePin);
  servoBlack.attach(servoBlackPin);

  servoWhite.write(SERVO_MAX_BLACK);
  servoBlack.write(SERVO_MIN_WHITE);

  String cmds = "";
  for (int i = 0; i < commandCount; i++)
  {
    cmds += commands[i];
    if (i < commandCount - 1)
      cmds += ", ";
  }
  Serial.println("System Ready. Send commands: " + cmds);
}

void dispenseWhite()
{
  Serial.println("Dispensing white");
  servoWhite.write(SERVO_MIN_WHITE);
  delay(1000);
  servoWhite.write(SERVO_MAX_WHITE);
  delay(1000);
}

void dispenseBlack()
{
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
      if (input == "START")
      {
        Serial.println("Starting command sequence...");
        shouldExecuteCommandsAtLaunch = true;
      }
      else {
        runCommand(input);
      }
    }
    // ...or start button press
    if (digitalRead(resume_switch_pin) == LOW)
    {
      Serial.println("Starting command sequence (A4 pressed)...");

      // Wait for button release to avoid immediate PAUSE trigger later
      while (digitalRead(resume_switch_pin) == LOW)
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
  digitalWrite(dir_pin_y, direction);

  for (int i = 0; i < steps; i++)
  {
    if (digitalRead(direction == LOW ? stop_switch_pin_y_min : stop_switch_pin_y_max) == LOW)
    {
      return HIGH;
    }

    currentY += (direction == LOW ? -1 : 1);
    step(step_pin_y);
  }

  return LOW;
}

void moveToTile(int x, int y)
{
  int dx = x - currentX;
  int dy = y - currentY;

  if (dx > 0)
    moveX(dx * stepsPerTile, HIGH);
  else if (dx < 0)
    moveX(-dx * stepsPerTile, LOW);

  if (dy > 0)
    moveY(dy * stepsPerTile, HIGH);
  else if (dy < 0)
    moveY(-dy * stepsPerTile, LOW);

  currentX = x;
  currentY = y;

  printCurrentPos();

  // Serial.print("Moved to tile: ");
  // Serial.print(currentX);
  // Serial.print(",");
  // Serial.println(currentY);
}

// ==================== HOMING ====================
void homeAll()
{

  x_max = 0;
  y_max = 0;
  Serial.println("HOME_X_MIN...");
  unsigned long startTime = millis();

  int home_timeout = 5000;

  while (digitalRead(stop_switch_pin_x_min) == HIGH)
  { // HIGH = not pressed (because INPUT_PULLUP)
    moveX(1, LOW);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_X_MIN timeout!");
      break;
    }
  }
  // We are at min x, so reset position
  currentX = 0;

  Serial.println("HOME_X_MIN complete, resetting currentX!");
  Serial.println("HOME_X_MAX...");
  delay(500);
  startTime = millis();

  while (digitalRead(stop_switch_pin_x_max) == HIGH)
  {
    moveX(1, HIGH);
    x_max += 1; // Increment x_max for each step until we hit the switch
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_X_MAX timeout!");
      return;
    }
  }

  Serial.println("HOME_X_MAX complete!");
  delay(500);
  Serial.println("HOME_Y_MIN...");

  startTime = millis();
  while (digitalRead(stop_switch_pin_y_min) == HIGH)
  {
    moveY(1, LOW);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_Y_MIN timeout!");
      return;
      break;
    }
  }

  // We are at MIN_Y, so reset currentY
  currentY = 0;

  Serial.println("HOME_Y_MIN complete, resetting currentY!");
  delay(500);
  Serial.println("HOME_Y_MAX...");

  startTime = millis();
  while (digitalRead(stop_switch_pin_y_max) == HIGH)
  {
    moveY(1, HIGH);
    y_max += 1; // Increment y_max for each step until we hit the switch
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_Y_MAX timeout!");
      break;
    }
  }

  Serial.println("HOME_Y_MAX complete!");

  Serial.println("Homing succesful!");
  Serial.println("X_MAX: " + String(x_max));
  Serial.println("Y_MAX: " + String(y_max));

  delay(500);

  startTime = millis();
  while (digitalRead(stop_switch_pin_x_min) == HIGH)
  { // HIGH = not pressed (because INPUT_PULLUP)
    moveX(1, LOW);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_X_MIN timeout!");
      break;
    }
  }

  delay(500);

  startTime = millis();
  while (digitalRead(stop_switch_pin_y_min) == HIGH)
  { // HIGH = not pressed (because INPUT_PULLUP)
    moveY(1, LOW);
    if (millis() - startTime > home_timeout)
    {
      Serial.println("HOME_X_MIN timeout!");
      break;
    }
  }

  Serial.println("currentX: " + String(currentX));
  Serial.println("currentY: " + String(currentY));
}

void runCommand(String command)
{
  command.trim();
  command.toUpperCase();

  if (command == "HOME")
  {
    homeAll();
  }
  else if (command == "PAUSE")
  {
    waitForResume();
  }
  else if (command.startsWith("X") && command.indexOf("Y") != -1)
  {
    int xIndex = 1;
    int yIndex = command.indexOf("Y") + 1;
    int x = command.substring(xIndex, command.indexOf("Y")).toInt();
    int y = command.substring(yIndex).toInt();
    moveToTile(x, y);
  }
  else if (command == "BLACK")
  {
    dispenseBlack();
  }
  else if (command == "WHITE")
  {
    dispenseWhite();
  }
  else
  {
    Serial.println("Unknown command!");
  }
}

void waitForResume()
{
  Serial.println("Paused. Press resume button..");
  while (digitalRead(resume_switch_pin) == HIGH)
  {
    delay(10); // debounce wait
  }
  while (digitalRead(resume_switch_pin) == LOW)
  {
    delay(10); // wait until released
  }
  Serial.println("Resumed!");
}

// ==================== SERIAL COMMANDS ====================
void handleSerialCommands()
{
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    runCommand(command);
  }
}

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
  delayMicroseconds(stepDelay);
  L(pin);
  delayMicroseconds(stepDelay);
}
