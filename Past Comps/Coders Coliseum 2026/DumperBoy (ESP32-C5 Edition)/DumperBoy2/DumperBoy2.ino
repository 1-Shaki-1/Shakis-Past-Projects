#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "index.h"
#include <MadgwickAHRS.h>

Madgwick filter;

// --- Credentials, Server & Endpoints ---
// const char* ssid = "SM-S908W6045";
// const char* password = "siva2024";

// const char* ssid = "aakesh";
// const char* password = "12345678";


const char* ssid = "BELL769";
const char* password = "C5C2F99DF6A3";
String url = "https://script.google.com/macros/s/AKfycbysuCBdJgK2PS7Itz93D_zxq67O36YJzP4QQbj6ne15pUHwmHijMAkusv6ijg7Q9Rlu/exec?";
WebServer server(80);

// --- Manual Mode Tracking ---
bool isManual = false;
char currentDir = 'S';  // Tracks F, B, L, R, S

// --- Pin Definitions ---
const int LEFT_PWM = 6;
const int LEFT_IN1 = 4;
const int LEFT_IN2 = 5;
const int RIGHT_PWM = 10;
const int RIGHT_IN1 = 11;
const int RIGHT_IN2 = 12;

const int TB_STBY = 7;

const int MPU_SCL = 23;
const int MPU_SDA = 24;

const int DUMPER_PIN = 15;

bool finalTurn = false;

enum STATE {
  START,
  FORWARD,
  STOP_CAR,
  UNLOAD,
  TURNING,
  FORWARD_RETURNING,
  FINISH,
  LOWER_DUMPER
};

STATE state = START;

// --- Objects & Globals ---
MPU6050 mpu;
Servo dumper;
HardwareSerial Sensor(2);

const int MAX_SAFE_SPEED = 230;
int turnSpeed = 150;
int turnSpeedM = 150;
long sensorDistance = 999;
int minDist = 30;

const int SONAR_RX = 9;
const int SONAR_TX = 8;

const int ONBOARD_LED = 2;

float currentError = 0.0;
float gyroOffset = 0.0;
unsigned long previousTime = 0;

int16_t ax, ay, az, gx, gy, gz;

const int ANGLE_CAPTURE_CD = 5;
float turnAngle = 170.0;

int leftBaseSpeed = 180;
int rightBaseSpeed = 180;

float k_p = .12;
float k_i = 0.00;
float k_d = 0.001;
float headingIntegral = 0.0;
float previousError = 0.0;

bool roverActive = false;



unsigned long t1, t2;
unsigned long start_run;
unsigned long run_dist;
const int STOP_TIME = 1000;
const int DUMP_TIME = 4000;

int currentServoPos = 140;  // Starting retracted position
int targetServoPos = 140;

const int RETRACT = 30;
const int DUMP = 140;

unsigned long lastServoUpdate = 0;
const int servoInterval = 15;  // Milliseconds between each step (adjust for speed)

void setup() {
  Serial.begin(115200);
  filter.begin(50.0);

  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  pinMode(TB_STBY, OUTPUT);
  digitalWrite(TB_STBY, HIGH);

  Wire.begin(MPU_SDA, MPU_SCL);
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
    calibrateAngle();
  } else {
    Serial.println("MPU6050 connection failed! Check wiring.");
  }

  // --- Wi-Fi Connection with Timeout ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {  // 10 second timeout limit
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("Dashboard IP: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed! Proceeding with offline setup...");
  }

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // --- Web Server Routes ---
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/calibrate", handleCalibration);
  server.on("/set", handleParamUpdate);
  server.on("/mode", []() {
    if (server.hasArg("manual")) {
      isManual = (server.arg("manual") == "1");
      if (isManual) {
        roverActive = false;
        setMotors(0, 0);
        currentDir = 'S';
      }
    }
    server.send(200, "text/plain", "Mode updated");
  });

  server.on("/move", []() {
    if (server.hasArg("dir")) {
      char prevDir = currentDir;
      currentDir = server.arg("dir")[0];

      // Reset PID error tracking whenever exiting a turn or stopping
      if ((prevDir == 'L' || prevDir == 'R') && (currentDir == 'S' || currentDir == 'F' || currentDir == 'B')) {
        currentError = 0.0;
        headingIntegral = 0.0;
        previousError = 0.0;
        previousTime = millis();
      }
    }
    server.send(200, "text/plain", "Move updated");
  });

  server.on("/dumper", []() {
    if (server.hasArg("state")) {
      bool dumpState = (server.arg("state") == "1");
      dump(dumpState);
    }
    server.send(200, "text/plain", "Dumper updated");
  });

  server.begin();
  Serial.println("HTTP server started");

  // --- Peripheral Setup ---
  Sensor.begin(9600, SERIAL_8N1, SONAR_RX, SONAR_TX);
  dumper.setPeriodHertz(50);
  dumper.attach(DUMPER_PIN, 500, 2400);
  dumper.write(RETRACT);

  // Send email notification only if Wi-Fi connected successfully
  if (WiFi.status() == WL_CONNECTED) {
    sendEmail();
  }

  previousTime = millis();
}

