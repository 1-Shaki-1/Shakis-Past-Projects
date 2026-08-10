#define BLYNK_TEMPLATE_ID "TMPL2ksohoznS"
#define BLYNK_TEMPLATE_NAME "Smart Trash Can"
#define BLYNK_AUTH_TOKEN "TWF4N7EYEEna6tP-c_TtcV8ztCiXvmaF"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>
#include <TinyGPSPlus.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WebServer.h>
#include <ArduinoJson.h> // JSON library
#include <index.h>

WebServer server(80);

// GLOBAL HANDLES & VARIABLES
TaskHandle_t emailTaskHandle = NULL;
volatile bool sendEmailRequest = false;

TaskHandle_t LEDTaskHandle = NULL;

String emailAddress;
String emailDate;
String emailTime;

// PINS CONFIGURATION
// GPS Pins (UART1)
const int GPS_RX = 23;  // GPS TX
const int GPS_TX = 5;   // GPS RX

// Sensor Pins (UART2)
const int SONAR_RX = 18;  // Sensor TX -> ESP32 RX
const int SONAR_TX = 19;  // Sensor RX -> ESP32 TX

// MPU6050 Pins (I2C)
const int MPU_SDA = 21;
const int MPU_SCL = 22;

// RGB LED Pins
const int LED_PINR = 25;
const int LED_PING = 26;
const int LED_PINB = 27;

// Board pin
const int ONBOARD_LED = 2;

// Buzzer Pin
const int BUZZER_PIN = 13;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
HardwareSerial SonarSerial(2);

Adafruit_MPU6050 mpu;

const char* ssid = "BELL769";
const char* password = "C5C2F99DF6A3";

// Trash states
enum TrashState {
  GREEN,
  YELLOW,
  RED,
  BLUE,
  BOARD_BLUE
};

TrashState colorState = BOARD_BLUE;
TrashState previousColorState = BOARD_BLUE;

// Height in cm
float currentBinHeight = 34.0;

bool gpsValid = false;

bool buzzerOn = false;
const int BEEP_CD = 500;

long distance = 999;    // Initial distance
double percentage = 0;  // Calculated percentage

// Dynamic threshold helpers
double getRedThreshold() {
  return ((currentBinHeight - 5.0) / currentBinHeight) * 100.0;
}

double getYellowThreshold() {
  return ((currentBinHeight - 10.0) / currentBinHeight) * 100.0;
}

bool isConnected = false;

unsigned long beepTimer = 0;
unsigned long blynkTimer = 0;
unsigned long reconnectTimer = 0;

bool fullNotificationSent = false;

int badReadings = 0;

unsigned long sensorTimer = 0;

// Send HTML page
void handleRoot() {
  server.send(200, "text/html", index_html);
}

// Send telemetry JSON
void handleData() {
  String json = "{";
  json += String("\"percentage\":") + " \"" + String(percentage, 0) + "%\"" + ",";
  json += "\"hasGPS\":" + String(gpsValid) + ",";
  json += "\"ledColor\":" + String(colorState) + ",";
  json += "\"height\":" + String(currentBinHeight, 1) + ","; // Return current height
  json += String("\"mapUrl\":\"https://maps.google.com/maps?q=") + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6) + "&output=embed\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Handle POST height request
void handleSetHeight() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (!error && doc.containsKey("height")) {
      float newHeight = doc["height"].as<float>();
      if (newHeight > 0) {
        currentBinHeight = newHeight;
        Serial.print("Updated trashcan height to: ");
        Serial.print(currentBinHeight);
        Serial.println(" cm");

        server.send(200, "application/json", "{\"status\":\"success\"}");
        return;
      }
    }
  }
  server.send(400, "application/json", "{\"status\":\"error\"}");
}

