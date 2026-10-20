#include <WiFi.h>
#include <time.h>
#include <TinyGPSPlus.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <index.h>

// --- CONSTANTS & CONFIG ---

// WiFi details
const char* ssid = " wifi user";
const char* password = "pass";
// Google script URL
String url = "https://script.google.com/macros/s/ID-HERE/exec?";

// --- MOCK GPS CONFIG (Hackathon Fallback) ---
const double MOCK_LAT = 43.653226;
const double MOCK_LNG = -79.383184;

// Pin definitions
const int GPS_RX = 23;      // GPS RX pin (unused now)
const int GPS_TX = 5;       // GPS TX pin (unused now)
const int SONAR_RX = 18;    // Sonar RX pin
const int SONAR_TX = 19;    // Sonar TX pin
const int MPU_SDA = 21;     // I2C data pin
const int MPU_SCL = 22;     // I2C clock pin
const int LED_PINR = 25;    // Red LED pin
const int LED_PING = 26;    // Green LED pin
const int LED_PINB = 27;    // Blue LED pin
const int ONBOARD_LED = 2;  // Onboard LED pin
const int BUZZER_PIN = 13;  // Buzzer pin

const int BEEP_CD = 500;  // Beep cooldown

// --- OBJECTS ---

WebServer server(80);           // Web server
TinyGPSPlus gps;                // GPS instance (retained for compatibility)
HardwareSerial GPSSerial(1);    // UART1 serial
HardwareSerial SonarSerial(2);  // UART2 serial
Adafruit_MPU6050 mpu;           // Gyro instance

// --- GLOBAL VARIABLES ---

//ESP32 Task handles
TaskHandle_t emailTaskHandle = NULL;
TaskHandle_t LEDTaskHandle = NULL;

// Email data
volatile bool sendEmailRequest = false;
String emailAddress;
String emailDate;
String emailTime;
bool emailAlreadySent = false;
bool fullNotificationSent = false;

// Trash states
enum TrashState { GREEN,
                  YELLOW,
                  RED,
                  BLUE,
                  BOARD_BLUE,
                  LID_OPEN };
TrashState colorState = BOARD_BLUE;
TrashState previousColorState = BOARD_BLUE;
bool ledOn = false;

// Sensor data
float currentBinHeight = 34.0;
long distance = 999;
double percentage = 0;
int badReadings = 0;
float tiltAngle = 0.0;

// GPS data (Mocked)
bool gpsValid = true;
bool locationValid = true;
unsigned long lastGpsByteTime = 0;

// System states
bool isConnected = false;
bool buzzerOn = false;
bool lidOpen = false;
// Timers
unsigned long beepTimer = 0;
unsigned long lidTimer = 0;
unsigned long reconnectTimer = 0;
unsigned long sensorTimer = 0;

const int FLASH_DELAY = 500;


// --- SETUP & LOOP ---

void setup() {
  Serial.begin(115200);
  SonarSerial.begin(9600, SERIAL_8N1, SONAR_RX, SONAR_TX);
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(100000); // Safe 100kHz speed
  delay(200);

  // Wake up MPU6050 (Write 0 to Power Management register 0x6B)
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("Error: MPU6050 did not respond to wake command!");
  } else {
    Serial.println("MPU6050 Woken Up & Ready!");
  }

  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(LED_PINR, OUTPUT);
  pinMode(LED_PING, OUTPUT);
  pinMode(LED_PINB, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  analogWriteResolution(ONBOARD_LED, 8);
  analogWriteResolution(LED_PINR, 8);
  analogWriteResolution(LED_PING, 8);
  analogWriteResolution(LED_PINB, 8);

  setRGBColor(0, 0, 0);
  Serial.println(F("Starting"));

  WiFi.begin(ssid, password);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();

  xTaskCreatePinnedToCore(emailTask, "EmailTask", 20000, NULL, 1, &emailTaskHandle, 0);
  xTaskCreatePinnedToCore(sensorErrorFlash, "SensorError", 2048, NULL, 2, &LEDTaskHandle, 1);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set-height", HTTP_POST, handleSetHeight);
  server.begin();
}

void loop() {
  handleConnection();

  updateSensor();
  updateGyro();
  updateTrashState();
  updateGPS();

  if (isConnected) {
    handleNotifications();
    server.handleClient();
    if (!emailAlreadySent) {
      sendLinkEmail(getDashBoardLink());
      emailAlreadySent = true;
    }
  }

  updateLEDs();
  updateBuzzer();
  board_flash();
}


// --- NETWORK & WEB FUNCTIONS ---