void setMotors(int leftSpeed, int rightSpeed) {

  if (leftSpeed < 0) leftSpeed = constrain(leftSpeed * 0.80, -MAX_SAFE_SPEED, 0);
  if (rightSpeed < 0) rightSpeed = constrain(rightSpeed * 0.80, -MAX_SAFE_SPEED, 0);

  leftSpeed = constrain(leftSpeed, -MAX_SAFE_SPEED, MAX_SAFE_SPEED);
  rightSpeed = constrain(rightSpeed, -MAX_SAFE_SPEED, MAX_SAFE_SPEED);

  if (leftSpeed > 0) {
    digitalWrite(LEFT_IN1, HIGH);
    digitalWrite(LEFT_IN2, LOW);
  } else if (leftSpeed < 0) {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, HIGH);
  } else {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, LOW);
  }

  if (rightSpeed > 0) {
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, HIGH);
  } else if (rightSpeed < 0) {
    digitalWrite(RIGHT_IN1, HIGH);
    digitalWrite(RIGHT_IN2, LOW);
  } else {
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, LOW);
  }

  analogWrite(LEFT_PWM, abs(leftSpeed));
  analogWrite(RIGHT_PWM, abs(rightSpeed));
}

long readDistance() {
  static uint8_t buffer[4];
  while (Sensor.available() >= 4) {
    if (Sensor.read() == 0xFF) {
      buffer[0] = 0xFF;
      for (int i = 1; i < 4; i++) buffer[i] = Sensor.read();
      if (((buffer[0] + buffer[1] + buffer[2]) & 0xFF) == buffer[3]) {
        return ((buffer[1] << 8) | buffer[2]) / 10;
      }
    }
  }
  return 999;
}

void handleStart() {
  roverActive = true;
  state = FORWARD;
  start_run = millis();
  Serial.println("Rover Started via Web Dashboard");
  server.send(200, "text/plain", "STARTED");
}

void handleStop() {
  roverActive = false;
  setMotors(0, 0);
  state = START;
  start_run = 0;
  Serial.println("Rover Stopped via Web Dashboard");
  server.send(200, "text/plain", "STOPPED");
}

void handleCalibration() {
  calibrateAngle();
  server.send(200, "text/plain", "STOPPED");
}

void calibrateAngle() {
  long sum = 0;
  digitalWrite(ONBOARD_LED, HIGH);
  for (int i = 0; i < 10; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gz, &gy, &gz);
    delay(2);
  }
  for (int i = 0; i < 250; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gz, &gy, &gz);
    sum += gz;
    delay(2);
  }
  gyroOffset = sum / 250.0;
  currentError = 0.0;
  headingIntegral = 0.0;
  previousError = 0.0;
  previousTime = millis();
  digitalWrite(ONBOARD_LED, LOW);
  Serial.print("Offset: ");
  Serial.println(gyroOffset);
}

