#include <Servo.h>

// Variables
const int LEFT_MOTOR_PIN_1 = 2;
const int LEFT_MOTOR_PIN_2 = 6;
const int RIGHT_MOTOR_PIN_1 = 3;
const int RIGHT_MOTOR_PIN_2 = 11;
const int BUTTON_PIN = 9;
const int LED_PIN = 5;
const int TRIG_PIN = 12;
const int ECHO_PIN = 13;
const int RADAR_PIN = 10;


const int TIME = 2000;


// Defining servo motor (Radar)
Servo myservo;

// STATES
const int IDLE = 0;
const int FORWARD = 1;
const int LEFT = 2;
const int RIGHT = 3;
const int STOP = 4;
const int RADAR_LEFT = 5;
const int RADAR_RIGHT = 6;
const int LED_ON_1 = 7;
const int LED_OFF_1 = 8;
const int LED_ON_3 = 9;
const int LED_OFF_3 = 10;
const int led1on = 11;
const int led3on = 12;
const int START = 13;
const int WIN = 14;
const int CHECK = 15;
const int CHECK_LEFT = 16;
const int CHECK_RIGHT = 17;
const char* STATE[30] = {
  "IDLE",
  "FORWARD",
  "LEFT",
  "RIGHT",
  "STOP",
  "RADAR_LEFT",
  "RADAR_RIGHT",
  "LED_1_ON",
  "LED_1_OFF",
  "LED_3_ON",
  "LED_3_OFF",
  "led1_start",
  "led3_start",
  "START",
  "WIN",
  "CHECK",
  "CHECK_LEFT",
  "CHECK_RIGHT"
};

// Defining the minimum distance to turn and the time it takes to turn
const int MIN_DISTANCE = 15;
const int TURN_TIME = 2000;

int state = IDLE;
int ledstate = IDLE;

int angle = 0;

// Duty cycles
const int FLASH_1 = 1;
const int FLASH_3 = 3;
const int DUTY_1 = 50;
const int DUTY_2 = 40;

// Determine wait times for LEDs
// LED_1
const int LED_1_PERIOD = 2000 / FLASH_1;
const int LED_1_ON_TIME = LED_1_PERIOD * DUTY_1 / 100;
const int LED_1_OFF_TIME = LED_1_PERIOD * (100 - DUTY_1) / 100;

// LED_3
const int LED_3_PERIOD = 1000 / FLASH_3;
const float LED_3_ON_TIME = LED_3_PERIOD * DUTY_2 / 100;
const float LED_3_OFF_TIME = LED_3_PERIOD * (100 - DUTY_2) / 100;

unsigned long t1;
int t_1;

unsigned long t2;
unsigned long t_2;
int buttonValue;

/*
Returns the distance (in cm) of the nearest object
*/
int getDistance() {
  unsigned long duration;
  // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  //Serial.println(duration);
  // Calculating the distance
  int distance = (duration * 0.034) / 2;
  // Return the distance
  return (distance);
}

// Robot controls

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

void no() {
  analogWrite(LEFT_MOTOR_PIN_1, 255);
  analogWrite(LEFT_MOTOR_PIN_2, 255);
  analogWrite(RIGHT_MOTOR_PIN_1, 255);
  analogWrite(RIGHT_MOTOR_PIN_2, 255);
}

void radarturn_left() {
  /*for (angle = 0; angle <= 150; angle += 1) {
    myservo.write(angle);
    delay(10);
  }
  if (angle == 150){
    delay(1000);
  }*/
  myservo.write(150);
}

void radarturn_right() {
  /*for (angle = 150; angle >= -5; angle -= 1) {
    myservo.write(angle);
    delay(10);
  }*/
  myservo.write(0);
}

void setup() {
  // put your setup code here, to run once:
  ;
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LEFT_MOTOR_PIN_1, OUTPUT);
  pinMode(LEFT_MOTOR_PIN_2, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN_1, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN_2, OUTPUT);
  Serial.begin(9600);
  myservo.attach(RADAR_PIN);
}

void loop() {
  // put your main code here, to run repeatedly:
  myservo.write(60);
  buttonValue = digitalRead(BUTTON_PIN);
  int distance = getDistance();
  Serial.print("Distance = ");
  Serial.print(distance);
  Serial.print(" Button Value = ");
  Serial.print(buttonValue);
  Serial.print(" Motor State = ");
  Serial.println(STATE[state]);
  Serial.print(" LED State =  ");
  Serial.println(STATE[ledstate]);

  if (ledstate == IDLE) {
    t1 = millis();
    ledstate = led1on;
  } else if (ledstate == led1on) {
    if (millis() - t1 > 1000) {
      t1 = millis();
      digitalWrite(LED_PIN, HIGH);
      ledstate = LED_ON_1;
    }
  } else if (ledstate == LED_ON_1) {
    if (millis() - t1 > 1000) {
      t1 = millis();
      digitalWrite(LED_PIN, LOW);
      ledstate = LED_OFF_1;
    }
  } else if (ledstate == LED_OFF_1) {
    if (millis() - t1 > 1000) {
      t1 = millis();
      digitalWrite(LED_PIN, HIGH);
      ledstate = LED_ON_1;
    }
  }
  if (state == IDLE && buttonValue == LOW) {
    forward();
    state = FORWARD;
    t2 = millis();
  } else if (state == FORWARD && buttonValue == LOW) {
    no();
    state = IDLE;
  } else if (state == FORWARD) {

    if (millis() - t2 > TIME) {
      no();
      state = CHECK_LEFT;
      radarturn_left();
      t_2 = millis();
    }
  } else if (state == CHECK_LEFT) {
    if (getDistance() > 100) {
      state = CHECK_RIGHT;
      radarturn_right();
    } else if (getDistance() < 100) {
      myservo.write(60);
      forward();
      state = FORWARD;
    }
  } else if (state == CHECK_RIGHT) {
    if (getDistance() > 100) {
      forward();
      state = FORWARD;
    } else if (getDistance() < 100) {
      myservo.write(60);
      state = FORWARD;
      forward();
    }
  }
}

