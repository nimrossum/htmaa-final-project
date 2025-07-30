const int stop_switch_pin = A0;
const int stop_switch_pin2 = A1;

int stopSwitchDown = LOW;
int stopSwitchDown2 = LOW;

const int step_pin1 = 10;
const int dir_pin1 = 11;

const int step_pin2 = 5;
const int dir_pin2 = 6;

const int step_count = 1;
int step_delay = 800;

const int dir_change_delay = 300;

int stopped = LOW;
int dir = LOW;

int stepsSinceLastStop = 0;


void setup() {
  pinMode(stop_switch_pin, INPUT);
  pinMode(stop_switch_pin2, INPUT);

  pinMode(step_pin1, OUTPUT);
  pinMode(dir_pin1, OUTPUT);

  pinMode(step_pin2, OUTPUT);
  pinMode(dir_pin2, OUTPUT);
  Serial.begin(9600);
  delay(5000);
}

int stopSwitchVal = HIGH;
int didSwitch = LOW;

void loop() {
  int stopSwitchVal = digitalRead(stop_switch_pin);
  int stopSwitchVal2 = digitalRead(stop_switch_pin2);


  if (stopSwitchVal == HIGH && stopSwitchDown == LOW) {
    stopSwitchDown = HIGH;
  }

  if (stopSwitchVal == LOW && stopSwitchDown == HIGH) {
    stopSwitchDown = LOW;
    dir = HIGH;
    didSwitch = HIGH;
  }

  // Switch closest to idler
  if (stopSwitchVal2 == HIGH && stopSwitchDown2 == LOW) {
    stopSwitchDown2 = HIGH;
  }

  if (stopSwitchVal2 == LOW && stopSwitchDown2 == HIGH) {
    stopSwitchDown2 = LOW;
    dir = LOW;
    didSwitch = HIGH;
  }


  digitalWrite(dir_pin1, dir);
  digitalWrite(dir_pin2, dir);

  // Step primary motor
  digitalWrite(step_pin1, HIGH);
  delayMicroseconds(step_delay);
  digitalWrite(step_pin1, LOW);
  delayMicroseconds(step_delay);

  // Step secondary motor
  digitalWrite(step_pin2, HIGH);
  delayMicroseconds(step_delay);
  digitalWrite(step_pin2, LOW);
  delayMicroseconds(step_delay);

  stepsSinceLastStop += 1;

  if (didSwitch == HIGH) {

    delay(dir_change_delay);
    // Serial.println("Hit a switch, pausing for" + String(dir_change_delay));
    // Serial.println("Continuing");
    Serial.println("Steps: " + String(stepsSinceLastStop));
    didSwitch = LOW;
    stepsSinceLastStop = 0;
  }
}

void togglePin(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}