void updateHeadingAndSteer(bool forward) {
  int leftSpeed;
  int rightSpeed;
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - previousTime) / 1000.0;
  if (deltaTime <= 0.02) return; // Limit loop execution rate (~50Hz max)

  // 1. Read raw motion data from MPU6050
  int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
  mpu.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);

  // 2. Convert Gyro to degrees/second (using single gyroOffset for Z-axis)
  float gx = (gx_raw - gyroOffset) / 131.1;
  float gy = (gy_raw - gyroOffset) / 131.1;
  float gz = (gz_raw - gyroOffset) / 131.1;

  // 3. Convert Accelerometer to g's (assuming ±2g range -> 16384 LSB/g)
  float ax = ax_raw / 16384.0;
  float ay = ay_raw / 16384.0;
  float az = az_raw / 16384.0;

  // 4. Update the Madgwick filter (6-DOF IMU mode)
  filter.updateIMU(gx, gy, gz, ax, ay, az);

  // 5. Extract fused yaw and calculate error using turnAngle
  float currentYaw = filter.getYaw();
  float currentError = turnAngle - currentYaw;

  // 6. PID Calculations
  headingIntegral += (currentError * deltaTime);
  headingIntegral = constrain(headingIntegral, -50.0, 50.0);

  float derivative = (currentError - previousError) / deltaTime;
  int correction = 0;
  if (abs(currentError) > 1.5) {
    correction = (k_p * currentError) + (k_i * headingIntegral) + (k_d * derivative);
  } else {
    headingIntegral = 0.0; // Reset windup when close to target
  }

  correction = constrain(correction, -80, 80);

  // 7. Apply differential motor speeds
  if (forward) {
    leftSpeed = leftBaseSpeed - correction;
    rightSpeed = rightBaseSpeed + correction;
    leftSpeed = constrain(leftSpeed, -MAX_SAFE_SPEED, MAX_SAFE_SPEED);
    rightSpeed = constrain(rightSpeed, -MAX_SAFE_SPEED, MAX_SAFE_SPEED);
    setMotors(-leftSpeed, -rightSpeed);
  } else {
    leftSpeed = leftBaseSpeed + correction;
    rightSpeed = rightBaseSpeed - correction;
    leftSpeed = constrain(leftSpeed, -MAX_SAFE_SPEED, MAX_SAFE_SPEED);
    rightSpeed = constrain(rightSpeed, -MAX_SAFE_SPEED, MAX_SAFE_SPEED);
    setMotors(leftSpeed, rightSpeed);
  }

  previousTime = currentTime;
  previousError = currentError;

  Serial.print("Left In: "); Serial.print(leftSpeed);
  Serial.print(" | Right In: "); Serial.println(rightSpeed);
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  String wifiStat = (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED";
  String currentMode = isManual ? "MANUAL" : "AUTO";
  String json = "{";
  json += "\"error\":" + String(currentError, 2) + ",";
  json += "\"distance\":" + String(sensorDistance) + ",";
  json += "\"wifiStatus\":\"" + wifiStat + "\",";
  json += "\"mode\":\"" + currentMode + "\",";  // ADDED MODE
  json += "\"speed\":" + String(leftBaseSpeed) + ",";
  json += "\"kp\":" + String(k_p, 3) + ",";
  json += "\"ki\":" + String(k_i, 3) + ",";
  json += "\"kd\":" + String(k_d, 3) + ",";
  json += "\"turnAngle\":" + String(turnAngle, 1) + ",";
  json += "\"turnSpeed\":" + String(turnSpeed) + ",";
  json += "\"turnSpeedM\":" + String(turnSpeedM) + ",";
  json += "\"minDist\":" + String(minDist);
  json += "}";
  server.send(200, "application/json", json);
}

void sendEmail() {
  String tmp = url + "link=" + urlencode(getDashBoardLink());
  sendHttpGet(tmp);
}

void sendHttpGet(String requestUrl) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, requestUrl);
  http.setTimeout(5000);
  int code = http.GET();
  if (code > 0) Serial.println(http.getString());
  http.end();
}

String urlencode(String str) {
  String encoded = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      char code1 = ((c & 0xf) > 9) ? (c & 0xf) - 10 + 'A' : (c & 0xf) + '0';
      char code0 = (((c >> 4) & 0xf) > 9) ? ((c >> 4) & 0xf) - 10 + 'A' : ((c >> 4) & 0xf) + '0';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

String getDashBoardLink() {
  return "http://" + WiFi.localIP().toString();
}

void handleParamUpdate() {
  // 1. Base Driving Speed (Forward / Backward)
  if (server.hasArg("speed")) {
    int requestedSpeed = server.arg("speed").toInt();
    requestedSpeed = constrain(requestedSpeed, 0, MAX_SAFE_SPEED);
    leftBaseSpeed = requestedSpeed;
    rightBaseSpeed = requestedSpeed;
  }

  // 2. Autonomous Turn Speed
  if (server.hasArg("turnSpeed")) {
    turnSpeed = constrain(server.arg("turnSpeed").toInt(), 0, MAX_SAFE_SPEED);
  }

  // 3. Manual Mode Turn Speed
  if (server.hasArg("turnSpeedM")) {
    turnSpeedM = constrain(server.arg("turnSpeedM").toInt(), 0, MAX_SAFE_SPEED);
  }

  // 4. PID Gain Tuning
  if (server.hasArg("kp")) k_p = server.arg("kp").toFloat();
  if (server.hasArg("ki")) k_i = server.arg("ki").toFloat();
  if (server.hasArg("kd")) k_d = server.arg("kd").toFloat();

  // 5. Ultrasonic Distance Threshold
  if (server.hasArg("minDist")) {
    minDist = server.arg("minDist").toInt();
    Serial.print("Updated Min Distance: ");
    Serial.println(minDist);
  }

  // 6. Target Turn Angle
  if (server.hasArg("turnAngle")) {
    turnAngle = server.arg("turnAngle").toFloat();
  }

  server.send(200, "text/plain", "OK");
}
void dump(bool dumping) {
  if (!dumping) {
    //targetServoPos = RETRACT;  // Retracted / Lowered
    dumper.write(RETRACT);
  } else {
    //targetServoPos = DUMP;  // Fully Dumped / Lifted
    dumper.write(DUMP);
  }
}

void updateServoSmoothly() {
  if (millis() - lastServoUpdate >= servoInterval) {
    lastServoUpdate = millis();

    if (currentServoPos < targetServoPos) {
      currentServoPos++;
      dumper.write(currentServoPos);
    } else if (currentServoPos > targetServoPos) {
      currentServoPos--;
      dumper.write(currentServoPos);
    }
  }
}

void handleManualMode() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - previousTime) / 1000.0;

  // 1. Forward / Backward (Apply PID to stay perfectly straight)
  if (currentDir == 'F' || currentDir == 'B') {
    if (deltaTime > 0.02) {
      mpu.getMotion6(&ax, &ay, &az, &gz, &gy, &gz);
      float turnRate = (gz - gyroOffset) / 131.1;

      if (abs(turnRate) > 1.0) currentError += (turnRate * deltaTime);

      headingIntegral += (currentError * deltaTime);
      headingIntegral = constrain(headingIntegral, -50.0, 50.0);

      float derivative = (currentError - previousError) / deltaTime;
      int correction = 0;
      if (abs(currentError) > 1.5) {
        correction = (k_p * currentError) + (k_i * headingIntegral) + (k_d * derivative);
      } else {
        headingIntegral = 0.0;
      }
      correction = constrain(correction, -80, 80);

      // Apply mixing based on direction
      if (currentDir == 'F') {
        updateHeadingAndSteer(true);
      } else {  // 'B' (Backward)
        updateHeadingAndSteer(false);
      }

      previousTime = currentTime;
      previousError = currentError;
    }
  }
  // 2. Turning (Raw Differential PWM, no PID fighting)
  else if (currentDir == 'L') {
    int effTurnSpeed = max(turnSpeedM, 180);
    setMotors(effTurnSpeed, -effTurnSpeed);
    previousTime = millis();
  } else if (currentDir == 'R') {
    int effTurnSpeed = max(turnSpeedM, 180);
    setMotors(-effTurnSpeed, effTurnSpeed);
    previousTime = millis();
  }
  // 3. Stopped
  else {
    setMotors(0, 0);
    previousTime = millis();
  }
}

