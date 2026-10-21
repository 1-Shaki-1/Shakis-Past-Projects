# DumperBoy ESP32

A Wi-Fi controlled mobile robot platform built on the ESP32 featuring active PID heading stabilization, ultrasonic obstacle detection, and an automated servo dumping mechanism.

\---

## Overview

DumperBoy is an open-source robotics platform designed for payload transportation and dumping. Built around an ESP32 microcontroller, it combines real-time WebSockets/HTTP web control with closed-loop PID control via an MPU6050 6-DOF IMU to maintain straight-line tracking and precise steering dynamics.

### Key Features

* **Web Control Dashboard:** Asynchronous WebServer serving an interactive HTML/JS interface for manual driving and telemetry monitoring.
* **PID Closed-Loop Heading Control:** Uses MPU6050 gyroscope data to continuously adjust motor PWM output, correcting for motor imbalance and surface friction drift.
* **Autonomous Safety \& Obstacle Ranging:** A02YYUW ultrasonic sensor integration to prevent forward collisions.
* **Servo Payload Dump:** High-torque servo actuation for automated material discharge.
* **Over-The-Air Calibration:** Onboard gyro bias calibration routine triggerable directly via web commands.

\---

## Hardware Architecture

### Components

* **Microcontroller:** ESP32-WROOM-32 Development Board
* **IMU Sensor:** MPU6050 (6-axis Accelerometer \& Gyroscope)
* **Distance Sensor:** A02YYUW Waterproof Ultrasonic Ranging Module
* **Motor Driver:** Dual H-Bridge Driver (TB6612FNG / L298N)
* **Actuators:** 2× Yellow TT DC Gearmotors + MG90S Servo Motor




## Software Architecture

The repository is structured into two core files:

```text
├── DumperBoy\_6.ino    # Main firmware, PID loops, sensor readings \& web routes
├── index.h             # Embedded HTML, CSS, and JS web dashboard frontend
└── README.md           # Project documentation

