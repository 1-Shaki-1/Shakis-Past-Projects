#define BLYNK_TEMPLATE_ID "TMPL2ksohoznS"
#define BLYNK_TEMPLATE_NAME "Smart Trash Can"
#define BLYNK_AUTH_TOKEN "TWF4N7EYEEna6tP-c_TtcV8ztCiXvmaF"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>


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

  int code = http.GET();

  Serial.print(F("HTTP Code: "));
  Serial.println(code);

  if (code > 0)
    Serial.println(http.getString());

  http.end();
}

const char* ssid = "BELL769";
const char* password = "C5C2F99DF6A3";

// Pins
const int LED_PINB = 13;
const int LED_PING = 12;
const int LED_PINR = 27;

const int TRIG_PIN = 18;
const int ECHO_PIN = 19;

const int BUZZER_PIN = 22;

// Trash states
enum TrashState {
  GREEN,
  YELLOW,
  RED,
  BLUE
};

bool fullNotificationSent = false;

int previousColorState = TrashState::BLUE;

// Fade states
const int BRIGHT = 0;
const int DIM = 1;

int colorState = TrashState::GREEN;

int fadeBrightness = 0;
int fadeDirection = 5;


const float HEIGHT = 34.0;

bool buzzerOn = false;
bool isConnected;

const int BEEP_CD = 500;

long distance = 999;
double percentage = 0;

//Converts physical centimeters of fill space into dynamic percentages based on total height
// Red triggers when there is less than 5cm of physical space remaining at the top
const double RED_THRESHOLD = ((HEIGHT - 5.0) / HEIGHT) * 100.0;
// Yellow triggers when there is less than 10cm of physical space remaining at the top
const double YELLOW_THRESHOLD = ((HEIGHT - 10.0) / HEIGHT) * 100.0;
// Green reset clearing buffer zone
const double GREEN_THRESHOLD = YELLOW_THRESHOLD - 5.0;

unsigned long beepTimer = 0;
unsigned long blynkTimer = 0;
unsigned long reconnectTimer = 0;

int badReadings = 0;

unsigned long sensorTimer = 0;

// Function declarations so compiler knows where to find them
double getPercentage(long distance);
long readDistance();
long getAverageDistance();
void greenFlash();
void yellowFlash();
void redFlash();
void blueFlash();
void beepFull();

void setup() {
  Serial.begin(115200);

  pinMode(LED_PINR, OUTPUT);
  pinMode(LED_PING, OUTPUT);
  pinMode(LED_PINB, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  analogWriteResolution(LED_PINR, 8);
  analogWriteResolution(LED_PING, 8);
  analogWriteResolution(LED_PINB, 8);

  ledsOff();

  Serial.println(F("Starting System..."));

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();

  Serial.println(F("Connected to Blynk"));
  Serial.println(F("SENDING STARTUP TEST EVENT"));

  // Output calculated threshold values to Serial Monitor for system verification
  Serial.print(F("Calculated Red Threshold %: "));
  Serial.println(RED_THRESHOLD);
  Serial.print(F("Calculated Yellow Threshold %: "));
  Serial.println(YELLOW_THRESHOLD);

  Blynk.logEvent("trash_online", "Trash can system initialized!");
}

void loop() {

  handleConnection();
  if (isConnected) {
    updateSensor();
    updateTrashState();
    updateLEDs();
    updateBuzzer();
    
  }

 
 

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print("  Percentage: ");
  Serial.print(percentage);
  Serial.print("  State: ");
  Serial.println(colorState);
  handleNotifications();
}


double getPercentage(long distance) {
  double percentage = ((HEIGHT - distance) / HEIGHT) * 100.0;
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  return percentage;
}

// Gets distance from sensor
long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  if (distance > 0 && distance <= 2) {
    return 1;
  }
  if (distance == 0) {
    return 999;
  }
  return distance;
}

void ledsOff() {
  analogWrite(LED_PINR, 0);
  analogWrite(LED_PING, 0);
  analogWrite(LED_PINB, 0);
}

void greenFlash() {
  analogWrite(LED_PINR, 0);
  analogWrite(LED_PINB, 0);
  analogWrite(LED_PING, 255);
}

