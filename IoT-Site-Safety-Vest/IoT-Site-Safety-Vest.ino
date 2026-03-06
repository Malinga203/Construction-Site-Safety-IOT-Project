#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include "DHT.h"
#include <math.h>

// WiFi
const char* ssid = "Redmi Note 13";
const char* password = "nazeef123";
WiFiServer server(80);

// MQ2
#define MQ2_A0 34  

// MQ135
#define MQ135_A0 35

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

  // FALL DETECTION
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float AccX = ax / 16384.0;
  float AccY = ay / 16384.0;
  float AccZ = az / 16384.0;

  float totalAcc = sqrt(AccX*AccX + AccY*AccY + AccZ*AccZ);
  float totalGyro = sqrt(gx*gx + gy*gy + gz*gz) / 131.0;

  if (totalAcc < 0.5 && !freeFall) {
    freeFall = true;
    Serial.println("Free fall detected");
  }

  if (freeFall && totalGyro > 200) {
    rotationDetected = true;
    Serial.println("Fast rotation detected");
  }

  if (rotationDetected && totalAcc > 2.5) {
    impactDetected = true;
    impactTime = millis();
    Serial.println("Impact detected");
  }

  if (impactDetected) {
    if (millis() - impactTime > 3000) {
      if (totalGyro < 10) {
        Serial.println("FALL CONFIRMED");
        fallConfirmed = true;

        freeFall = false;
        rotationDetected = false;
        impactDetected = false;
      }
    }
  }

  // SENSOR READINGS
  int gasLevel = analogRead(MQ2_A0);
  int airQuality = analogRead(MQ135_A0);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    humidity = 0;
    temperature = 0;
  }

  // GAS STATUS (MQ2)
  String gasStatus;

  if (gasLevel < 1000)
    gasStatus = "Clean Air";
  else if (gasLevel < 1500)
    gasStatus = "Possible Smoke";
  else if (gasLevel < 2500)
    gasStatus = "Possible LPG or Methane";
  else
    gasStatus = "Heavy Gas Leak";

  // AIR QUALITY STATUS (MQ135)
  String airStatus;

  if (airQuality < 1000)
    airStatus = "Good Air";
  else if (airQuality < 2000)
    airStatus = "Moderate Pollution";
  else if (airQuality < 3000)
    airStatus = "Unhealthy Air";
  else
    airStatus = "Dangerous Air Quality";

  // SERIAL OUTPUT
  Serial.print("MQ2 Gas: ");
  Serial.print(gasLevel);

  Serial.print(" | MQ135 Air: ");
  Serial.print(airQuality);

  Serial.print(" | Temp: ");
  Serial.print(temperature);

  Serial.print("C | Humidity: ");
  Serial.print(humidity);

  Serial.print(" | Fall: ");
  Serial.println(fallConfirmed ? "YES" : "NO");


  // WIFI SERVER
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

  client.print("\"airQuality\":");
  client.print(airQuality);
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

  client.print("\"gasStatus\":\"");
  client.print(gasStatus);
  client.print("\",");

  client.print("\"airStatus\":\"");
  client.print(airStatus);
  client.print("\"");

  client.print("}");

  fallConfirmed = false;
  delay(100);
}
