#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include "DHT.h"
#include <Firebase_ESP_Client.h>
#include <time.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WIFI
const char* ssid = "Redmi Note 13";
const char* password = "nazeef123";

// FIREBASE
#define API_KEY "AIzaSyAeWvWmLjq_DeCtZPcptmgYt1HM41FtyxI"
#define DATABASE_URL "https://iot-smart-vest-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "malingalakmal2003@gmail.com"
#define USER_PASSWORD "123456"

// VEST ID
#define VEST_ID "5BD386"

// SENSOR PINS
#define MQ2_A0 34
#define MQ135_A0 35
#define DHTPIN 4
#define DHTTYPE DHT22
#define REED_PIN 32
#define WIFI_LED 2

DHT dht(DHTPIN, DHTTYPE);
MPU6050 mpu;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

// MPU DATA
int16_t ax, ay, az, gx, gy, gz;

//FALL DETECTION VARIABLES
bool fallPossible = false;       
bool fallConfirmed = false;      
unsigned long fallStartTime = 0;
const unsigned long FALL_DURATION = 3000; 
int fallCount = 0;

// GET TIME 
String getTime()
{
  time_t now;
  time(&now);

  struct tm *timeinfo = localtime(&now);

  char buffer[30];
  strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",timeinfo);

  return String(buffer);
}

// GET ZONE 
String getZone()
{
  String path = "/workers/" + String(VEST_ID) + "/zone";

  if(Firebase.RTDB.getString(&fbdo,path))
    return fbdo.stringData();

  return "Unknown";
}

// STORE FALL LOG 
void logFall()
{
  String countPath = "/fall_logs/" + String(VEST_ID) + "/count";

  if(Firebase.RTDB.getInt(&fbdo,countPath))
    fallCount = fbdo.intData();
  else
    fallCount = 0;

  fallCount++;

  Firebase.RTDB.setInt(&fbdo,countPath,fallCount);

  String timeNow = getTime();
  String zone = getZone();

  String logPath = "/fall_logs/" + String(VEST_ID) + "/logs/fall" + String(fallCount);

  Firebase.RTDB.setString(&fbdo,logPath + "/time",timeNow);
  Firebase.RTDB.setString(&fbdo,logPath + "/zone",zone);

  Serial.println("===== FALL LOGGED =====");
  Serial.print("Fall #: "); Serial.println(fallCount);
  Serial.print("Zone: "); Serial.println(zone);
  Serial.print("Time: "); Serial.println(timeNow);
}


void setup()
{
  Serial.begin(115200);

  Wire.begin(21,22);

  pinMode(REED_PIN,INPUT_PULLUP);
  pinMode(WIFI_LED,OUTPUT);

  digitalWrite(WIFI_LED,LOW);

  mpu.initialize();
  dht.begin();

  WiFi.begin(ssid,password);

  Serial.print("Connecting WiFi");

  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  digitalWrite(WIFI_LED,HIGH);
  Serial.println("\nWiFi Connected");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config,&auth);
  Firebase.reconnectWiFi(true);

  configTime(0,0,"pool.ntp.org");
}

// FALL DETECTION FUNCTION 
void checkFall(float AccX)
{
  // Normal
  if(abs(AccX) > 0.7) 
  {
    // Reset horizontal timing
    fallPossible = false;
    fallStartTime = 0;

    // Reset confirmed fall only when vest is vertical
    if(fallConfirmed)
    {
      Serial.println("Vest returned vertical. Fall reset.");
      fallConfirmed = false;
    }
    return;
  }

  // Vest is horizontal 
  if(abs(AccX) <= 0.3) 
  {
    if(!fallPossible && !fallConfirmed) 
    {
      // Start timing horizontal position
      fallPossible = true;
      fallStartTime = millis();
      Serial.println("Possible Fall Detected: Horizontal Orientation");
    } 
    else if(fallPossible && (millis() - fallStartTime >= FALL_DURATION) && !fallConfirmed) 
    {
      // Confirm fall after 3 seconds horizontal
      fallConfirmed = true;
      Serial.println("===== FALL CONFIRMED =====");
      logFall(); // log to Firebase
      fallPossible = false; // stop timing, stay confirmed
    }
  }
}

void loop()
{
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

  float AccX = ax / 16384.0;
  float AccY = ay / 16384.0;
  float AccZ = az / 16384.0;

  float totalAcc = sqrt(AccX*AccX + AccY*AccY + AccZ*AccZ);

  Serial.print("ACC:");
  Serial.println(totalAcc);

  //FALL DETECTION 
  checkFall(AccX); //X axis is used to determine if the vest is vertical or horiznatl

  //SENSOR READINGS
  int gasLevel = analogRead(MQ2_A0);
  int airQuality = analogRead(MQ135_A0);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if(isnan(humidity)) humidity = 0;
  if(isnan(temperature)) temperature = 0;

  bool vestStatus = digitalRead(REED_PIN) == LOW;

  //GAS STATUS
  String gasStatus;
  if(gasLevel < 1000)
    gasStatus = "Clean Air";
  else if(gasLevel < 1500)
    gasStatus = "Possible Smoke";
  else if(gasLevel < 2500)
    gasStatus = "Possible LPG or Methane";
  else
    gasStatus = "Heavy Gas Leak";

  // AIR STATUS
  String airStatus;
  if(airQuality < 1000)
    airStatus = "Good Air";
  else if(airQuality < 2000)
    airStatus = "Moderate Pollution";
  else if(airQuality < 3000)
    airStatus = "Unhealthy Air";
  else
    airStatus = "Dangerous Air Quality";

  //FIREBASE UPDATE
  if(millis() - sendDataPrevMillis > 1000)
  {
    sendDataPrevMillis = millis();

    String vestPath = "/smart_vests/" + String(VEST_ID);

    Firebase.RTDB.setFloat(&fbdo, vestPath + "/temperature", temperature);
    Firebase.RTDB.setFloat(&fbdo, vestPath + "/humidity", humidity);

    Firebase.RTDB.setInt(&fbdo, vestPath + "/gasLevel", gasLevel);
    Firebase.RTDB.setString(&fbdo, vestPath + "/gasStatus", gasStatus);

    Firebase.RTDB.setInt(&fbdo, vestPath + "/airQuality", airQuality);
    Firebase.RTDB.setString(&fbdo, vestPath + "/airStatus", airStatus);

    Firebase.RTDB.setBool(&fbdo, vestPath + "/fall", fallConfirmed);

    Firebase.RTDB.setBool(&fbdo,"/workers/" + String(VEST_ID) + "/vestStatus",vestStatus);

    //Serial Debugging
    Serial.println("------ SENSOR DATA ------");
    Serial.print("Temp: "); Serial.println(temperature);
    Serial.print("Humidity: "); Serial.println(humidity);
    Serial.print("Gas: "); Serial.print(gasLevel);
    Serial.print(" | "); Serial.println(gasStatus);
    Serial.print("Air: "); Serial.print(airQuality);
    Serial.print(" | "); Serial.println(airStatus);
    Serial.print("Fall: "); Serial.println(fallConfirmed ? "YES":"NO");
    Serial.print("Vest: "); Serial.println(vestStatus ? "WORN":"REMOVED");
    Serial.println("-------------------------");
  }

  delay(200);
}