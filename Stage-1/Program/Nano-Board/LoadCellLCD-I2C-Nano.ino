#include <HX711_ADC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin connections for HX711
const int HX711_dout = 4;  // DOUT pin
const int HX711_sck  = 5;  // SCK pin

// Create HX711 object
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Default calibration factor (rough guess, will be corrected)
float calibration_factor = 420.0;  

// LCD object (I2C address 0x27, 16x2 display)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timing
unsigned long lastPrintTime = 0;
const int printInterval = 500; // print every 500ms

void setup() {
  Serial.begin(57600);
  delay(10);

  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Load Cell Init");

  Serial.println("HX711 Load Cell Example with Auto Calibration");

  // Initialize load cell
  LoadCell.begin();

  Serial.println("Taring... remove all weight");
  lcd.setCursor(0,1);
  lcd.print("Taring...");

  LoadCell.start(2000, true);  // 2 second stabilization + tare
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
  lcd.setCursor(0,1);
  lcd.print("Cal:");
  lcd.print(calibration_factor,1);

  delay(2000);
  lcd.clear();
  lcd.print("Place Weight...");
}

void loop() {
  static boolean newDataReady = false;

  // Check if new data is ready
  if (LoadCell.update()) {
    newDataReady = true;
  }

  // Print weight at intervals
  if (newDataReady && millis() - lastPrintTime > printInterval) {
    float weight = LoadCell.getData();
    
    // Serial output
    Serial.print("Weight: ");
    Serial.print(weight, 2); // 2 decimal places
    Serial.println(" g");

    // LCD output
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Weight:");
    lcd.setCursor(0,1);
    lcd.print(weight, 2);
    lcd.print(" g");

    newDataReady = false;
    lastPrintTime = millis();
  }

  // Handle Serial commands
  if (Serial.available() > 0) {
    if (Serial.peek() == 't') {
      Serial.read(); // clear 't'
      LoadCell.tareNoDelay();
      Serial.println("Tare started...");
      lcd.clear();
      lcd.print("Taring...");
    } else {
      // Assume user entered a number (known weight)
      float knownWeight = Serial.parseFloat();
      if (knownWeight > 0) {
        Serial.print("Calibrating: ");
        Serial.print(knownWeight);
        Serial.println(" g");

        lcd.clear();
        lcd.print("Calib with:");
        lcd.setCursor(0,1);
        lcd.print(knownWeight,1);
        lcd.print(" g");

        // Refresh readings for accuracy
        LoadCell.refreshDataSet();
        // Calculate new calibration factor
        float newCal = LoadCell.getNewCalibration(knownWeight);
        calibration_factor = newCal;
        LoadCell.setCalFactor(newCal);

        Serial.print("✅ New Cal: ");
        Serial.println(newCal, 2);

        lcd.clear();
        lcd.print("New Cal:");
        lcd.setCursor(0,1);
        lcd.print(newCal,2);
      }
      // Clear buffer
      while (Serial.available() > 0) Serial.read();
    }
  }

  // Report tare completion
  if (LoadCell.getTareStatus()) {
    Serial.println("Tare complete");
    lcd.clear();
    lcd.print("Tare Complete");
  }
}
