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
#include <ArduinoJson.h>
#include <index.h>

// --- CONSTANTS & CONFIG ---

// WiFi details
const char* ssid = "BELL769";
const char* password = "C5C2F99DF6A3";

// Google script URL
String url = "https://script.google.com/macros/s/AKfycbzauEF7aP8uP3ZqOLGqv1ytd_-U0fHIB0qkS7nX2VDVlA2euF6D32Cr5Shb_IYMfZU/exec?";

// Pin definitions
const int GPS_RX = 23;      // GPS RX pin
const int GPS_TX = 5;       // GPS TX pin
const int SONAR_RX = 18;    // Sonar RX pin
const int SONAR_TX = 19;    // Sonar TX pin
const int MPU_SDA = 21;     // I2C data pin
const int MPU_SCL = 22;     // I2C clock pin
const int LED_PINR = 25;    // Red LED pin
const int LED_PING = 26;    // Green LED pin
const int LED_PINB = 27;    // Blue LED pin
const int ONBOARD_LED = 2;  // Onboard LED pin
const int BUZZER_PIN = 13;  // Buzzer pin

const int BEEP_CD = 500;    // Beep cooldown

// --- OBJECTS ---

WebServer server(80);           // Web server
TinyGPSPlus gps;                // GPS instance
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
enum TrashState { GREEN, YELLOW, RED, BLUE, BOARD_BLUE };
TrashState colorState = BOARD_BLUE;
TrashState previousColorState = BOARD_BLUE;

// Sensor data
float currentBinHeight = 34.0;
long distance = 999;
double percentage = 0;
int badReadings = 0;

// GPS data
bool gpsValid = false;
bool locationValid = false;
unsigned long lastGpsByteTime = 0;

// System states
bool isConnected = false;
bool buzzerOn = false;

// Timers
unsigned long beepTimer = 0;
unsigned long blynkTimer = 0;
unsigned long reconnectTimer = 0;
unsigned long sensorTimer = 0;


// --- SETUP & LOOP ---

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  SonarSerial.begin(9600, SERIAL_8N1, SONAR_RX, SONAR_TX);
  Wire.begin(MPU_SDA, MPU_SCL);
  
  if (!mpu.begin()) {
    Serial.println(F("MPU missing"));
  } else {
    Serial.println(F("MPU ready"));
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
  Blynk.config(BLYNK_AUTH_TOKEN);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();

  xTaskCreatePinnedToCore(emailTask, "EmailTask", 10000, NULL, 1, &emailTaskHandle, 0);
  xTaskCreatePinnedToCore(sensorErrorFlash, "SensorError", 2048, NULL, 2, &LEDTaskHandle, 1);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set-height", HTTP_POST, handleSetHeight);
  server.begin();
}

void loop() {
  handleConnection();
  
  updateSensor();

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

void handleConnection() { // connect to wifi
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

void handleRoot() { // Initialise website
  server.send(200, "text/html", index_html);
}

void handleData() { //  JSON file to website for updating data
  String wifiStat = (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED";
  String sonarStat = (distance > 0 && distance < 400) ? "ONLINE" : "OFFLINE";
  String gpsStat = !gpsValid ? "OFFLINE" : (!locationValid ? "checking for satellites" : "ONLINE");

  String json = "{";
  json += "\"percentage\":\"" + String(percentage, 0) + "%\",";
  json += "\"hasGPS\":" + String(locationValid ? "true" : "false") + ",";
  json += "\"gpsLocation\":\"" + getGPSLocation() + "\",";
  json += "\"ledColor\":" + String(colorState) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"height\":" + String(currentBinHeight, 1) + ",";
  json += "\"esp32Status\":\"ONLINE\",";
  json += "\"ultrasonicStatus\":\"" + sonarStat + "\",";
  json += "\"gpsStatus\":\"" + gpsStat + "\",";
  json += "\"wifiStatus\":\"" + wifiStat + "\",";
  json += "\"mapUrl\":\"" + (locationValid ? getMapLink() : "") + "&output=embed\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleSetHeight() { // Recieves input from Website for new height
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

void updateSensor() { // updates sensor readings
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

long readDistance() { // Gets sensor data
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
  return 999;
}

double getPercentage(long dist) { // Convert Sensor Distance to Trash Can Percentage
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



void updateGPS() { // grabbing gps coordinates
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
    lastGpsByteTime = millis();
  }
  gpsValid = (millis() - lastGpsByteTime < 3000) && (lastGpsByteTime > 0);
  locationValid = gpsValid && gps.location.isValid() && (gps.location.age() < 2000);
}

String getGPSLocation() { 
  if (!gpsValid) return "OFFLINE";
  if (locationValid) {
    return String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6);
  }
  return "checking for satellites";
}
// --- ALERT & NOTIFICATION FUNCTIONS ---

void handleNotifications() {
  if (isConnected && colorState == RED) {
    if (!fullNotificationSent) {
      Blynk.logEvent("trash_full", "Trash can is full!");
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

void sendEmail(String address, String date, String time) { // Main function for sending email
  String tmp = url + "address=" + urlencode(address);
  tmp += "&status=" + urlencode("FULL");
  tmp += "&date=" + urlencode(date);
  tmp += "&time=" + urlencode(time);
  sendHttpGet(tmp);
}

void sendLinkEmail(String link) { // Helper function for dashboard link
  sendHttpGet(url + "link=" + urlencode(link));
}

String getMapLink() { // Gets google maps link 
  if (!locationValid) return "checking for satellites";
  return "https://maps.google.com/?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
}

String getDashBoardLink() { // DAshboard Link from ip address
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
// --- HARDWARE UI (LEDS & BUZZER) ---

void setRGBColor(uint8_t r, uint8_t g, uint8_t b) { // Assigns colour to led
  analogWrite(LED_PINR, r);
  analogWrite(LED_PING, g);
  analogWrite(LED_PINB, b);
}

void updateLEDs() { // Give led colour based on state
  if (colorState == GREEN) setRGBColor(0, 255, 0);
  else if (colorState == YELLOW) setRGBColor(255, 255, 0);
  else if (colorState == RED) setRGBColor(255, 0, 0);
  else setRGBColor(0, 0, 0);
}

void board_flash() { // Offline board flash
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

void sensorErrorFlash(void* parameter) { // Flashes blue for sensor error
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
// --- UTILITY FUNCTIONS ---

String urlencode(String str) { // Encoding strings for Google URL
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