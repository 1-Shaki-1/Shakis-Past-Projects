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

// ================= GLOBAL HANDLES & VARIABLES =================
TaskHandle_t emailTaskHandle = NULL;
volatile bool sendEmailRequest = false;

TaskHandle_t LEDTaskHandle = NULL;

String emailAddress;
String emailDate;
String emailTime;

// ================= PINS CONFIGURATION (BOTTOM ROW) =================
// GPS Pins (UART1)
const int GPS_RX = 23;  // Connect to GPS TX
const int GPS_TX = 5;   // Connect to GPS RX

// Distance Sensor Pins (UART2)
const int SONAR_RX = 18;  // Yellow Wire (Sensor TX -> ESP32 RX)
const int SONAR_TX = 19;  // White Wire  (Sensor RX -> ESP32 TX)

// MPU6050 Pins (I2C)
const int MPU_SDA = 21;
const int MPU_SCL = 22;

// RGB LED Pins (0 = OFF, 255 = ON)
const int LED_PINR = 25;
const int LED_PING = 26;
const int LED_PINB = 27;

// Board pin
const int ONBOARD_LED = 2;

// Buzzer Pin
const int BUZZER_PIN = 13;
// ===================================================================

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

const float HEIGHT = 34.0;

bool buzzerOn = false;
const int BEEP_CD = 500;

long distance = 999;    // Initial distance
double percentage = 0;  // Percent calculated

const double RED_THRESHOLD = ((HEIGHT - 5.0) / HEIGHT) * 100.0;
const double YELLOW_THRESHOLD = ((HEIGHT - 10.0) / HEIGHT) * 100.0;
const double GREEN_THRESHOLD = YELLOW_THRESHOLD - 5.0;

bool isConnected = false;

unsigned long beepTimer = 0;
unsigned long blynkTimer = 0;
unsigned long reconnectTimer = 0;

bool fullNotificationSent = false;

int badReadings = 0;

unsigned long sensorTimer = 0;

// Function declarations
String urlencode(String str);
void sendEmail(String address, String date, String time);
void emailTask(void* parameter);
double getPercentage(long distance);
long readDistance();
long getAverageDistance();
void ledsOff();
void greenFlash();
void yellowFlash();
void redFlash();
void beepFull();
void handleConnection();
void updateSensor();
void updateTrashState();
void updateBlynk();
void handleNotifications();
void updateLEDs();
void updateBuzzer();
String getDateTime();
void blueFadeTask(void* parameter);
void board_flash();
void updateGPS();
// Encodes strings into valid URL formats.
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

// Sends full alert email via HTTP endpoint.
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

// Manages background task for email requests.
void emailTask(void* parameter) {
  while (true) {
    if (sendEmailRequest) {
      sendEmailRequest = false;

      Serial.println("EMAIL TASK STARTED");

      sendEmail(getMapLink(),
                emailDate,
                emailTime);

      Serial.println("EMAIL TASK FINISHED");
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Initializes hardware components, tasks, and Blynk.
void setup() {
  Serial.begin(115200);

  // Initialize GPS on UART1
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // Initialize Distance Sensor on UART2
  SonarSerial.begin(9600, SERIAL_8N1, SONAR_RX, SONAR_TX);

  // Initialize MPU6050 on I2C
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

  // Non-blocking WiFi & Blynk setup
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();

  Serial.print(F("Calculated Red Threshold %: "));
  Serial.println(RED_THRESHOLD);
  Serial.print(F("Calculated Yellow Threshold %: "));
  Serial.println(YELLOW_THRESHOLD);

  xTaskCreatePinnedToCore(
    emailTask,
    "EmailTask",
    10000,
    NULL,
    1,
    &emailTaskHandle,
    0);

  xTaskCreatePinnedToCore(
    blueFadeTask,
    "BlueFadeTask",
    2048,
    NULL,
    2,
    &LEDTaskHandle,
    1);
}

// Runs main loop processing sensor streams.
void loop() {
  handleConnection();
  
  updateSensor();
  updateTrashState();
  updateGPS();
  if (isConnected) {
    updateBlynk();
    handleNotifications();
  }

  updateLEDs();
  updateBuzzer();
  board_flash();
  // Read GPS stream
  

   
}

// Converts distance measurement into fill percentage.
double getPercentage(long distance) {
  double percentage = ((HEIGHT - distance) / HEIGHT) * 100.0;
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  return percentage;
}

// Reads raw distance from ultrasonic sensor.
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

// Turns all RGB channels completely off.
void ledsOff() {
  analogWrite(LED_PINR, 0);
  analogWrite(LED_PING, 0);
  analogWrite(LED_PINB, 0);
}

// Illuminates the green LED channel only.
void greenFlash() {
  analogWrite(LED_PINR, 0);
  analogWrite(LED_PING, 255);
  analogWrite(LED_PINB, 0);
}

// Blends red and green channels for yellow.
void yellowFlash() {
  analogWrite(LED_PINR, 255);
  analogWrite(LED_PING, 255);
  analogWrite(LED_PINB, 0);
}

// Illuminates the red LED channel only.
void redFlash() {
  analogWrite(LED_PINR, 255);
  analogWrite(LED_PING, 0);
  analogWrite(LED_PINB, 0);
}

// Beeps buzzer when bin is full.
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

// Maintains WiFi and Blynk cloud connection without blocking loop.
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

// Updates distance and percentage readings periodically.
void updateSensor() {
  if (millis() - sensorTimer > 250) {
    sensorTimer = millis();

    distance = getAverageDistance();
    percentage = getPercentage(distance);
  }
}

// Determines bin status from calculated percentage.
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
      if (percentage >= RED_THRESHOLD) {
        colorState = RED;
      } else if (percentage >= YELLOW_THRESHOLD) {
        colorState = YELLOW;
      } else {
        colorState = GREEN;
      }
    } else {
      if (percentage < RED_THRESHOLD - 5)
        colorState = YELLOW;
    }
  }
}

// Pushes updated measurements to Blynk dashboard.
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
    Serial.print(F(" State="));
    Serial.println(colorState);
  }
}

// Triggers Blynk events and email alerts.
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

// Selects active color based on state.
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

// Drives buzzer activity during full state.
void updateBuzzer() {
  if (colorState == RED) {
    beepFull();
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }
}

// Returns formatted string containing current time.
String getDateTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "Time unavailable";
  }

  char buffer[50];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Calculates average from five distance readings.
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

// Fades external blue LED during sensor error state.
void blueFadeTask(void* parameter) {
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

// Handles pulsing the onboard LED when disconnected from WiFi.
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

// Reads GPS stream and prints location.
void updateGPS() {
  // Feed raw characters to parser.
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  // Print data when position updates.
  if (gps.location.isUpdated()) {
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

  }
}

// Returns formatted location string.
String getGPSLocation() {
  // Parse waiting serial bytes.
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  // Check for valid fix.
  if (gps.location.isValid()) {
    String lat = String(gps.location.lat(), 6);
    String lng = String(gps.location.lng(), 6);
    return lat + ", " + lng;
  } else {
    return "No GPS Lock";
  }
}

String getMapLink() {

  if (!gps.location.isValid()) {
    return "No GPS Lock";
  }

  String LATITUDE = String(gps.location.lat(),6);
  String LONGITUDE = String(gps.location.lng(),6);
  String link = "https://maps.google.com/?q="+LATITUDE+","+LONGITUDE+"";
  return link;

}