#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "time.h"

// --- CREDENTIALS ---
#define WIFI_SSID "------"
#define WIFI_PASSWORD "-----"
#define Web_API_KEY "-----"
#define DATABASE_URL "-----"
#define USER_EMAIL "-----" 
#define USER_PASS "-----"

// --- PIN DEFINITIONS ---
#define TDS_PIN 32
#define PH_SENSOR_PIN 34
#define ONE_WIRE_BUS 4
#define BUZZER_PIN 26
#define PUMP_PIN 27

// --- FUNCTION PROTOTYPES ---
float get_ai_health_score(float pH, float TDS, float Temp);
void apply_intelligent_control(float health_score, float current_ph, float current_tds);
void processData(AsyncResult &aResult);
void processGetPump(AsyncResult &aResult);

// --- SENSOR & FIREBASE GLOBALS ---
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; 
String uid = "";
const char* ntpServer = "pool.ntp.org";

float currentTempC = 25.0;
float tdsValue = 0;
float ph_act = 0;

#define VREF 3.3
#define SCOUNT 30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];

float slope = -4.04;            
float calibration_value = 16.87; 
unsigned long int avgval;
int buffer_arr[10], ph_temp;

bool isPumpOn = false;
bool manualPumpOverride = false; 

object_t jsonData, objTemp, objPH, objTDS, objTime, objAI;
JsonWriter writer;

void initWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi Connected!");
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return(0);
  time(&now);
  return now;
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i]; bTab[i] = bTab[i + 1]; bTab[i + 1] = bTemp;
      }
    }
  }
  return (iFilterLen & 1) ? bTab[(iFilterLen - 1) / 2] : (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- System Boot. ID: 2220791 ---");

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH); 
  isPumpOn = false;

  pinMode(TDS_PIN, INPUT);
  sensors.begin();
  analogSetAttenuation(ADC_11db);
  
  initWiFi();
  configTime(0, 0, ntpServer);

  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop() {
  app.loop();

  if (app.ready() && (millis() - lastSendTime >= sendInterval)) {
    lastSendTime = millis();
    
    if (uid == "" || uid == "null") {
      uid = app.getUid().c_str();
      if (uid == "" || uid == "null") return; 
    }
    
    String pumpControlPath = "/UsersData/" + uid + "/controls/pump";
    Database.get(aClient, pumpControlPath, processGetPump, "RTDB_GET_PUMP");

    // === 1. SENSOR READING (Temp, TDS, pH) ===
    sensors.requestTemperatures(); 
    float tempReading = sensors.getTempCByIndex(0);
    if(tempReading != DEVICE_DISCONNECTED_C) currentTempC = tempReading;

    for (int i = 0; i < SCOUNT; i++) {
      analogBuffer[i] = analogRead(TDS_PIN);
      delay(10);
    }
    for (int i = 0; i < SCOUNT; i++) analogBufferTemp[i] = analogBuffer[i];
    int medianValue = getMedianNum(analogBufferTemp, SCOUNT);
    float tdsVoltage = medianValue * (VREF / 4095.0);
    float compensationCoefficient = 1.0 + 0.02 * (currentTempC - 25.0);
    tdsValue = (133.42 * pow(tdsVoltage/compensationCoefficient, 3) - 255.86 * pow(tdsVoltage/compensationCoefficient, 2) + 857.39 * tdsVoltage/compensationCoefficient) * 0.5;
    if (tdsValue < 0) tdsValue = 0;

    for (int i = 0; i < 10; i++) {
      buffer_arr[i] = analogRead(PH_SENSOR_PIN);
      delay(30);
    }
    avgval = 0;
    for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
    float phVolt = (avgval / 6.0) * 3.3 / 4095.0;
    float instant_ph = (slope * phVolt) + calibration_value;
    ph_act = (ph_act == 0) ? instant_ph : (instant_ph * 0.15) + (ph_act * 0.85);

    // === 2. EDGE AI INFERENCE ===
    float ai_health_score = get_ai_health_score(ph_act, tdsValue, currentTempC);

    // === 3. INTELLIGENT AI CONTROL (Action) ===
    apply_intelligent_control(ai_health_score, ph_act, tdsValue);

    // === SERIAL OUTPUT ===
    Serial.println("\n--- POND TELEMETRY & AI ---");
    Serial.print("Temp: "); Serial.print(currentTempC, 1); Serial.println(" C");
    Serial.print("pH: "); Serial.print(ph_act, 2);
    Serial.print(" | TDS: "); Serial.print(tdsValue, 0); Serial.println(" ppm");
    Serial.print("=> AI Predicted Fish Health: "); Serial.print(ai_health_score, 1); Serial.println("%");
    Serial.print("Pump: "); Serial.print(isPumpOn ? "ON" : "OFF");
    Serial.println("---------------------------");

    // === FIREBASE PUSH ===
    unsigned long ts = getTime();
    if (ts > 0) {
      String currentPath = "/UsersData/" + uid + "/readings/" + String(ts);
      writer.create(objTemp, "/temperature", currentTempC);
      writer.create(objPH, "/ph", ph_act);
      writer.create(objTDS, "/tds", tdsValue);
      writer.create(objTime, "/timestamp", ts);
      writer.create(objAI, "/ai_health_score", ai_health_score);
      writer.join(jsonData, 5, objTemp, objPH, objTDS, objTime, objAI);
      Database.set<object_t>(aClient, currentPath, jsonData, processData, "RTDB_Send_Data");
    }
  }
}

