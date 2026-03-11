#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

//WiFi
#define WIFI_SSID "Redmi Note 13"
#define WIFI_PASSWORD "nazeef123"

//Firebase
#define API_KEY "AIzaSyAeWvWmLjq_DeCtZPcptmgYt1HM41FtyxI"
#define DATABASE_URL "https://iot-smart-vest-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "malingalakmal2003@gmail.com"
#define USER_PASSWORD "123456"

//RFID
#define RST_PIN D0
#define SS_1 D1
#define SS_2 D2
#define SS_3 D8

MFRC522 reader1(SS_1, RST_PIN);
MFRC522 reader2(SS_2, RST_PIN);
MFRC522 reader3(SS_3, RST_PIN);

//Buzzers
#define BUZZER1 D3
#define BUZZER2 D4
#define BUZZER3 A0

//Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


bool dangerState = false;
unsigned long lastSensorRead = 0;

//RFID UID extraction 

String getUID(MFRC522 &reader)
{
  String uid = "";

  for (byte i = 0; i < reader.uid.size; i++)
  {
    uid += String(reader.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();
  return uid;
}

//Updating worker zone

void updateWorkerZone(String uid, String newZone)
{
  String workerPath = "/workers/" + uid + "/zone";
  String previousZone = "";

  if (Firebase.RTDB.getString(&fbdo, workerPath))
  {
    previousZone = fbdo.stringData();
  }

  if (previousZone == newZone)
  {
    Firebase.RTDB.deleteNode(&fbdo, "/zones/" + newZone + "/" + uid);
    Firebase.RTDB.deleteNode(&fbdo, "/workers/" + uid);

    Serial.println(uid + " EXIT " + newZone);
    return;
  }

  if (previousZone == "")
  {
    Firebase.RTDB.setBool(&fbdo, "/zones/" + newZone + "/" + uid, true);
    Firebase.RTDB.setString(&fbdo, workerPath.c_str(), newZone);

    Serial.println(uid + " ENTER " + newZone);
    return;
  }

  Firebase.RTDB.deleteNode(&fbdo, "/zones/" + previousZone + "/" + uid);

  Firebase.RTDB.setBool(&fbdo, "/zones/" + newZone + "/" + uid, true);
  Firebase.RTDB.setString(&fbdo, workerPath.c_str(), newZone);

  Serial.println(uid + " MOVE " + newZone);
}

//Danger Checked from vest

bool isDanger()
{
  int gas = 0;
  int air = 0;
  float temp = 0;
  bool fall = false;

  if (Firebase.RTDB.getInt(&fbdo, "/smart_vests/5BD386/gasLevel"))
    gas = fbdo.intData();

  if (Firebase.RTDB.getInt(&fbdo, "/smart_vests/5BD386/airQuality"))
    air = fbdo.intData();

  if (Firebase.RTDB.getFloat(&fbdo, "/smart_vests/5BD386/temperature"))
    temp = fbdo.floatData();

  if (Firebase.RTDB.getBool(&fbdo, "/smart_vests/5BD386/fall"))
    fall = fbdo.boolData();

  Serial.print("Gas: ");
  Serial.println(gas);

  Serial.print("Air: ");
  Serial.println(air);

  Serial.print("Temp: ");
  Serial.println(temp);

  Serial.print("Fall: ");
  Serial.println(fall);

  if (gas > 1500 || air > 2000 || temp > 40 || fall)
    return true;

  return false;
}

//Buzzer controller

void controlBuzzer()
{
  String zone = "";

  if (Firebase.RTDB.getString(&fbdo, "/workers/5BD386/zone"))
    zone = fbdo.stringData();

  digitalWrite(BUZZER1, LOW);
  digitalWrite(BUZZER2, LOW);
  digitalWrite(BUZZER3, LOW);

  if (!dangerState)
    return;

  if (zone == "Zone1")
    digitalWrite(BUZZER1, HIGH);

  if (zone == "Zone2")
    digitalWrite(BUZZER2, HIGH);

  if (zone == "Zone3")
    digitalWrite(BUZZER3, HIGH);
}


void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER1, OUTPUT);
  pinMode(BUZZER2, OUTPUT);
  pinMode(BUZZER3, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi Connected");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  SPI.begin();

  reader1.PCD_Init();
  reader2.PCD_Init();
  reader3.PCD_Init();

  Serial.println("RFID Zone System Ready");
}



void loop()
{
  if (reader1.PICC_IsNewCardPresent() && reader1.PICC_ReadCardSerial())
  {
    updateWorkerZone(getUID(reader1), "Zone1");
    reader1.PICC_HaltA();
    reader1.PCD_StopCrypto1();
  }

  if (reader2.PICC_IsNewCardPresent() && reader2.PICC_ReadCardSerial())
  {
    updateWorkerZone(getUID(reader2), "Zone2");
    reader2.PICC_HaltA();
    reader2.PCD_StopCrypto1();
  }

  if (reader3.PICC_IsNewCardPresent() && reader3.PICC_ReadCardSerial())
  {
    updateWorkerZone(getUID(reader3), "Zone3");
    reader3.PICC_HaltA();
    reader3.PCD_StopCrypto1();
  }

  /* Read vest sensors every 3 seconds */
  if (millis() - lastSensorRead > 3000)
  {
    dangerState = isDanger();
    lastSensorRead = millis();
  }

  controlBuzzer();

  delay(50);
}
