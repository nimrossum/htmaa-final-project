// DIR = GREEN, 0
// STEP = GREY, 1

#define dir_pin_x 4   // GREEN
#define step_pin_x 5 // GREY

#define dir_pin_y 6   // GREEN
#define step_pin_y 7  // GREY

#define step_count 200
#define step_delay 800

void setup() {

  pinMode(step_pin_x, OUTPUT);
  pinMode(dir_pin_x, OUTPUT);
  pinMode(step_pin_y, OUTPUT);
  pinMode(dir_pin_y, OUTPUT);
  // initialize the serial port:
  Serial.begin(9600);
}

void loop() {
  runMotorBackAndForth(step_pin_y, dir_pin_y);
  delay(500);
  runMotorBackAndForth(step_pin_x, dir_pin_x);
  delay(500);
}

void runMotorBackAndForth(int step_pin, int dir_pin) {
  L(step_pin);
  L(dir_pin);

  for (int i = 0; i < step_count; i++) {
    H(step_pin);
    delayMicroseconds(step_delay);
    L(step_pin);
    delayMicroseconds(step_delay);
  }

  delay(500);
  digitalWrite(dir_pin, HIGH);


  for (int i = 0; i < step_count; i++) {
    H(step_pin);
    delayMicroseconds(step_delay);
    L(step_pin);
    delayMicroseconds(step_delay);
  }

  delay(500);
}

void H(int pin) {
  digitalWrite(pin, HIGH);
}

void L(int pin) {
  digitalWrite(pin, LOW);
}