// --- INTELLIGENT CONTROL IMPLEMENTATION ---
void apply_intelligent_control(float health_score, float current_ph, float current_tds) {
    // 1. Intelligent Buzzer Urgency
    if (health_score < 60) {
        tone(BUZZER_PIN, 1500); // Critical
    } else if (health_score < 80) {
        static unsigned long lastBeep = 0;
        if (millis() - lastBeep > 2000) {
            tone(BUZZER_PIN, 1000, 200); // Warning Pulse
            lastBeep = millis();
        }
    } else {
        noTone(BUZZER_PIN);
    }

    // 2. Intelligent Proactive Pump
    bool ai_pump_request = (health_score < 85); 
    bool safety_pump_request = (current_tds > 500 || current_ph < 6.5 || current_ph > 9.0);

    if (manualPumpOverride || ai_pump_request || safety_pump_request) {
        digitalWrite(PUMP_PIN, LOW); // ON
        isPumpOn = true;
    } else {
        digitalWrite(PUMP_PIN, HIGH); // OFF
        isPumpOn = false;
    }
}

// --- UPDATED EDGE AI MODEL (Decision Tree) ---
float get_ai_health_score(float pH, float TDS, float Temp) {
  // Input parameters mapped to model variables
  float temperature = Temp;
  float ph = pH;
  float tds = TDS;

  if (temperature <= 24.91) {
    if (temperature <= 24.41) {
      if (ph <= 6.83) {
        if (ph <= 6.64) {
          if (temperature <= 24.22) return 68.63; else return 71.15;
        } else {
          if (temperature <= 24.16) return 71.54; else return 74.10;
        }
      } else {
        if (temperature <= 21.90) {
          if (temperature <= 19.56) return 2.90; else return 36.58;
        } else {
          if (temperature <= 24.22) return 77.28; else return 79.54;
        }
      }
    } else {
      if (ph <= 6.82) {
        if (ph <= 6.68) {
          if (temperature <= 24.66) return 74.27; else return 77.23;
        } else {
          if (temperature <= 24.66) return 77.50; else return 80.41;
        }
      } else {
        if (temperature <= 24.66) {
          if (temperature <= 24.53) return 81.47; else return 83.00;
        } else {
          if (temperature <= 24.78) return 84.43; else return 85.89;
        }
      }
    }
  } else {
    if (temperature <= 25.47) {
      if (ph <= 6.82) {
        if (ph <= 6.67) {
          if (temperature <= 25.16) return 80.10; else return 82.94;
        } else {
          if (temperature <= 25.16) return 83.21; else return 86.43;
        }
      } else {
        if (temperature <= 25.16) {
          if (temperature <= 25.03) return 87.42; else return 88.88;
        } else {
          if (temperature <= 25.28) return 90.40; else return 92.27;
        }
      }
    } else {
      if (ph <= 6.81) {
        if (ph <= 4.81) {
          if (tds <= 144.08) return 39.36; else return 33.27;
        } else {
          if (temperature <= 29.00) return 90.15; else return 79.44;
        }
      } else {
        if (ph <= 12.66) {
          if (temperature <= 25.78) return 94.90; else return 98.91;
        } else {
          if (ph <= 14.38) return 12.93; else return 1.73;
        }
      }
    }
  }
  return 0;
}

void processData(AsyncResult &aResult) {}
void processGetPump(AsyncResult &aResult) {
  if (aResult.isResult()) {
    manualPumpOverride = (String(aResult.c_str()) == "1");
  }
}