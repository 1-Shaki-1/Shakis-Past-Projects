/// TASKS

#include <Servo.h>
Servo leftMotor;
Servo rightMotor;

// Const Variables

// time variables
unsigned long t_1;
unsigned long t_2;

unsigned long led_t1;
unsigned long led_t2;

// OptoSensor Pins
const int RightSensorPin = A0;
const int MidSensorPin = A1;
const int LeftSensorPin = A2;

// Motor Pins
const int RightMotorPin = 7;
const int LeftMotorPin = 8;

const int turnDelay = 5000;
const int turnTime = 500;

// Device Pins
const int StartButtonPin = 6;
const int CalibrateButtonPin = 4;
const int LedPin = 5;


// Calibration
const int calibrateWaitTime = 5000;
int blackThreshold;
int whiteThreshold;

int calibrationBlinkTime = 500;
int calibrationBlinkTime2 = 250;

int lapCounter = 0;

// Sensor Reading Pins
int LeftSensor;
int MidSensor;
int RightSensor;

// LED Variables
const int standBy_waitTime = 1000;
int brightness = 0;

int ledGlowWaitTime = 4;
int brightIncrement = 1;

// States
int state;
int ledstate;

// Win states
int winBlinkTime = 500;


// P Controller
float Kp = 50;  // turning constant  (use 30)


// Base Motor Speeds
int leftBaseSpeed = 80;  // conservative value not too fast (87 is optimal)
int rightBaseSpeed = 55; // (use 80)



int LED1_ON = 0;
int LED1_OFF = 1;
int BRIGHT = 2;
int DIM = 3;
int LED2_ON = 4;
int LED2_OFF = 5;
int LED3_ON = 6;
int LED3_OFF = 7;

int START = 0;
int IDLE = 1;
int GET_BLACK = 2;
int GET_WHITE = 3;
int FORWARD = 4;
int TURNING = 5;
int TURNING180 = 6;
int WIN = 7;
int CALIBRATION = 8;


// State Table
const char* STATE[20] = {
  "START",
  "IDLE",
  "GET_BLACK",
  "GET_WHITE",
  "FORWARD",
  "TURNING",
  "TURNING180",
  "WIN",
  "CALIBRATION",
};

String LEDSTATE[20] = {
  "LED1_ON",
  "LED1_OFF",
  "BRIGHT",
  "DIM",
  "LED2_ON",
  "LED2_OFF",
  "LED3_ON",
  "LED3_OFF"
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LeftSensorPin, INPUT);
  pinMode(MidSensorPin, INPUT);
  pinMode(RightSensorPin, INPUT);
  pinMode(StartButtonPin, INPUT);
  pinMode(CalibrateButtonPin, INPUT);

  leftMotor.attach(LeftMotorPin);
  rightMotor.attach(RightMotorPin);

  pinMode(LedPin, OUTPUT);

  state = START;
}

