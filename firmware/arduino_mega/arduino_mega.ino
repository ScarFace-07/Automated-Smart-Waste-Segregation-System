/*
  Automated Smart Waste Segregation System with Live Monitoring and Bin Alerts
  ------------------------------------------------------------------------------
  Board   : Arduino Mega 2560
  Purpose : Reads moisture, metal, and ultrasonic sensors to classify waste as
            Metal / Plastic / Moisture(Wet), drives the conveyor DC motor,
            rotates a NEMA17 stepper + MG995 servo to align the correct bin,
            and displays live status on a 16x2 I2C LCD.
  Author  : D. Siddu Maheswara Rao, J. Sri Sai Durga Veerendra,
            K. Sai Veerendra Vasu, P. Satya Krishna
  Guide   : Mr. P. Ramesh, M.Tech, Ph.D
  Dept    : Electronics & Communication Engineering,
            Aditya College of Engineering & Technology (A)
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Stepper Motor Pins
#define DIR_PIN 4
#define STEP_PIN 3
#define ENABLE_PIN 5

// Moisture Sensor & Buzzer
#define MOISTURE_PIN A2
#define BUZZER_PIN 12
#define EXTRA_BUZZER_PIN 8
const int moistureThreshold = 1000;

// Metal Sensor Pins
#define METAL_PULSE_PIN A0
#define METAL_CAP_PIN A1
const int metalThreshold = 40;

// Stepper Motor Config
const int stepDelay = 500;
const int stepsToMove = 75;

// Servo Motor
#define SERVO_PIN 9
Servo myServo;

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Ultrasonic Sensor Pins for Plastic Detection
#define TRIG_PIN 6
#define ECHO_PIN 7
const int plasticThreshold = 15; // Adjust based on testing

// L298N Motor Driver Pins
#define ENA 10 // PWM Speed Control
#define IN1 22 // Motor Direction
#define IN2 23

void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Monitoring...");

  // Stepper & Sensors Initialization
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(EXTRA_BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(EXTRA_BUZZER_PIN, LOW);
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  setupMetalSensor();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(ENABLE_PIN, LOW);

  // DC Motor Initialization
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  runMotor(100); // Start DC motor at speed 200
  Serial.println("Monitoring sensors...");
}

void loop() {
  int moistureValue = analogRead(MOISTURE_PIN);
  bool metalDetected = detectMetal();
  int distance = getUltrasonicDistance();

  Serial.print("Moisture Level: "); Serial.print(moistureValue);
  Serial.print(" | Metal: "); Serial.print(metalDetected ? "Detected" : "No Metal");
  Serial.print(" | Plastic Distance: "); Serial.println(distance);

  if (metalDetected) { // Highest Priority
    Serial.println("Metal Detected - Moving 75 steps Counterclockwise...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Metal Detected");
    activateBuzzer();
    stepMotor(stepsToMove, LOW);
    delay(1000);
    rotateServo(1000);
    delay(1000);
    stepMotor(stepsToMove, HIGH);
  }
  else if (distance < plasticThreshold) { // Second Priority
    Serial.println("Plastic Detected - Stepper remains idle.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Plastic Detected");
    rotateServo(500);
  }
  else if (moistureValue < moistureThreshold) { // Lowest Priority
    Serial.println("Moisture Detected - Moving 75 steps Clockwise...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture Detected");
    activateBuzzer();
    stepMotor(stepsToMove, HIGH);
    delay(1000);
    rotateServo(1000);
    delay(1000);
    stepMotor(stepsToMove, LOW);
  }
  else {
    Serial.println("No Object Detected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Object");
  }

  delay(2000);
}

void stepMotor(int steps, bool direction) {
  digitalWrite(DIR_PIN, direction);
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
  }
}

int getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void rotateServo(int speedDelay) {
  myServo.write(180);
  delay(speedDelay);
  myServo.write(0);
}

void setupMetalSensor() {
  pinMode(METAL_PULSE_PIN, OUTPUT);
  digitalWrite(METAL_PULSE_PIN, LOW);
  pinMode(METAL_CAP_PIN, INPUT);
}

bool detectMetal() {
  int minval = 1023, maxval = 0;
  long unsigned int sum = 0;

  for (int imeas = 0; imeas < 128; imeas++) {
    pinMode(METAL_CAP_PIN, OUTPUT);
    digitalWrite(METAL_CAP_PIN, LOW);
    delayMicroseconds(10);
    pinMode(METAL_CAP_PIN, INPUT);

    for (int ipulse = 0; ipulse < 8; ipulse++) {
      digitalWrite(METAL_PULSE_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(METAL_PULSE_PIN, LOW);
      delayMicroseconds(2);
    }

    int val = analogRead(METAL_CAP_PIN);
    minval = min(val, minval);
    maxval = max(val, maxval);
    sum += val;
  }

  sum -= minval;
  sum -= maxval;

  static long int sumsum = 0, skip = 0, diff = 0;
  if (sumsum == 0) sumsum = sum << 6;
  long int avgsum = (sumsum + 32) >> 6;
  diff = sum - avgsum;

  if (abs(diff) < (avgsum >> 10)) {
    sumsum = sumsum + sum - avgsum;
    skip = 0;
  } else {
    skip++;
  }

  if (skip > 32) {
    sumsum = sum << 6;
    skip = 0;
  }

  return abs(diff) > metalThreshold;
}

void activateBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(EXTRA_BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(EXTRA_BUZZER_PIN, LOW);
}

// Function to Run DC Motor with Speed Control
void runMotor(int speed) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, speed); // Speed range: 0-255
}
