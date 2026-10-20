# FillAlert — Smart Waste Management System

### 

A low-power, IoT-enabled trash can fill-level monitor and alerting system. Built around an ESP32 microcontroller, FillAlert tracks trash bin capacity using an ultrasonic sensor, monitors lid state and orientation via an MPU6050 IMU, and serves a live web interface while triggering automated Google Apps Script notifications when the bin reaches critical capacity.



### Technical Stack

Microcontroller: ESP32-WROOM-32 (30-Pin DevKit V1)



Sensors: A02YYUW Waterproof Ultrasonic Sensor (UART), MPU6050 Accelerometer/Gyroscope (I2C)



Hardware CAD: Custom PCB designed in KiCad 8.0



Firmware Stack: C++ / Arduino Framework (FreeRTOS Tasks, WebServer, HTTPClient, Wire, ArduinoJson)



Backend \& Alerts: Google Apps Script, Webhook Email API



### 

### Key System Features

**Real-Time Capacity Tracking**: Samples volumetric fill level using custom noise-filtering algorithms (getAverageDistance) and normalizes readings to adjustable bin heights.  



**Orientation \& Lid Detection**: Reads 3-axis accelerometer data from the MPU6050 via I2C to calculate real-time tilt angle (tiltAngle). Automatically flags lid openings (>45 degrees) to prevent false fill alerts.  



**Multi-Threaded Architecture**: Utilizes ESP32 FreeRTOS core pinning (xTaskCreatePinnedToCore) to offload network HTTP requests (emailTask) and hardware visual indicators (sensorErrorFlash) without blocking sensor polling. 



**Local Web Dashboard**: Hosts an embedded JSON API (/data) and responsive UI directly from the ESP32 to configure bin parameters and view status metrics.  



**Automated Email Webhooks**: Dispatches automated timestamped email notifications when the trash level breaches critical threshold zones.





### 

### Hardware \& PCB Architecture

The system transitions from breadboard prototyping to a custom-designed printed circuit board.





#### 

#### Hardware Layout Highlights

**Modular Socket Design**: Incorporates dual 1x15 (2.54mm pitch) female sockets spaced $0.9''$ apart to allow drop-in mounting of the 30-pin ESP32 without direct soldering.

**Current Protection**: Inline current-limiting resistors ($330\\Omega$) on all RGB LED channels to protect MCU output pins.  **Dedicated Sensor Headers**: Standardized pin break-outs for I2C and HardwareSerial UART sensor interface modules







### Setup and Flash Instructions 



1. Open FillAlert.ino in Arduino IDE or VS Code (PlatformIO).  
2. Install required dependencies:Adafruit MPU6050 \& Adafruit Sensor  TinyGPSPlus  ArduinoJson  
3. Update network credentials (ssid, password) and your Google Apps Script deployment URL in FillAlert.ino.  
4. Select board ESP32 Dev Module and upload at 115200 baud rate.  

