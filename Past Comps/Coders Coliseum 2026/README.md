# DumperBoy ESP32

A Wi-Fi controlled mobile robot platform built on the ESP32 featuring active PID heading stabilization, ultrasonic obstacle detection, and an automated servo dumping mechanism.

\---

## Overview

DumperBoy is an open-source robotics platform designed for payload transportation and dumping. Built around an ESP32 microcontroller, it combines real-time WebSockets/HTTP web control with closed-loop PID control via an MPU6050 6-DOF IMU to maintain straight-line tracking and precise steering dynamics.

### Key Features

* **Web Control Dashboard:** Asynchronous WebServer serving an interactive HTML/JS interface for manual driving and telemetry monitoring.
* **PID Closed-Loop Heading Control:** Uses MPU6050 gyroscope data to continuously adjust motor PWM output, correcting for motor imbalance and surface friction drift.
* **Autonomous Safety \& Obstacle Ranging:** HC-SR04 ultrasonic sensor integration to prevent forward collisions.
* **Servo Payload Dump:** High-torque servo actuation for automated material discharge.
* **Over-The-Air Calibration:** Onboard gyro bias calibration routine triggerable directly via web commands.

\---

## Hardware Architecture

### Components

* **Microcontroller:** ESP32-WROOM-32 Development Board
* **IMU Sensor:** MPU6050 (6-axis Accelerometer \& Gyroscope)
* **Distance Sensor:** HC-SR04 Ultrasonic Ranging Module
* **Motor Driver:** Dual H-Bridge Driver (TB6612FNG / L298N)
* **Actuators:** 2× Yellow TT DC Gearmotors + SG90 / MG996R Servo Motor
* **Power Supply:** 2S Li-ion / LiPo Battery (7.4V) with 5V DC-DC Buck Converter

### Circuit Pinout

|ESP32 Pin|Connected Hardware|Function|
|-|-|-|
|**GPIO 21**|MPU6050 SDA|I2C Data|
|**GPIO 22**|MPU6050 SCL|I2C Clock|
|**GPIO 12**|Motor Driver IN1|Left Motor Direction A|
|**GPIO 13**|Motor Driver IN2|Left Motor Direction B|
|**GPIO 14**|Motor Driver IN3|Right Motor Direction A|
|**GPIO 27**|Motor Driver IN4|Right Motor Direction B|
|**GPIO 25**|Motor Driver ENA|Left Motor PWM|
|**GPIO 26**|Motor Driver ENB|Right Motor PWM|
|**GPIO 4**|Servo Signal|Dumper Mechanism Actuator|
|**GPIO 5**|HC-SR04 Trig|Ultrasonic Trigger Pulse|
|**GPIO 18**|HC-SR04 Echo|Ultrasonic Echo Return|
|**GPIO 2**|Onboard LED|Status \& Calibration Indicator|

> \*\*Power Note:\*\* Keep logic and motor power on isolated rails. Ensure all ground pins are tied together to establish a common voltage reference.

\---

## Software Architecture

The repository is structured into two core files:

```text
├── DumperBoy\_6.ino    # Main firmware, PID loops, sensor readings \& web routes
├── index.h             # Embedded HTML, CSS, and JS web dashboard frontend
└── README.md           # Project documentation