void handleState() {
  float deltaTime;
  unsigned long currentTime;
  float turnRate;

  switch (state) {
    case START:
      setMotors(0, 0);
      break;

    case FORWARD:
      roverActive = true;
      updateHeadingAndSteer(true);
      if (sensorDistance <= minDist && sensorDistance != 999) {
        state = STOP_CAR;
        t1 = millis();
        run_dist = millis() - start_run;
      }
      break;

    case STOP_CAR:
      setMotors(0, 0);
      if (millis() - t1 >= STOP_TIME) {
        currentError = 0.0;
        headingIntegral = 0.0;
        previousError = 0.0;
        previousTime = millis();
        state = TURNING;
      }
      break;

    case TURNING:
      setMotors(turnSpeed, -turnSpeed);
      currentTime = millis();
      deltaTime = (currentTime - previousTime) / 1000.0;
      if (deltaTime > 0.02) {
        mpu.getMotion6(&ax, &ay, &az, &gz, &gy, &gz);
        turnRate = (gz - gyroOffset) / 131.1;
        if (abs(turnRate) > 0.5) currentError += (turnRate * deltaTime);
        previousTime = currentTime;
      }

      if (abs(currentError) >= turnAngle) {
        setMotors(0, 0);
        currentError = 0.0;
        headingIntegral = 0.0;
        previousError = 0.0;
        previousTime = millis();
        t2 = millis();
        if (!finalTurn) {
          state = UNLOAD;
          finalTurn = true;
        } else {
          state = FINISH;
          finalTurn = false;
        }
      }
      break;

    case UNLOAD:
      dump(true);
      if (millis() - t2 >= DUMP_TIME) {
        dump(false);

        currentError = 0.0;
        headingIntegral = 0.0;
        previousError = 0.0;
        previousTime = millis();

        state = FORWARD_RETURNING;
        t1 = millis();
      }
      break;
    case LOWER_DUMPER:
      if (millis() - t2 >= 1000) {
        currentError = 0.0;
        headingIntegral = 0.0;
        previousError = 0.0;
        previousTime = millis();

        state = FORWARD_RETURNING;
        t1 = millis();
      }
      break;
    case FORWARD_RETURNING:
      updateHeadingAndSteer(true);
      if (millis() - t1 >= run_dist) {
        state = TURNING;
      }
      break;

    case FINISH:
      setMotors(0, 0);
      roverActive = false;
      state = START;
      break;
  }
}

void loop() {
  server.handleClient();
  // updateServoSmoothly();
  long currentDist = readDistance();
  if (currentDist != 999) {
    sensorDistance = currentDist;
  }
  if (isManual) {
    handleManualMode();
  } else if (roverActive) {
    handleState();
  } else {
    setMotors(0, 0);
    state = START;
  }


}