// Initialize hardware, tasks, Blynk
void setup() {
  Serial.begin(115200);

  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  SonarSerial.begin(9600, SERIAL_8N1, SONAR_RX, SONAR_TX);

  Wire.begin(MPU_SDA, MPU_SCL);
  if (!mpu.begin()) {
    Serial.println(F("Warning: MPU6050 not detected!"));
  } else {
    Serial.println(F("MPU6050 Initialized successfully!"));
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

  ledsOff();

  Serial.println(F("Starting System..."));

  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();

  Serial.print(F("Initial Red Threshold %: "));
  Serial.println(getRedThreshold());
  Serial.print(F("Initial Yellow Threshold %: "));
  Serial.println(getYellowThreshold());

  xTaskCreatePinnedToCore(
    emailTask,
    "EmailTask",
    10000,
    NULL,
    1,
    &emailTaskHandle,
    0);

  xTaskCreatePinnedToCore(
    sensorErrorFlash,
    "SensorError",
    2048,
    NULL,
    2,
    &LEDTaskHandle,
    1);

  // Register HTTP routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set-height", HTTP_POST, handleSetHeight);

  server.begin();
}

// Main loop
void loop() {
  handleConnection();

  updateSensor();
  updateTrashState();
  updateGPS();
  if (isConnected) {
    updateBlynk();
    handleNotifications();
    server.handleClient();
  }

  updateLEDs();
  updateBuzzer();
  board_flash();
}

// Maintain WiFi and Blynk
void handleConnection() {
  isConnected = (WiFi.status() == WL_CONNECTED);

  if (!isConnected) {
    colorState = BOARD_BLUE;

    if (millis() - reconnectTimer > 5000) {
      reconnectTimer = millis();
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  } else {
    if (Blynk.connected()) {
      Blynk.run();
    } else {
      static unsigned long lastBlynkConnect = 0;

      if (millis() - lastBlynkConnect > 10000) {
        lastBlynkConnect = millis();
        Blynk.connect();
      }
    }
  }
}

// Update sensor readings
void updateSensor() {
  if (millis() - sensorTimer > 250) {
    sensorTimer = millis();

    distance = getAverageDistance();
    percentage = getPercentage(distance);
  }
}

// Convert distance to percentage
double getPercentage(long distance) {
  if (currentBinHeight <= 0) return 0;
  double pct = ((currentBinHeight - distance) / currentBinHeight) * 100.0;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// Read distance from sensor
long readDistance() {
  static uint8_t buffer[4];

  while (SonarSerial.available() >= 4) {
    if (SonarSerial.read() == 0xFF) {
      buffer[0] = 0xFF;
      buffer[1] = SonarSerial.read();
      buffer[2] = SonarSerial.read();
      buffer[3] = SonarSerial.read();

      uint8_t checksum = (buffer[0] + buffer[1] + buffer[2]) & 0xFF;

      if (checksum == buffer[3]) {
        long distance = ((buffer[1] << 8) | buffer[2]) / 10;
        return distance;
      }
    }
  }

  return 999;
}

// Average five readings
long getAverageDistance() {
  long total = 0;
  int count = 0;
  for (int i = 0; i < 5; i++) {
    long reading = readDistance();
    if (reading != 999) {
      total += reading;
      count++;
    }
  }
  if (count == 0) {
    return 999;
  }
  return total / count;
}

// Update state from percentage
void updateTrashState() {
  if (!isConnected) {
    colorState = BOARD_BLUE;
    return;
  }

  if (distance == 999) {
    badReadings++;
    if (badReadings >= 5) colorState = BLUE;
  } else {
    badReadings = 0;
    if (colorState != RED) {
      if (percentage >= getRedThreshold()) {
        colorState = RED;
      } else if (percentage >= getYellowThreshold()) {
        colorState = YELLOW;
      } else {
        colorState = GREEN;
      }
    } else {
      if (percentage < getRedThreshold() - 5)
        colorState = YELLOW;
    }
  }
}

// Send data to Blynk
void updateBlynk() {
  if (millis() - blynkTimer > 250) {
    blynkTimer = millis();
    Blynk.virtualWrite(V0, distance);
    Blynk.virtualWrite(V3, percentage);

    const char* statusText = (colorState == RED) ? "FULL" : (colorState == YELLOW) ? "HALF FULL"
                                                                                   : "EMPTY";
    Blynk.virtualWrite(V1, statusText);

    Serial.print(F("Dist="));
    Serial.print(distance);
    Serial.print(F(" Pct="));
    Serial.print(percentage);
    Serial.print(F(" Height="));
    Serial.print(currentBinHeight);
    Serial.print(F(" State="));
    Serial.print(colorState);
    Serial.print(" IP ADDRESS ");
    Serial.print(WiFi.localIP());
    Serial.print(" GPS: ");
    Serial.print(getGPSLocation());
    Serial.println(" ");
  }
}

// Handle full notifications
void handleNotifications() {
  if (isConnected && colorState == RED) {
    if (!fullNotificationSent) {
      Serial.println(F("SENDING NOTIFICATION -> TRASH FULL!"));
      Blynk.logEvent("trash_full", "Trash can is full!");
      fullNotificationSent = true;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char dateBuffer[20], timeBuffer[20];
        strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);
        if (!sendEmailRequest) {
          emailAddress = "123MainStreet";
          emailDate = String(dateBuffer);
          emailTime = String(timeBuffer);
          sendEmailRequest = true;
        }
      }
    }
  } else if (colorState != RED) {
    fullNotificationSent = false;
  }

  if (colorState != previousColorState) {
    previousColorState = colorState;
  }
}

// Set RGB LED colors
void updateLEDs() {
  switch (colorState) {
    case GREEN:
      greenFlash();
      break;
    case YELLOW:
      yellowFlash();
      break;
    case RED:
      redFlash();
      break;
    case BLUE:
      break;
    case BOARD_BLUE:
      break;
  }
}

// Turn LEDs off
void ledsOff() {
  analogWrite(LED_PINR, 0);
  analogWrite(LED_PING, 0);
  analogWrite(LED_PINB, 0);
}

// Set LED to green
void greenFlash() {
  analogWrite(LED_PINR, 0);
  analogWrite(LED_PING, 255);
  analogWrite(LED_PINB, 0);
}

// Set LED to yellow
void yellowFlash() {
  analogWrite(LED_PINR, 255);
  analogWrite(LED_PING, 255);
  analogWrite(LED_PINB, 0);
}

// Set LED to red
void redFlash() {
  analogWrite(LED_PINR, 255);
  analogWrite(LED_PING, 0);
  analogWrite(LED_PINB, 0);
}

// Pulse onboard LED offline
void board_flash() {
  const long cd = 10;
  static unsigned long currentMillis = 0;
  static int boardFade = 0;
  static int boardDirection = 5;

  if (colorState == BOARD_BLUE) {
    ledsOff();

    if (millis() - currentMillis > cd) {
      currentMillis = millis();
      boardFade += boardDirection;

      if (boardFade >= 255) {
        boardFade = 255;
        boardDirection = -5;
      }
      if (boardFade <= 0) {
        boardFade = 0;
        boardDirection = 5;
      }

      analogWrite(ONBOARD_LED, boardFade);
    }
  } else {
    analogWrite(ONBOARD_LED, 0);
  }
}

// Flash blue on error
void sensorErrorFlash(void* parameter) {
  int localFade = 0;
  int localDirection = 5;

  while (true) {
    if (colorState == BLUE) {
      localFade += localDirection;

      if (localFade >= 255) {
        localFade = 255;
        localDirection = -5;
      }
      if (localFade <= 0) {
        localFade = 0;
        localDirection = 5;
      }

      analogWrite(LED_PINR, 0);
      analogWrite(LED_PING, 0);
      analogWrite(LED_PINB, localFade);
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Update buzzer state
void updateBuzzer() {
  if (colorState == RED) {
    beepFull();
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }
}

// Beep when full
void beepFull() {
  if (!buzzerOn) {
    if (millis() - beepTimer > BEEP_CD) {
      digitalWrite(BUZZER_PIN, HIGH);
      beepTimer = millis();
      buzzerOn = true;
    }
  } else {
    if (millis() - beepTimer > BEEP_CD) {
      digitalWrite(BUZZER_PIN, LOW);
      beepTimer = millis();
      buzzerOn = false;
    }
  }
}

// Encode URL string
String urlencode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;

  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);

    if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
        code1 = (c & 0xf) - 10 + 'A';

      c = (c >> 4) & 0xf;
      code0 = c + '0';

      if (c > 9)
        code0 = c - 10 + 'A';

      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }

  return encoded;
}

// Send alert email
void sendEmail(String address, String date, String time) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://script.google.com/macros/s/AKfycbzauEF7aP8uP3ZqOLGqv1ytd_-U0fHIB0qkS7nX2VDVlA2euF6D32Cr5Shb_IYMfZU/exec?";

  url += "address=" + urlencode(address);
  url += "&status=" + urlencode("FULL");
  url += "&date=" + urlencode(date);
  url += "&time=" + urlencode(time);

  Serial.println(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, url);
  http.setTimeout(5000);
  int code = http.GET();

  Serial.print(F("HTTP Code: "));
  Serial.println(code);

  if (code > 0)
    Serial.println(http.getString());

  http.end();
}

// Background email task
void emailTask(void* parameter) {
  while (true) {
    if (sendEmailRequest) {
      sendEmailRequest = false;

      Serial.println("EMAIL TASK STARTED");

      sendEmail(getMapLink(), emailDate, emailTime);

      Serial.println("EMAIL TASK FINISHED");
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Format date and time
String getDateTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "Time unavailable";
  }

  char buffer[50];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Update GPS data
void updateGPS() {
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  if (gps.location.isUpdated()) {
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
  }
}

// Get GPS location string
String getGPSLocation() {
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  if (gps.location.isValid()) {
    String lat = String(gps.location.lat(), 6);
    String lng = String(gps.location.lng(), 6);
    gpsValid = true;
    return lat + ", " + lng;
  } else {
    gpsValid = false;
    return "No GPS Lock";
  }
}

// Build Google Maps URL
String getMapLink() {
  if (!gps.location.isValid()) {
    return "No GPS Lock";
  }

  String LATITUDE = String(gps.location.lat(), 6);
  String LONGITUDE = String(gps.location.lng(), 6);
  return "https://maps.google.com/?q=" + LATITUDE + "," + LONGITUDE;
}