void yellowFlash() {
  analogWrite(LED_PINR, 255);
  analogWrite(LED_PING, 255);
  analogWrite(LED_PINB, 0);
}

void redFlash() {
  analogWrite(LED_PINR, 255);
  analogWrite(LED_PING, 0);
  analogWrite(LED_PINB, 0);
}
void blueFlash() {
  static unsigned long lastFade = 0;
  // (Static brightness variables removed from here so it uses the globals)

  if (millis() - lastFade > 5) {
    lastFade = millis();

    fadeBrightness += fadeDirection;

    if (fadeBrightness >= 255 || fadeBrightness <= 0) {
      fadeDirection = -fadeDirection;
    }

    analogWrite(LED_PINB, fadeBrightness);
  }

  analogWrite(LED_PINR, 0);
  analogWrite(LED_PING, 0);
}


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

void handleConnection() {
  isConnected = (WiFi.status() == WL_CONNECTED);

  if (!isConnected) {
    colorState = TrashState::BLUE;
    digitalWrite(BUZZER_PIN, LOW);

    if (millis() - reconnectTimer > 5000) {
      reconnectTimer = millis();
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

void updateSensor() {
  if (millis() - sensorTimer > 30) {
    sensorTimer = millis();

    distance = getAverageDistance();
    percentage = getPercentage(distance);
  }
}

void updateTrashState() {
  if (distance == 999) {
    badReadings++;
    if (badReadings >= 5) colorState = TrashState::BLUE;
  } else {
    badReadings = 0;
    if (colorState != TrashState::RED) {
      if (percentage >= RED_THRESHOLD) {
        colorState = TrashState::RED;
      } else if (percentage >= YELLOW_THRESHOLD) {
        colorState = TrashState::YELLOW;
      } else {
        colorState = TrashState::GREEN;
      }
    } else {
      if (percentage < RED_THRESHOLD - 5)
        colorState = YELLOW;
    }
  }
}



void updateBlynk() {
  // 3. --- DATA STREAM UPDATES (Only if online) ---
  if (millis() - blynkTimer > 250) {
    blynkTimer = millis();
    Blynk.virtualWrite(V0, distance);
    Blynk.virtualWrite(V3, percentage);

    const char* statusText = (colorState == TrashState::RED) ? "FULL" : (colorState == TrashState::YELLOW) ? "HALF FULL"
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

void handleNotifications() {
  // 4. --- NOTIFICATION LOGIC (Only if online & freshly full) ---
  if (isConnected && colorState == TrashState::RED) {
    if (!fullNotificationSent) {
      Serial.println(F("SENDING NOTIFICATION -> TRASH FULL!"));
      Blynk.logEvent("trash_full", "Trash can is full!");
      fullNotificationSent = true;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char dateBuffer[20], timeBuffer[20];
        strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);
        sendEmail("123MainStreet", String(dateBuffer), String(timeBuffer));
      }
    }
  } else if (colorState != TrashState::RED) {
    fullNotificationSent = false;
  }

  if (colorState != previousColorState) {
    fadeBrightness = 0;
    fadeDirection = 5;  // Reset the fade direction back to brightening up
    analogWrite(LED_PINB, 0);

    previousColorState = colorState;
  }
}

void updateLEDs() {
  // 6. --- PHYSICAL HARDWARE OUTPUTS (Always runs) ---
  switch (colorState) {
    case TrashState::GREEN:
      greenFlash();

      break;
    case TrashState::YELLOW:
      yellowFlash();

      break;
    case TrashState::RED:
      redFlash();
      break;
    case TrashState::BLUE:
      blueFlash();

      break;
  }
}


void updateBuzzer() {
  if (colorState == TrashState::RED) {
    beepFull();
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }
}
String getDateTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "Time unavailable";
  }

  char buffer[50];

  strftime(buffer,
           sizeof(buffer),
           "%Y-%m-%d %H:%M:%S",
           &timeinfo);

  return String(buffer);
}
// Gets 3 distance readings and create average distance
long getAverageDistance() {
  long total = 0;
  int count = 0;
  for (int i = 0; i < 10; i++) {
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
