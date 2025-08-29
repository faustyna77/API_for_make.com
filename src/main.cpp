#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MPU6050.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
// ===== Konfiguracja WiFi =====


// ===== Konfiguracja PIR =====
const int pirPin = 13;
int pirState = LOW;

// ===== Konfiguracja HC-SR04 =====
const int trigPin = 12;
const int echoPin = 14;

// ===== Konfiguracja MPU6050 =====
MPU6050 mpu;

// ===== Webhook n8n =====


// ===== NTP =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000); // strefa +2h, aktualizacja co 60s
void sendSensorData(int pir, float distance, float gx, float gy, float gz) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<300> jsonDoc;
    jsonDoc["pir"] = pir;
    jsonDoc["distance"] = distance;
    jsonDoc["gyro"]["x"] = gx;
    jsonDoc["gyro"]["y"] = gy;
    jsonDoc["gyro"]["z"] = gz;
    jsonDoc["time"] = timeClient.getFormattedTime();

    String payload;
    serializeJson(jsonDoc, payload);

    int httpResponseCode = http.POST(payload);

    if(httpResponseCode > 0) {
      Serial.print("Dane wysłane! Kod: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Błąd wysyłki: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Brak połączenia z WiFi");
  }
}
float readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distanceCm = duration * 0.034 / 2;
  return distanceCm;
}

void readGyro(float &x, float &y, float &z) {
  x = mpu.getRotationX() / 131.0;
  y = mpu.getRotationY() / 131.0;
  z = mpu.getRotationZ() / 131.0;
}
void setup() {
  Serial.begin(9600);

  pinMode(pirPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Połączenie WiFi
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Połączono z WiFi!");

  // Start NTP
  timeClient.begin();

  // Start MPU6050
  Wire.begin();
  mpu.initialize();
 
}

void loop() {
  timeClient.update();

  pirState = digitalRead(pirPin);
  float distance = readDistance();
  float gyroX, gyroY, gyroZ;
  readGyro(gyroX, gyroY, gyroZ);

  sendSensorData(pirState, distance, gyroX, gyroY, gyroZ);

  delay(2000); // wysyłamy co 2s
}

// ===== Funkcje =====