void handleConnection() {  // connect to wifi
  isConnected = (WiFi.status() == WL_CONNECTED);
  if (!isConnected) {
    colorState = BOARD_BLUE;
    if (millis() - reconnectTimer > 5000) {
      reconnectTimer = millis();
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }
}

void handleRoot() {  // Initialise website
  server.send(200, "text/html", index_html);
}

void handleData() {  //  JSON file to website for updating data
  String wifiStat = (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED";
  String sonarStat = (distance > 0 && distance < 400) ? "ONLINE" : "OFFLINE";
  String gpsStat = "ONLINE";

  String json = "{";
  json += "\"percentage\":\"" + String(percentage, 0) + "%\",";
  json += "\"hasGPS\":true,";
  json += "\"gpsLocation\":\"" + getGPSLocation() + "\",";
  json += "\"ledColor\":" + String(colorState) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"tiltAngle\":" + String(tiltAngle, 1) + ",";
  json += "\"height\":" + String(currentBinHeight, 1) + ",";
  json += "\"esp32Status\":\"ONLINE\",";
  json += "\"ultrasonicStatus\":\"" + sonarStat + "\",";
  json += "\"gpsStatus\":\"" + gpsStat + "\",";
  json += "\"wifiStatus\":\"" + wifiStat + "\",";
  json += "\"gyroStatus\":true,";
  json += "\"mapUrl\":\"" + getMapLink() + "&output=embed\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleSetHeight() {  // Recieves input from Website for new height
  if (server.hasArg("plain")) {
    StaticJsonDocument<200> doc;
    if (!deserializeJson(doc, server.arg("plain")) && doc.containsKey("height")) {
      float newHeight = doc["height"].as<float>();
      if (newHeight > 0) {
        currentBinHeight = newHeight;
        Serial.print("Height updated: ");
        Serial.println(currentBinHeight);
        server.send(200, "application/json", "{\"status\":\"success\"}");
        return;
      }
    }
  }
  server.send(400, "application/json", "{\"status\":\"error\"}");
}

// --- SENSOR LOGIC FUNCTIONS ---

void updateSensor() {  // updates sensor readings
  if (millis() - sensorTimer > 250) {
    sensorTimer = millis();
    distance = getAverageDistance();
    percentage = getPercentage(distance);
  }
}

long getAverageDistance() {  // Filters out bad readings
  long total = 0;
  int count = 0;
  for (int i = 0; i < 5; i++) {
    long reading = readDistance();
    if (reading != 999) {
      total += reading;
      count++;
    }
  }
  return count == 0 ? 999 : total / count;
}

long readDistance() {  // Gets sensor data
  if(lidOpen) {
    return 999;
  } else {
    static uint8_t buffer[4];
    while (SonarSerial.available() >= 4) {
      if (SonarSerial.read() == 0xFF) {
        buffer[0] = 0xFF;
        for (int i = 1; i < 4; i++) buffer[i] = SonarSerial.read();
        if (((buffer[0] + buffer[1] + buffer[2]) & 0xFF) == buffer[3]) {
          return ((buffer[1] << 8) | buffer[2]) / 10;
        }
      }
    }
  }
  return 999;
} 

double getPercentage(long dist) {  // Convert Sensor Distance to Trash Can Percentage
  if (currentBinHeight <= 0) return 0;
  double pct = ((currentBinHeight - dist) / currentBinHeight) * 100.0;
  return constrain(pct, 0, 100);
}

double getRedThreshold() {
  return ((currentBinHeight - 5.0) / currentBinHeight) * 100.0;
}

double getYellowThreshold() {
  return ((currentBinHeight - 10.0) / currentBinHeight) * 100.0;
}

void updateTrashState() {
  if (!isConnected) {
    colorState = BOARD_BLUE;
    return;
  }
  if (lidOpen) {
    colorState = LID_OPEN;
  } else {
    if (distance == 999) {
      if (++badReadings >= 5) colorState = BLUE;
    } else {
      badReadings = 0;
      if (colorState != RED) {
        if (percentage >= getRedThreshold()) colorState = RED;
        else if (percentage >= getYellowThreshold()) colorState = YELLOW;
        else colorState = GREEN;
      } else if (percentage < getRedThreshold() - 5) {
        colorState = YELLOW;
      }
    }
  }
}

void updateGPS() {  // Mocked for hackathon stability
  gpsValid = true;
  locationValid = true;
}

String getGPSLocation() {
  return String(MOCK_LAT, 6) + ", " + String(MOCK_LNG, 6);
}

// --- ALERT & NOTIFICATION FUNCTIONS ---

void handleNotifications() {
  if (isConnected && colorState == RED) {
    if (!fullNotificationSent) {
      fullNotificationSent = true;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char dateBuf[20], timeBuf[20];
        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &timeinfo);
        strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
        if (!sendEmailRequest) {
          emailAddress = "123MainStreet";
          emailDate = String(dateBuf);
          emailTime = String(timeBuf);
          sendEmailRequest = true;
        }
      }
    }
  } else if (colorState != RED) {
    fullNotificationSent = false;
  }
  previousColorState = colorState;
}

