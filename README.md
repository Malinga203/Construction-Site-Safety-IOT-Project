Smart Safety Vest with RFID Location Tracking for Construction Site Safety
📌 Project Overview

The Smart Safety Vest with RFID Location Tracking is an IoT-based safety monitoring system designed to improve worker safety in construction environments. The system integrates RFID technology, environmental sensors, and motion detection to provide real-time monitoring of worker locations and hazardous conditions.

Construction sites are high-risk environments where accidents can occur due to restricted zone entry, gas exposure, heat stress, or falls. This project addresses these issues by using a wearable smart safety vest that collects sensor data and transmits it to a cloud-based monitoring system. Supervisors can monitor worker activity through a web dashboard and receive alerts when unsafe situations occur.

🎯 Objectives

Implement RFID-based worker identification and zone tracking

Monitor hazardous gas levels near workers

Detect unsafe temperature conditions and heat stress

Identify falls or sudden impact incidents

Provide real-time alerts and cloud monitoring

⚙️ System Architecture

The system consists of a smart safety vest equipped with sensors and RFID technology. RFID readers placed at different construction zones detect worker locations. Sensor data is processed by a microcontroller and transmitted to a cloud database for monitoring.

If a worker enters a restricted zone, experiences unsafe environmental conditions, or falls, the system generates instant alerts for supervisors.

🧰 Hardware Components

NodeMCU (ESP8266 / ESP32)

RFID Tag

RFID Reader (RC522)

MQ-2 Gas Sensor

MQ-135 Gas Sensor

DHT22 Temperature Sensor

MPU6050 Accelerometer & Gyroscope

LED Indicators

Buzzer

Power Supply

💻 Software Components

Arduino IDE

Firebase Realtime Database

Web Monitoring Dashboard

IoT Communication via WiFi

🚀 Key Features

Real-time worker location tracking

Restricted zone alert system

Gas exposure detection

Temperature and heat stress monitoring

Fall detection using motion sensors

Cloud-based safety dashboard

📊 Expected Outcomes

Improve construction site safety monitoring

Reduce accidents caused by restricted zone entry

Provide early detection of hazardous environmental conditions

Improve emergency response time

Maintain digital safety records and worker movement logs
