
/*
 Stepper Motor Control -  revolution

 This program drives a unipolar or bipolar stepper motor.
 The motor is attached to digital pins 8 - 11 of the Arduino.

 The motor should revolve one revolution in one direction, then
 one revolution in the other direction.




 */
//Stepper myStepper(stepsPerRevolution,10, 11);

const int switch_pin = A0;
const int step_pin = 10;
const int dir_pin = 11;

const int step_count = 150;
const int step_delay = 4;

void setup() {
  pinMode(switch_pin, INPUT);

  pinMode(step_pin, OUTPUT);
  pinMode(dir_pin, OUTPUT);
  // initialize the serial port:
  Serial.begin(9600);
}

void loop() {
  int val = analogRead(switch_pin);
  Serial.println(String(val));

  // step one revolution  in one direction:
  Serial.println("clockwise");
  digitalWrite(dir_pin, LOW);
  digitalWrite(step_pin, LOW);

  for (int i = 0; i < step_count; i++) {
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(step_pin, LOW);
    delay(step_delay);
  }
  delay(500);
  digitalWrite(dir_pin, HIGH);

  for (int i = 0; i < step_count; i++) {
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(step_pin, LOW);
    delay(step_delay);
  }

  delay(500);

  // step one revolution in the other direction:
  Serial.println("counterclockwise");
  delay(0);
}