void emailTask(void* parameter) {
  while (true) {
    if (sendEmailRequest) {
      sendEmailRequest = false;
      sendEmail(getMapLink(), emailDate, emailTime);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void sendEmail(String address, String date, String time) {  // Main function for sending email
  String tmp = url + "address=" + urlencode(address);
  tmp += "&status=" + urlencode("FULL");
  tmp += "&date=" + urlencode(date);
  tmp += "&time=" + urlencode(time);
  sendHttpGet(tmp);
}

void sendLinkEmail(String link) {  // Helper function for dashboard link
  sendHttpGet(url + "link=" + urlencode(link));
}

String getMapLink() {  // Gets google maps link
  return "https://maps.google.com/?q=" + String(MOCK_LAT, 6) + "," + String(MOCK_LNG, 6);
}

String getDashBoardLink() {  // Dashboard Link from ip address
  return "http://" + WiFi.localIP().toString();
}

void sendHttpGet(String requestUrl) {  // Sends link to google, perform action
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, requestUrl);
  http.setTimeout(5000);
  int code = http.GET();
  Serial.print(F("HTTP Code: "));
  Serial.println(code);
  if (code > 0) Serial.println(http.getString());
  http.end();
}

// --- HARDWARE UI (LEDS & BUZZER & GYRO) ---

void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {  // Assigns colour to led
  analogWrite(LED_PINR, r);
  analogWrite(LED_PING, g);
  analogWrite(LED_PINB, b);
}

void updateLEDs() {  // Give led colour based on state
  if (colorState == GREEN) setRGBColor(0, 255, 0);
  else if (colorState == YELLOW) setRGBColor(255, 255, 0);
  else if (colorState == RED) setRGBColor(255, 0, 0);
  else if (colorState == LID_OPEN) lidOpenFlash();
  else setRGBColor(0, 0, 0);
}

void board_flash() {  // Offline board flash
  static unsigned long currentMillis = 0;
  static int boardFade = 0;
  static int boardDirection = 5;

  if (colorState == BOARD_BLUE) {
    setRGBColor(0, 0, 0);
    if (millis() - currentMillis > 10) {
      currentMillis = millis();
      boardFade += boardDirection;
      if (boardFade >= 255 || boardFade <= 0) boardDirection = -boardDirection;
      analogWrite(ONBOARD_LED, boardFade);
    }
  } else {
    analogWrite(ONBOARD_LED, 0);
  }
}

void lidOpenFlash() {
  if (millis() - lidTimer > FLASH_DELAY) {
    lidTimer = millis();
    ledOn = !ledOn;

    if (ledOn) {
      setRGBColor(0, 0, 255);  // Blue ON
    } else {
      setRGBColor(0, 0, 0);  // Blue OFF
    }
  }
}

void sensorErrorFlash(void* parameter) {  // Flashes blue for sensor error
  int localFade = 0;
  int localDirection = 5;
  while (true) {
    if (colorState == BLUE) {
      localFade += localDirection;
      if (localFade >= 255 || localFade <= 0) localDirection = -localDirection;
      setRGBColor(0, 0, localFade);
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void updateBuzzer() {
  if (colorState == RED) {
    if (millis() - beepTimer > BEEP_CD) {
      beepTimer = millis();
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }
}

void updateGyro() {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return;
  
  Wire.requestFrom(0x68, 6, true);
  if (Wire.available() < 6) return;

  int16_t accelX = Wire.read() << 8 | Wire.read();
  int16_t accelY = Wire.read() << 8 | Wire.read();
  int16_t accelZ = Wire.read() << 8 | Wire.read();

  tiltAngle = abs(atan2((float)accelY, (float)accelZ) * 57.2958);

  if (tiltAngle > 45.0) {
    lidOpen = true;
  } else {
    lidOpen = false;
  }
}

// --- UTILITY FUNCTIONS ---

String urlencode(String str) {  // Encoding strings for Google URL
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