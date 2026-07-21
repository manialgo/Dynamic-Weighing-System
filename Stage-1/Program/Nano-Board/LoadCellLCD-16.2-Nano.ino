#include <HX711_ADC.h>
#include <LiquidCrystal.h>

// HX711 connections
const int HX711_dout = 4;  // DT
const int HX711_sck  = 5;  // SCK

HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Default calibration factor (adjust after calibration)
float calibration_factor = 420.0;

// LCD pin mapping: RS=7, E=8, D4=9, D5=10, D6=11, D7=12
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Timing
unsigned long lastPrintTime = 0;
const int printInterval = 500;

void setup() {
  Serial.begin(9600);  // Nano works best at 9600 baud
  delay(10);

  // LCD init
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Load Cell Init");

  Serial.println("HX711 Load Cell Example with Auto Calibration");

  // Initialize load cell
  LoadCell.begin();

  Serial.println("Taring... remove all weight");
  lcd.setCursor(0, 1);
  lcd.print("Taring...");

  LoadCell.start(2000, true);  // 2 sec stabilization + tare
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Error: HX711 not connected.");
    lcd.clear();
    lcd.print("HX711 ERROR!");
    while (1); // stop execution
  }
  Serial.println("Tare complete");
  lcd.clear();
  lcd.print("Tare Complete");

  // Set calibration factor
  LoadCell.setCalFactor(calibration_factor);
  Serial.print("Initial Cal: ");
  Serial.println(calibration_factor);
  lcd.setCursor(0, 1);
  lcd.print("Cal:");
  lcd.print(calibration_factor, 1);

  delay(2000);
  lcd.clear();
  lcd.print("Place Weight...");
}

void loop() {
  static boolean newDataReady = false;

  if (LoadCell.update()) {
    newDataReady = true;
  }

  if (newDataReady && millis() - lastPrintTime > printInterval) {
    float weight = LoadCell.getData();

    // Serial
    Serial.print("Weight: ");
    Serial.print(weight, 2);
    Serial.println(" g");

    // LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Weight:");
    lcd.setCursor(0, 1);
    lcd.print(weight, 2);
    lcd.print(" g");

    newDataReady = false;
    lastPrintTime = millis();
  }

  // Handle Serial commands
  if (Serial.available() > 0) {
    if (Serial.peek() == 't') {
      Serial.read();
      LoadCell.tareNoDelay();
      Serial.println("Tare started...");
      lcd.clear();
      lcd.print("Taring...");
    } else {
      float knownWeight = Serial.parseFloat();
      if (knownWeight > 0) {
        Serial.print("Calibrating: ");
        Serial.print(knownWeight);
        Serial.println(" g");

        lcd.clear();
        lcd.print("Calib with:");
        lcd.setCursor(0, 1);
        lcd.print(knownWeight, 1);
        lcd.print(" g");

        LoadCell.refreshDataSet();
        float newCal = LoadCell.getNewCalibration(knownWeight);
        calibration_factor = newCal;
        LoadCell.setCalFactor(newCal);

        Serial.print("✅ New Cal: ");
        Serial.println(newCal, 2);

        lcd.clear();
        lcd.print("New Cal:");
        lcd.setCursor(0, 1);
        lcd.print(newCal, 2);
      }
      while (Serial.available() > 0) Serial.read();
    }
  }

  if (LoadCell.getTareStatus()) {
    Serial.println("Tare complete");
    lcd.clear();
    lcd.print("Tare Complete");
  }
}
