# IoT-Enabled Smart Aquaculture: Real-Time Water Quality Monitoring

A thesis project — an ESP32-based embedded IoT system for real-time 
aquaculture water quality monitoring with on-device Edge AI inference and 
automated pump control.

# Overview:

Traditional aquaculture monitoring relies on manual checks, risking delayed 
response to critical water quality changes. This system provides continuous 
automated monitoring, Edge AI-based fish health prediction, and intelligent 
pump actuation — all synced live to a Firebase-connected web dashboard.

# Features:

1. Real-time sensing of pH, TDS, and Temperature (every 10 seconds)
2. On-device Decision Tree model for Fish Health Score prediction (0–100%)
3. Intelligent pump actuation based on AI health score and safety thresholds
4. Buzzer alerts: warning pulse (<80%) and critical alarm (<60%)
5. Live dashboard: real-time readings, last 30 data trend charts, downloadable JSON logs
6. Temperature compensation applied to TDS readings to eliminate probe drift

# Tech Stack:

1. Hardware: ESP32, pH sensor, TDS sensor, DS18B20 temperature sensor, relay module, buzzer
2. Firmware: Arduino IDE (C/C++)
3. Backend: Firebase Realtime Database (authenticated user sessions)
4. Frontend: HTML, CSS, JavaScript
5. AI Model: Decision Tree classifier — 96.7% classification accuracy

# System Architecture:

Sensors → ESP32 (Edge AI Inference + Control Logic) → Firebase Realtime Database → Web Dashboard

# AI Model — Edge Inference

- The Decision Tree model runs entirely on the ESP32 (no cloud inference). 
- It takes pH, TDS, and Temperature as inputs and outputs a Fish Health Score. (External Dataset [https://www.kaggle.com/datasets/bobsis/small-aquaculture-fishpond] analyzed with marzing the dataset from my device)
- Pump actuation triggers automatically when:
Health Score < 85% (proactive AI response),
TDS > 500 ppm (safety threshold),
pH < 6.5 or pH > 9.0 (safety threshold).
