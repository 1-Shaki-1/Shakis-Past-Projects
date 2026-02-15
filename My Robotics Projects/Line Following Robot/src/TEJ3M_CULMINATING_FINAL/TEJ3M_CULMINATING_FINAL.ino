/*
Line-Following Robot
- Uses two motors and a 3-sensor array (Left, Mid, Right) to follow a black line
- Includes PID-style proportional control for smooth turning
- Supports calibration, start button, and LED feedback for states
- Tracks laps and signals WIN after 4 laps
*/

#include <Servo.h>

// Servo objects for controlling left and right motors
Servo leftMotor;
Servo rightMotor;


// Time tracking variables for delays

unsigned long delay_1;
unsigned long delay_2;

// Time tracking for LED updates
unsigned long led_delay1;
unsigned long led_delay2;

// Analog Pins  For OptoSensors
const int RightSensorPin = A0;
const int MidSensorPin = A1;
const int LeftSensorPin = A2;

// Motor Pins
const int RightMotorPin = 7;
const int LeftMotorPin = 8;


// Timing constants for turns
const int turnDelay = 5000; // Wait before checking for lap completion
const int turnTime = 500; // Time to execute 180-degree turn

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
int robotstate;
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
  // Creates a table of states to print out
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
  // Creates a table of led states to print out
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

  Serial.begin(9600);  // Initialize serial communication for debugging

  // Set sensor and button pins as inputs
  pinMode(LeftSensorPin, INPUT);
  pinMode(MidSensorPin, INPUT);
  pinMode(RightSensorPin, INPUT);
  pinMode(StartButtonPin, INPUT);
  pinMode(CalibrateButtonPin, INPUT);

  
  // Attach servo objects to motor pins
  leftMotor.attach(LeftMotorPin);
  rightMotor.attach(RightMotorPin);

  // Set LED pin as output
  pinMode(LedPin, OUTPUT);

  // Initialize robot state to START
  robotstate = START;
}

