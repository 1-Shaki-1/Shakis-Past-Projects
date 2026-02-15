




#include <Servo.h>
// Motor Variables
Servo LeftMotor;
Servo RightMotor;

// Const Variables

// time variables
unsigned long t_1;
unsigned long t_2;


// OptoSensor Pins
const int LeftSensorPin = 0;
const int MidSensorPin = 1;
const int RightSensorPin = 2;

// Motor Pins
const int RightMotorPin = 8;
const int LeftMotorPin = 7;
const int turnTime = 500;

// Device Pins
const int StartButtonPin = 6;
const int CalibrateButtonPin = 4;
const int LedPin = 9;


// Calibration
const int calibrateWaitTime = 5000;
int blackThreshold;
int whiteThreshold;

int calibrationBlinkTime = 500;
int calibrationBlinkTime2 = 250;


// Sensor Reading Pins
int LeftSensor;
int MidSensor;
int RightSensor;

// LED Variables
const int standBy_waitTime = 1000;
int brightness = 0;
unsigned long led_t1;
unsigned long led_t2;
int ledGlowWaitTime = 100;
int brightIncrement = 5;

// States
int state;
int ledstate;

int BRIGHT = 0;
int DIM = 1;
int LED_ON = 2;
int LED_OFF = 3;

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

const char* LEDSTATE[20] = {
  "LED_ON",
  "LED_OFF",
  "BRIGHT",
  "DIM"
};


void setup() {
  Serial.begin(9600);
  pinMode(LeftSensorPin, INPUT);
  pinMode(MidSensorPin, INPUT);
  pinMode(RightSensorPin, INPUT);
  pinMode(StartButtonPin, INPUT);
  pinMode(CalibrateButtonPin, INPUT);

  state = IDLE;
  ledstate = BRIGHT;

  LeftMotor.attach(LeftMotorPin);
  RightMotor.attach(RightMotorPin);
  // put your setup code here, to run once:
  led_t1 = millis();
}


// Functions
void moveForward() {
  LeftMotor.write(180);
  RightMotor.write(0);
}

void moveBackward() {
  LeftMotor.write(0);
  RightMotor.write(180);
}

void stop() {
  LeftMotor.write(90);
  RightMotor.write(90);
}

void turnLeft() {
  LeftMotor.write(180);
  RightMotor.write(180);
}

void turnRight() {
  LeftMotor.write(0);
  RightMotor.write(180);
}

void turnAround180() {


  if (millis() - t_2 < turnTime) {
    LeftMotor.write(180);
    RightMotor.write(180);
  }

  else {
    MidSensor = analogRead(MidSensorPin);  // Update sensor reading

    if (MidSensor > blackThreshold) {
      // Line found! Resume following
      state = START;
    } else {
      // Still searching for the line
      LeftMotor.write(180);
      RightMotor.write(180);
    }
  }
}







void getWhite() {
  LeftSensor = analogRead(LeftSensorPin);
  MidSensor = analogRead(MidSensorPin);
  RightSensor = analogRead(RightSensorPin);

  whiteThreshold = (LeftSensor + RightSensor + MidSensor);
}



void getBlack() {
  t_1 = millis();
  if (millis() - t_1 > calibrateWaitTime) {
    LeftSensor = analogRead(LeftSensorPin);
    MidSensor = analogRead(MidSensorPin);
    RightSensor = analogRead(RightSensorPin);

    blackThreshold = (LeftSensor + RightSensor + MidSensor);
  }
}

void P_Controller() {
  LeftSensor = analogRead(LeftSensorPin);
  MidSensor = analogRead(MidSensorPin);
  RightSensor = analogRead(RightSensorPin);

  int sum = LeftSensor + MidSensor + RightSensor;
  float error = 0;

  if (sum > 0) {
    error = (-1.0 * LeftSensor + 0.0 * MidSensor + 1.0 * RightSensor) / sum;
  }

  float Kp = 40;        // turning constant
  int baseSpeed = 130;  // conservative value not too fast ( 255 is max )

  int correction = Kp * error;

  int leftSpeed = baseSpeed - correction;
  int rightSpeed = baseSpeed + correction;

  leftSpeed = constrain(leftSpeed, 90, 180);
  rightSpeed = constrain(rightSpeed, 90, 180);


  setMotorSpeeds(leftSpeed, rightSpeed);
}

void setMotorSpeeds(int leftMotorSpeed, int rightMotorSpeed) {

  leftMotorSpeed = constrain(leftMotorSpeed, 0, 180);
  rightMotorSpeed = constrain(rightMotorSpeed, 0, 180);

  LeftMotor.write(leftMotorSpeed);
  RightMotor.write(180 - rightMotorSpeed);
}

void calibrationBlink() {


  digitalWrite(LedPin, HIGH);
  ledstate = LED_ON;
  led_t1 = millis();
  if (millis() - led_t1 > calibrationBlinkTime) {
    digitalWrite(LedPin, LOW);
    ledstate = LED_OFF;
  }
  led_t1 = millis();
  if (millis() - led_t1 > calibrationBlinkTime) {
    digitalWrite(LedPin, HIGH);
    ledstate = LED_ON;
  }

  if (millis() - led_t2 > calibrateWaitTime) {
    led_t2 = millis();
    digitalWrite(LedPin, HIGH);
    ledstate = LED_ON;
    led_t1 = millis();
    if (millis(); - led_t1 > calibrationBlinkTime2) {
      digitalWrite(LedPin, LOW);
      ledstate = LED_OFF;
      led_t1 = millis();
    }
    led_t1 = millis();
    if (millis() - led_t1 > calibrationBlinkTime2) {
      digitalWrite(LedPin, HIGH);
      ledstate = LED_ON;
    }
  }
  if (millis() - led_t2 > calibrateWaitTime) {
    state = IDLE;
    ledstate = LED_OFF;
  }
}



void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("STATE: ");
  Serial.print(STATE[state]);
  Serial.print(" ");
  Serial.print("LED STATE: ");
  Serial.println(LEDSTATE[ledstate]);
  Serial.println(brightness);

  int startButtonValue = digitalRead(StartButtonPin);
  int calibrateButtonValue = digitalRead(CalibrateButtonPin);


  if (state == IDLE) {
    Standby();


    if (startButtonValue == LOW) {
      state = START;
      delay(200);
    }
    if (calibrateButtonValue == LOW) {
      state = CALIBRATION;
      t_2 = millis();
    }
  } else if (state == START) {
    P_Controller();
    if (LeftSensor > blackThreshold && MidSensor > blackThreshold && RightSensor > blackThreshold) {
      t_2 = millis();
      state = TURNING180;
    }
    if (startButtonValue == LOW) {
      state = IDLE;
      delay(200);
    }
  } else if (state == TURNING180) {
    turnAround180();
    if (startButtonValue == LOW) {
      state = IDLE;
      delay(200);
    }
  }





  if (state == CALIBRATION) {
    calibrationBlink();
    getBlack();
    t_2 = millis();
    if (millis() - t_2 > calibrateWaitTime) {
      getWhite();
      t_2 = millis();
      if (millis() - t_2 > calibrateWaitTime) {
        state = IDLE;
      }
    }
  }
}





// Standby mode

// You are to design a program where the led fades in and when it reaches the max brightnes it should dim






///18 multiple choice 9 written answers
