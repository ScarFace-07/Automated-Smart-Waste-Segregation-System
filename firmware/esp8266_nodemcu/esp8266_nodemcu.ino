/*
  Automated Smart Waste Segregation System — ESP8266 NodeMCU Firmware
  ---------------------------------------------------------------------
  Board   : ESP8266 NodeMCU
  Purpose : Reads three ultrasonic sensors (one per bin) to monitor fill
            levels, pushes readings to Firebase Realtime Database, and
            sends a Firebase Cloud Messaging (FCM) push notification when
            a bin crosses the "almost full" threshold. Also drives the
            conveyor motor via an L298N driver.

  IMPORTANT: Wi-Fi credentials and API keys live in secrets.h, which is
  NOT committed to this repository (see .gitignore). Copy
  secrets.h.example to secrets.h and fill in your own values before
  compiling.
*/

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

#define FCM_URL "https://fcm.googleapis.com/fcm/send"

// Ultrasonic Sensor Pins
#define TRIG1 D1
#define ECHO1 D2
#define TRIG2 D5
#define ECHO2 D6
#define TRIG3 D7
#define ECHO3 D8

// L298N Motor Control Pins
#define MOTOR_IN1 D3 // GPIO0
#define MOTOR_IN2 D4 // GPIO2
#define MOTOR_ENA D0 // GPIO16 (PWM Speed Control)

FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;
WiFiClientSecure client;

void setup() {
  Serial.begin(115200);

  // Ultrasonic sensor pins
  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

  // L298N Motor Driver pins
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  client.setInsecure(); // Allow HTTPS requests
  startMotor(); // Start the motor continuously
}

// Function to get distance from ultrasonic sensor
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

// Function to send FCM notification
void sendNotification(String title, String message) {
  Serial.println("Sending Notification...");
  WiFiClientSecure client;
  client.setInsecure(); // Bypass SSL verification
  client.connect("fcm.googleapis.com", 443);

  if (!client.connected()) {
    Serial.println("Connection to FCM failed!");
    return;
  }

  StaticJsonDocument<512> jsonData;
  jsonData["to"] = "/topics/bin_alert"; // Topic-based Notification
  jsonData["priority"] = "high";

  JsonObject notification = jsonData.createNestedObject("notification");
  notification["title"] = title;
  notification["body"] = message;

  String requestBody;
  serializeJson(jsonData, requestBody);

  client.println("POST " FCM_URL " HTTP/1.1");
  client.println("Host: fcm.googleapis.com");
  client.println("Authorization: key=" FCM_SERVER_KEY);
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(requestBody.length()));
  client.println();
  client.print(requestBody);

  Serial.println("Notification Sent!");
}

// Function to start the motor continuously
void startMotor() {
  Serial.println("Starting Motor...");
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_ENA, 255); // Full speed
}

void loop() {
  float bin1 = getDistance(TRIG1, ECHO1);
  float bin2 = getDistance(TRIG2, ECHO2);
  float bin3 = getDistance(TRIG3, ECHO3);

  Serial.print("Bin 1: "); Serial.println(bin1);
  Serial.print("Bin 2: "); Serial.println(bin2);
  Serial.print("Bin 3: "); Serial.println(bin3);

  Firebase.setFloat(firebaseData, "/bins/bin1", bin1);
  Firebase.setFloat(firebaseData, "/bins/bin2", bin2);
  Firebase.setFloat(firebaseData, "/bins/bin3", bin3);

  // Send notifications if bins are full
  if (bin1 <= 10) sendNotification("Bin 1 Full!", "Please empty Bin 1.");
  if (bin2 <= 10) sendNotification("Bin 2 Full!", "Please empty Bin 2.");
  if (bin3 <= 10) sendNotification("Bin 3 Full!", "Please empty Bin 3.");

  delay(1000); // Delay before next reading
}
