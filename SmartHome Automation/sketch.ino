#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Pin Definitions
#define DHTPIN 2          // DHT22 sensor pin
#define DHTTYPE DHT22     // DHT22 (AM2302)
#define PIRPIN 3          // PIR sensor pin
#define LDRPIN A0         // LDR sensor pin
#define RED_LED 4         // Red LED pin
#define GREEN_LED 5       // Green LED pin
#define BLUE_LED 6        // Blue LED pin
#define BUZZER 7          // Buzzer pin

// LCD setup
LiquidCrystal_I2C lcd(0*27, 16, 2);  // I2C address 0*27, 16 chars, 2 line display

// DHT22 sensor
DHT dht(DHTPIN, DHTTYPE);

// Global Variables
float temperature = 0;
float humidity = 0;
int lightLevel = 0;
bool motionDetected = false;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialUpdate = 0;
unsigned long lastMotionTime = 0;
bool powerSavingMode = false;

void setup() {
  // Initialize components
  lcd.begin(0*27, 16, 2);
  lcd.backlight();
  
  dht.begin();
  
  pinMode(PIRPIN, INPUT);
  pinMode(LDRPIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Serial.begin(9600);

  lcd.print("Smart Home Init");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read sensor data
  readSensors();

  // Update LCD display every 5 seconds
  if (millis() - lastDisplayUpdate > 5000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // Send data to Serial every 10 seconds
  if (millis() - lastSerialUpdate > 10000) {
    sendSerialData();
    lastSerialUpdate = millis();
  }

  // Check and respond to motion and light levels
  handleMotionAndLight();

  // Check for power-saving mode
  handlePowerSaving();
}

void readSensors() {
  // Read temperature and humidity from DHT22
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Error handling for DHT22
  if (isnan(temperature) || isnan(humidity)) {
    lcd.setCursor(0, 0);
    lcd.print("Temp/Hum Error");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T: ");
    lcd.print(temperature);
    lcd.print("C ");
    lcd.setCursor(0, 1);
    lcd.print("H: ");
    lcd.print(humidity);
    lcd.print("% ");
  }

  // Read light level from LDR
  lightLevel = analogRead(LDRPIN);

  // Read motion state from PIR sensor
  motionDetected = digitalRead(PIRPIN);
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Light: ");
  lcd.print(map(lightLevel, 0, 1023, 0, 100));
  lcd.print("%");
}

void sendSerialData() {
  // Create a JSON object with sensor data
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["temperature"] = temperature;
  jsonDoc["humidity"] = humidity;
  jsonDoc["lightLevel"] = map(lightLevel, 0, 1023, 0, 100);
  jsonDoc["motionDetected"] = motionDetected;

  // Serialize JSON data and send to Serial
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  Serial.println(jsonString);
}

void handleMotionAndLight() {
  // LED Indicators based on temperature
  if (temperature > 30) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
  } else if (temperature >= 20 && temperature <= 30) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  } else {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
  }

  // Buzzer Alarm when motion is detected and it's dark
  if (motionDetected && lightLevel < 300) {
    digitalWrite(BUZZER, HIGH);
    delay(3000);  // Sound for 3 seconds
    digitalWrite(BUZZER, LOW);
  }
}

void handlePowerSaving() {
  if (motionDetected) {
    lastMotionTime = millis();
    if (powerSavingMode) {
      lcd.backlight();
      powerSavingMode = false;
    }
  } else if (millis() - lastMotionTime > 30000 && !powerSavingMode) {
    lcd.noBacklight();
    powerSavingMode = true;
  }
}
