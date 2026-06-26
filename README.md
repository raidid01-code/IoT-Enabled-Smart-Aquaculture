# IoT-Enabled Smart Aquaculture: Real-Time Water Quality Monitoring

A B.Sc. thesis project — an ESP32-based embedded IoT system for real-time 
aquaculture water quality monitoring with on-device Edge AI inference and 
automated pump control.

Overview:

Traditional aquaculture monitoring relies on manual checks, risking delayed 
response to critical water quality changes. This system provides continuous 
automated monitoring, Edge AI-based fish health prediction, and intelligent 
pump actuation — all synced live to a Firebase-connected web dashboard.

Features:

Real-time sensing of pH, TDS, and Temperature (every 10 seconds)
On-device Decision Tree model for Fish Health Score prediction (0–100%)
Intelligent pump actuation based on AI health score and safety thresholds
Buzzer alerts: warning pulse (<80%) and critical alarm (<60%)
Manual pump override via Firebase remote control
Live dashboard: real-time readings, last 30 data trend charts, downloadable JSON logs
Temperature compensation applied to TDS readings to eliminate probe drift

Tech Stack:

Hardware: ESP32, pH sensor, TDS sensor, DS18B20 temperature sensor, relay module, buzzer
Firmware: Arduino IDE (C/C++)
Backend: Firebase Realtime Database (authenticated user sessions)
Frontend: HTML, CSS, JavaScript
AI Model: Decision Tree classifier — 96.7% classification accuracy

System Architecture

Sensors → ESP32 (Edge AI Inference + Control Logic) → Firebase Realtime 
Database → Web Dashboard

AI Model — Edge Inference

The Decision Tree model runs entirely on the ESP32 (no cloud inference). 
It takes pH, TDS, and Temperature as inputs and outputs a Fish Health Score. 
Pump actuation triggers automatically when:
Health Score < 85% (proactive AI response)
TDS > 500 ppm (safety threshold)
pH < 6.5 or pH > 9.0 (safety threshold)
