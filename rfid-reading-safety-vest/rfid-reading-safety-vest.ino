#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

/* WIFI */
#define WIFI_SSID "Redmi Note 13"
#define WIFI_PASSWORD "nazeef123"

/* FIREBASE */
/* FIREBASE */
#define API_KEY "AIzaSyAeWvWmLjq_DeCtZPcptmgYt1HM41FtyxI"
#define DATABASE_URL "https://iot-smart-vest-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "malingalakmal2003@gmail.com"
#define USER_PASSWORD "123456"

/* RFID */
#define RST_PIN D0
#define SS_1 D1
#define SS_2 D2
#define SS_3 D8

MFRC522 reader1(SS_1, RST_PIN);
MFRC522 reader2(SS_2, RST_PIN);
MFRC522 reader3(SS_3, RST_PIN);

/* Firebase */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/* UID Extraction */
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

/* Log Event WITHOUT FirebaseJson */
void logWorkerEvent(String uid, String zone, String action)
{
  String basePath = "/logs/" + uid;

  String logKey = String(millis());

  String pathZone = basePath + "/" + logKey + "/zone";
  String pathAction = basePath + "/" + logKey + "/action";
  String pathTime = basePath + "/" + logKey + "/timestamp";

  Firebase.RTDB.setString(&fbdo, pathZone.c_str(), zone);
  Firebase.RTDB.setString(&fbdo, pathAction.c_str(), action);
  Firebase.RTDB.setInt(&fbdo, pathTime.c_str(), millis());
}

/* Zone Update Logic */
void updateWorkerZone(String uid, String newZone)
{
  String workerPath = "/workers/" + uid + "/zone";

  String previousZone = "";

  if (Firebase.RTDB.getString(&fbdo, workerPath))
  {
    previousZone = fbdo.stringData();
  }

  /* Same zone → EXIT */
  if (previousZone == newZone)
  {
    Firebase.RTDB.deleteNode(&fbdo, "/zones/" + newZone + "/" + uid);
    Firebase.RTDB.deleteNode(&fbdo, "/workers/" + uid);

    logWorkerEvent(uid, newZone, "EXIT");

    Serial.println(uid + " EXIT " + newZone);
    return;
  }

  /* First entry */
  if (previousZone == "")
  {
    Firebase.RTDB.setBool(&fbdo, "/zones/" + newZone + "/" + uid, true);
    Firebase.RTDB.setString(&fbdo, workerPath.c_str(), newZone);

    logWorkerEvent(uid, newZone, "ENTER");

    Serial.println(uid + " ENTER " + newZone);
    return;
  }

  /* Move zones */
  Firebase.RTDB.deleteNode(&fbdo, "/zones/" + previousZone + "/" + uid);
  logWorkerEvent(uid, previousZone, "EXIT");

  Firebase.RTDB.setBool(&fbdo, "/zones/" + newZone + "/" + uid, true);
  Firebase.RTDB.setString(&fbdo, workerPath.c_str(), newZone);

  logWorkerEvent(uid, newZone, "ENTER");

  Serial.println(uid + " MOVE " + newZone);
}

/* Setup */
void setup()
{
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi Connected");

  /* Firebase Config */
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

  Serial.println("RFID Zone Tracking Started");
}

/* Loop */
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

  delay(100);
}