void loop() {
  // put your main code here, to run repeatedly:

  int startButtonValue = digitalRead(StartButtonPin);
  int calibrateButtonValue = digitalRead(CalibrateButtonPin);

  // LED Standby Glowing
  if (state == START) {
    state = IDLE;
    ledstate = BRIGHT;
    brightness = 0;
    led_t1 = millis();
  }

  else if (state == IDLE) {
    leftMotor.write(90);  // Motors Stopped
    rightMotor.write(90);


    if (millis() - led_t1 >= ledGlowWaitTime) {
      led_t1 = millis();

      // FADE UP
      if (ledstate == BRIGHT) {
        brightness += brightIncrement;
        if (brightness >= 255) {
          brightness = 255;
          ledstate = DIM;  // Switch direction
        }
      }
      // FADE DOWN
      else if (ledstate == DIM) {
        brightness -= brightIncrement;
        if (brightness <= 0) {
          brightness = 0;
          ledstate = BRIGHT;  // Switch direction
        }
      }
      analogWrite(LedPin, brightness);
    }


    if (startButtonValue == LOW) {
      state = FORWARD;
      analogWrite(LedPin, 255);  // Solid ON when moving
      delay(500);                // Debounce
      t_2 = millis();
      lapCounter = 0;
    }

    if (calibrateButtonValue == LOW) {
      state = CALIBRATION;
      blackThreshold = 0;
      t_1 = millis();
      led_t1 = millis();
      ledstate = LED1_ON;  // Switch to Blink mode
      brightness = 255;
    }
  } else if (state == CALIBRATION) {
    if (millis() - led_t1 >= calibrationBlinkTime) {
      led_t1 = millis();  // Reset timer

      // Toggle between ON and OFF
      if (ledstate == LED1_ON) {
        ledstate = LED1_OFF;
        analogWrite(LedPin, 0);
      } else {
        ledstate = LED1_ON;
        analogWrite(LedPin, 255);
      }
    }
    if (blackThreshold != 0 && millis() - led_t1 >= calibrationBlinkTime2) {
      led_t1 = millis();  // Reset timer

      // Toggle between ON and OFF
      if (ledstate == LED2_ON) {
        ledstate = LED2_OFF;
        analogWrite(LedPin, 0);
      } else {
        ledstate = LED2_ON;
        analogWrite(LedPin, 255);
      }
    }
    if (millis() - t_1 >= calibrateWaitTime) {
      LeftSensor = analogRead(LeftSensorPin);
      MidSensor = analogRead(MidSensorPin);
      RightSensor = analogRead(RightSensorPin);
      if (blackThreshold == 0) {
        blackThreshold = LeftSensor + MidSensor + RightSensor;
        ledstate = LED2_ON;
        led_t1 = millis();
        t_1 = millis();
      } else {
        whiteThreshold = LeftSensor + MidSensor + RightSensor;
        state = IDLE;

        ledstate = BRIGHT;
        brightness = 0;
        led_t2 = millis();
      }
    }
  } else if (state == FORWARD) {

    if (startButtonValue == LOW) {

      state = IDLE;
      delay(500);
    }
    LeftSensor = analogRead(LeftSensorPin);
    MidSensor = analogRead(MidSensorPin);
    RightSensor = analogRead(RightSensorPin);

    int sum = LeftSensor + MidSensor + RightSensor;
    if (millis() - t_2 > turnDelay) {
      if (sum >= (blackThreshold - 100)) {  // Threshold + a margin to ensure all are black
        state = TURNING180;
        lapCounter+= 1;
        t_1 = millis();  // Start the safety timer
      }
    }
    float error = 0;

    if (sum > 0) {
      error = (-1.0 * LeftSensor + 0.0 * MidSensor + 1.0 * RightSensor) / sum;
    }


    int correction = (Kp * error);

    int leftSpeed = leftBaseSpeed - correction;
    int rightSpeed = rightBaseSpeed + correction;

    leftSpeed = constrain(leftSpeed, 0, 90);
    rightSpeed = constrain(rightSpeed, 0, 90);


    rightMotor.write(rightSpeed);
    leftMotor.write(leftSpeed);



  } else if (state == TURNING180) {
    leftMotor.write(86);
    rightMotor.write(110);
    digitalWrite(LedPin, LOW);

    int currentMid = analogRead(MidSensorPin);
    int midSensorBlack = blackThreshold / 3;

    if (millis() - t_1 > turnTime) {


      if (currentMid > (midSensorBlack - 100)) {  // Adding 100 to ensure the opto sensor detects atleast a black trace
        if (lapCounter >= 4) {
          leftMotor.write(90);
          rightMotor.write(90);
          state = WIN;
          ledstate = LED3_ON;
          led_t2 = millis();
        } else {
          state = FORWARD;
          analogWrite(LedPin, 255);
          t_2 = millis();
        }
      }
    }
    if (digitalRead(StartButtonPin) == LOW) {
      leftMotor.write(90);
      rightMotor.write(90);
      state = IDLE;

      delay(500);
    }
  } else if (state == WIN) {

    if (ledstate == LED3_ON && millis() - led_t2 > winBlinkTime) {
      digitalWrite(LedPin, LOW);
      led_t2 = millis();
      ledstate = LED3_OFF;
    }
    if (ledstate == LED3_OFF && millis() - led_t2 > winBlinkTime) {
      digitalWrite(LedPin, HIGH);
      led_t2 = millis();
      ledstate = LED3_ON;
    }
  }
}
