#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include "DHT.h"
#include <math.h>

// WiFi
const char* ssid = "WIFI_SSID";
const char* password = "PASSWORD";
WiFiServer server(80);

// MQ2
#define MQ2_A0 34  

// DHT22
#define DHTPIN 4 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// MPU6050
MPU6050 mpu;

int16_t ax, ay, az;
int16_t gx, gy, gz;

bool freeFall = false;
bool rotationDetected = false;
bool impactDetected = false;
bool fallConfirmed = false;

unsigned long impactTime = 0;

void setup() {

  Serial.begin(115200);
  delay(100);

  Wire.begin(21, 22);   

  mpu.initialize();

  if (mpu.testConnection())
    Serial.println("MPU6050 Connected Successfully!");
  else
    Serial.println("MPU6050 Connection Failed!");

  dht.begin();

  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {

  //Fall Detection
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float AccX = ax / 16384.0;
  float AccY = ay / 16384.0;
  float AccZ = az / 16384.0;

  float totalAcc = sqrt(AccX*AccX + AccY*AccY + AccZ*AccZ);
  float totalGyro = sqrt(gx*gx + gy*gy + gz*gz) / 131.0;

  //Freefall
  if (totalAcc < 0.5 && !freeFall) {
    freeFall = true;
    Serial.println("Free fall detected");
  }

  //Rotation
  if (freeFall && totalGyro > 200) {
    rotationDetected = true;
    Serial.println("Fast rotation detected");
  }

  //Impact
  if (rotationDetected && totalAcc > 2.5) {
    impactDetected = true;
    impactTime = millis();
    Serial.println("Impact detected");
  }

  //Inactivity
  if (impactDetected) {
    if (millis() - impactTime > 3000) {
      if (totalGyro < 10) {
        Serial.println("FALL CONFIRMED");
        fallConfirmed = true;

        // Reset states
        freeFall = false;
        rotationDetected = false;
        impactDetected = false;
      }
    }
  }

  //Sensor Readings
  int gasLevel = analogRead(MQ2_A0);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    humidity = 0;
    temperature = 0;
  }

  // Gas Status
  String gasStatus;

  if (gasLevel < 1000)
    gasStatus = "Clean Air";
  else if (gasLevel < 1500)
    gasStatus = "Possible Alcohol/Smoke";
  else if (gasLevel < 2500)
    gasStatus = "Possible LPG or Methane";
  else
    gasStatus = "Heavy Gas Concentration Detected!";

  // Serial Output
  Serial.print("Gas: ");
  Serial.print(gasLevel);
  Serial.print(" | Temp: ");
  Serial.print(temperature);
  Serial.print("C | Humidity: ");
  Serial.print(humidity);
  Serial.print(" | Fall: ");
  Serial.println(fallConfirmed ? "YES" : "NO");

  //WiFi Server
  WiFiClient client = server.available();
  if (!client) return;

  client.readStringUntil('\r');

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();

  client.print("{");
  client.print("\"gasLevel\":");
  client.print(gasLevel);
  client.print(",");

  client.print("\"temperature\":");
  client.print(temperature);
  client.print(",");

  client.print("\"humidity\":");
  client.print(humidity);
  client.print(",");

  client.print("\"fall\":");
  client.print(fallConfirmed ? "true" : "false");
  client.print(",");

  client.print("\"status\":\"");
  client.print(gasStatus);
  client.print("\"");
  client.print("}");

  fallConfirmed = false;
  delay(100);
}
