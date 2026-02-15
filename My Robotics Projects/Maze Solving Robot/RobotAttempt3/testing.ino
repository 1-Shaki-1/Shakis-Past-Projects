const int LEFT_MOTOR_PIN_1 = 2;
const int LEFT_MOTOR_PIN_2 = 6;
const int RIGHT_MOTOR_PIN_1 = 3;
const int RIGHT_MOTOR_PIN_2 = 11;
const int BUTTON_PIN = 9;
const int LED_PIN = 5;
const int TRIG_PIN = 12;
const int ECHO_PIN = 13;
const int RADAR_PIN = 10;


unsigned long t1;

void forward() {
  analogWrite(LEFT_MOTOR_PIN_1, 200);
  analogWrite(LEFT_MOTOR_PIN_2, 0);
  analogWrite(RIGHT_MOTOR_PIN_1, 200);
  analogWrite(RIGHT_MOTOR_PIN_2, 0);
}

void backward() {
  analogWrite(LEFT_MOTOR_PIN_1, 0);
  analogWrite(LEFT_MOTOR_PIN_2, 200);
  analogWrite(RIGHT_MOTOR_PIN_1, 0);
  analogWrite(RIGHT_MOTOR_PIN_2, 200);
}

void turnLeft() {
  analogWrite(LEFT_MOTOR_PIN_1, 0);
  analogWrite(LEFT_MOTOR_PIN_2, 200);
  analogWrite(RIGHT_MOTOR_PIN_1, 200);
  analogWrite(RIGHT_MOTOR_PIN_2, 0);
}

void turnRight() {
  analogWrite(LEFT_MOTOR_PIN_1, 200);
  analogWrite(LEFT_MOTOR_PIN_2, 0);
  analogWrite(RIGHT_MOTOR_PIN_1, 0);
  analogWrite(RIGHT_MOTOR_PIN_2, 200);
}

void stop() {
  analogWrite(LEFT_MOTOR_PIN_1, 255);
  analogWrite(LEFT_MOTOR_PIN_2, 255);
  analogWrite(RIGHT_MOTOR_PIN_1, 255);
  analogWrite(RIGHT_MOTOR_PIN_2, 255);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LEFT_MOTOR_PIN_1, OUTPUT);
  pinMode(LEFT_MOTOR_PIN_2, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN_1, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN_2, OUTPUT);
  Serial.begin(9600);
}

const int IDLE = 0;
const int START = 1;
const int ON = 2;
const int OFF = 3;
int state = IDLE;


const char* STATE[11] = { "IDLE", "START", "ON", "OFF" };

void loop() {
  // put your main code here, to run repeatedly:
 /* Serial.print(" LED State = ");
  Serial.println(STATE[state]);
  if (state == IDLE) {
    t1 = millis();
    Serial.println(t1);
    state = START;
  }
  if (state == START) {
    if (millis() - t1 > 2000) {
      forward();
      state = ON;
    }
  } */

  
}