void loop() {

  // reading start and calibrate button pins
  int startButtonValue = digitalRead(StartButtonPin);
  int calibrateButtonValue = digitalRead(CalibrateButtonPin);

  // START state: initialize LED and move to IDLE
  if (robotstate == START) {
    robotstate = IDLE;
    ledstate = BRIGHT;
    brightness = 0;
    led_delay1 = millis();
  }
   // IDLE state: robot stopped, LED glows in standby
  else if (robotstate == IDLE) {
    leftMotor.write(90); // Stop left motor
    rightMotor.write(90); // Stop right motor

    // LED fade logic (brighten then dim)
    if (millis() - led_delay1 >= ledGlowWaitTime) {
      led_delay1 = millis();

      
      if (ledstate == BRIGHT) {
        brightness += brightIncrement;
        if (brightness >= 255) { // Max brightness
          brightness = 255;
          ledstate = DIM; 
        }
      }
      // FADE DOWN
      else if (ledstate == DIM) {
        brightness -= brightIncrement;
        if (brightness <= 0) { // Min brightness
          brightness = 0;
          ledstate = BRIGHT;   
        }
      }
      analogWrite(LedPin, brightness);
    }

     // Check if start button pressed
    if (startButtonValue == LOW) {
      robotstate = FORWARD;
      analogWrite(LedPin, 255);  // Solid ON when moving
      delay(500);                // Debounce
      delay_2 = millis();
      lapCounter = 0;
    }

    // Check if calibrate button pressed
    if (calibrateButtonValue == LOW) {
      robotstate = CALIBRATION;
      blackThreshold = 0;
      delay_1 = millis();
      led_delay1 = millis();
      ledstate = LED1_ON;  // Switch to Blink mode
      brightness = 255;
    }

    // CALIBRATION state: determine black and white thresholds
  } else if (robotstate == CALIBRATION) {
    // LED blink to indicate calibration
    if (millis() - led_delay1 >= calibrationBlinkTime) {
      led_delay1 = millis();  // Reset timer

      // Toggle between ON and OFF
      if (ledstate == LED1_ON) {
        ledstate = LED1_OFF;
        analogWrite(LedPin, 0);
      } else {
        ledstate = LED1_ON;
        analogWrite(LedPin, 255);
      }
    }
    if (blackThreshold != 0 && millis() - led_delay1 >= calibrationBlinkTime2) {
      led_delay1 = millis();  // Reset timer

      // Toggle between ON and OFF
      if (ledstate == LED2_ON) {
        ledstate = LED2_OFF;
        analogWrite(LedPin, 0);
      } else {
        ledstate = LED2_ON;
        analogWrite(LedPin, 255);
      }
    }
    // Read sensors for calibration after wait time
    if (millis() - delay_1 >= calibrateWaitTime) {
      LeftSensor = analogRead(LeftSensorPin);
      MidSensor = analogRead(MidSensorPin);
      RightSensor = analogRead(RightSensorPin);
      if (blackThreshold == 0) {
        blackThreshold = LeftSensor + MidSensor + RightSensor;
        ledstate = LED2_ON;
        led_delay1 = millis();
        delay_1 = millis();
      } else {
        whiteThreshold = LeftSensor + MidSensor + RightSensor;
        state = IDLE;

        ledstate = BRIGHT;
        brightness = 0;
        led_delay2 = millis();
      }
    }
    // FORWARD state: main line-following logic
  } else if (robotstate == FORWARD) {

    if (startButtonValue == LOW) {

      robotstate = IDLE;
      delay(500);
    }
    LeftSensor = analogRead(LeftSensorPin);
    MidSensor = analogRead(MidSensorPin);
    RightSensor = analogRead(RightSensorPin);

    int sum = LeftSensor + MidSensor + RightSensor;
    // Check if it's time to turn 180

    if (millis() - delay_2 > turnDelay) {
      if (sum >= (blackThreshold - 100)) {  // Threshold + a margin to ensure all are black
        robotstate = TURNING180;
        lapCounter+= 1;
        delay_1 = millis();  // Start the safety timer
      }
    }

    // PID proportional error calculation
    float error = 0;

    if (sum > 0) {
      error = (-1.0 * LeftSensor + 0.0 * MidSensor + 1.0 * RightSensor) / sum;
    }

    // PID proportional error calculation
    int correction = (Kp * error);

    int leftSpeed = leftBaseSpeed - correction;
    int rightSpeed = rightBaseSpeed + correction;

    leftSpeed = constrain(leftSpeed, 0, 90);
    rightSpeed = constrain(rightSpeed, 0, 90);


    rightMotor.write(rightSpeed);
    leftMotor.write(leftSpeed);


    // TURNING180 state: robot turns and faces backwards.
  } else if (robotstate == TURNING180) {
    leftMotor.write(86);
    rightMotor.write(110);
    digitalWrite(LedPin, LOW);

    int currentMid = analogRead(MidSensorPin);
    int midSensorBlack = blackThreshold / 3;

    if (millis() - delay_1 > turnTime) {


      if (currentMid > (midSensorBlack - 100)) {  // Adding 100 to ensure the opto sensor detects atleast a black trace
        if (lapCounter >= 4) {
          leftMotor.write(90);
          rightMotor.write(90);
          robotstate = WIN;
          ledstate = LED3_ON;
          led_delay2 = millis();
        } else {
          robotstate = FORWARD;
          analogWrite(LedPin, 255);
          delay_2 = millis();
        }
      }
    }
    if (digitalRead(StartButtonPin) == LOW) {
      leftMotor.write(90);
      rightMotor.write(90);
      robotstate = IDLE;

      delay(500);
    }
    // WIN state: stops robot and blinks rapidly
  } else if (robotstate == WIN) {

    if (ledstate == LED3_ON && millis() - led_delay2 > winBlinkTime) {
      digitalWrite(LedPin, LOW);
      led_delay2 = millis();
      ledstate = LED3_OFF;
    }
    if (ledstate == LED3_OFF && millis() - led_delay2 > winBlinkTime) {
      digitalWrite(LedPin, HIGH);
      led_delay2 = millis();
      ledstate = LED3_ON;
    }
  }
}
