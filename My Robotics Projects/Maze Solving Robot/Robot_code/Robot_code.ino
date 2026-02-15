
#include <Servo.h>

// Variables
const int LEFT_MOTOR_PIN_1 = 2;
const int LEFT_MOTOR_PIN_2 = 6;
const int RIGHT_MOTOR_PIN_1 = 3;
const int RIGHT_MOTOR_PIN_2 = 11;
const int BUTTON_PIN = 8;
const int LED_PIN = 5;
const int TRIG_PIN = 12;
const int ECHO_PIN = 13;
const int RADAR_PIN = 10;

Servo myservo;
const int TIME = 10000;


unsigned long t_1;
unsigned long t1;
unsigned long t;
// STATES
const int IDLE = 0;
const int FORWARD = 1;
const int LEFT = 2;
const int RIGHT = 3;
const int STOP = 4;
const int RADAR_LEFT = 5; // NOT USED
const int RADAR_RIGHT = 6; // NOT USED
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

// create state table
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

// Setting up default states

int state = IDLE;
int buttonValue;
int ledstate = IDLE;

// Defining the minimum distance to turn and the time it takes to turn
const int MIN_DISTANCE = 10;
const int MAX_DISTANCE = 100;
const int TURN_TIME = 200;


// LED on and off times
const int LED1_ON_TIME = 1000;
const int LED1_OFF_TIME = 1000;
const int LED3_ON_TIME = 133;
const int LED3_OFF_TIME = 200;

// getting distance
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

// Robot controlls
void forward() {
  analogWrite(LEFT_MOTOR_PIN_1, 200);
  analogWrite(LEFT_MOTOR_PIN_2, 0);
  analogWrite(RIGHT_MOTOR_PIN_1, 160);
  analogWrite(RIGHT_MOTOR_PIN_2, 0);
}



void turnLeft() {
  analogWrite(LEFT_MOTOR_PIN_1, 0);
  analogWrite(LEFT_MOTOR_PIN_2, 100);
  analogWrite(RIGHT_MOTOR_PIN_1, 100);
  analogWrite(RIGHT_MOTOR_PIN_2, 0);
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
  myservo.attach(RADAR_PIN);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Print statements
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


  // LED handling
  // LED 1
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
    if (millis() - t1 > LED1_ON_TIME) {
      t1 = millis();
      digitalWrite(LED_PIN, LOW);
      ledstate = LED_OFF_1;
    }
  } else if (ledstate == LED_OFF_1) {
    if (millis() - t1 > LED1_OFF_TIME) {
      t1 = millis();
      digitalWrite(LED_PIN, HIGH);
      ledstate = LED_ON_1;
    }
  }

  // LED 3
  if (ledstate == WIN) {
    t1 = millis();
    ledstate = led3on;
  } else if (ledstate == led3on) {
    if (millis() - t1 > LED3_ON_TIME) {
      t1 = millis();
      digitalWrite(LED_PIN, HIGH);
      ledstate = LED_ON_3;
    }
  } else if (ledstate == LED_ON_3) {
    if (millis() - t1 > LED3_OFF_TIME) {
      t1 = millis();
      digitalWrite(LED_PIN, LOW);
      ledstate = LED_OFF_3;
    }
  } else if (ledstate == LED_OFF_3) {
    if (millis() - t1 > LED3_ON_TIME) {
      t1 = millis();
      digitalWrite(LED_PIN, HIGH);
      ledstate = LED_ON_3;
    }
  }


  // Motor Handling
  buttonValue = digitalRead(BUTTON_PIN);
  if (buttonValue == LOW && state == IDLE) {
    forward();
    state = FORWARD;
    t_1 = millis();
  } else if (buttonValue == LOW && state != FORWARD) {
    state = IDLE;
    stop();
  } else if (state == FORWARD) {
    if (millis() - t_1 > TIME) {
      stop();
      turnLeft();
      t_1 = millis();
      state = CHECK_LEFT;
    }
  
    // Sensor
    if (getDistance() < MIN_DISTANCE) {
      turnLeft();
      t = millis();
      state = LEFT;
      Serial.println(t);
    }

    // if this state occured
  } else if (state == LEFT) {
    if (millis() - t > TURN_TIME) {
      if (getDistance() > MIN_DISTANCE) {
        state = START;
      }
    }
  } else if (state == CHECK_LEFT)  {
    if (millis() - t_1 > TURN_TIME*4) {
      if (getDistance() > MAX_DISTANCE){
      ledstate = WIN;
      state = WIN;
      stop();
      }
      else if (getDistance() < MAX_DISTANCE){
        forward();
        state = FORWARD;
      }
    }
  } else if (state == START) {
    forward();
    state = FORWARD;
  }
}