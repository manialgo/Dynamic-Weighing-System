#include <HX711_ADC.h>
#include <LiquidCrystal.h>

// HX711 pins
const int HX711_dout = 4;
const int HX711_sck  = 5;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// LCD pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Calibration factor
float calibration_factor = 420.0;

// Threshold (grams)
const float LOAD_THRESHOLD = 20.0;

// Sampling
const int updateInterval = 100; // ms between readings

// Variables for averaging while object is on load cell
bool objectOnScale = false;
float sumWeight = 0;
unsigned long sampleCount = 0;

unsigned long lastUpdateTime = 0;

void setup() {
  Serial.begin(9600);
  delay(10);

  lcd.begin(16, 2);
  lcd.print("Load Cell Init");

  LoadCell.begin();
  lcd.setCursor(0, 1);
  lcd.print("Taring...");
  LoadCell.start(2000, true);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    lcd.clear();
    lcd.print("HX711 ERROR!");
    while (1);
  }

  LoadCell.setCalFactor(calibration_factor);
  lcd.clear();
  lcd.print("Ready...");
  Serial.println("System Ready (Conveyor Mode)");
  Serial.println("Send 't' to tare or number (g) to calibrate");
}

void loop() {
  static bool newDataReady = false;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady && millis() - lastUpdateTime > updateInterval) {
    float weight = LoadCell.getData();
    if (weight < 0) weight = 0;

    // --- Object arrives ---
    if (weight > LOAD_THRESHOLD) {
      if (!objectOnScale) {
        // First detection of object
        objectOnScale = true;
        sumWeight = 0;
        sampleCount = 0;
        lcd.clear();
        lcd.print("Object Detected");
        Serial.println("Object Detected");
      }

      // Accumulate readings while object is on scale
      sumWeight += weight;
      sampleCount++;
    }

    // --- Object leaves ---
    else if (objectOnScale && weight <= LOAD_THRESHOLD) {
      objectOnScale = false;

      if (sampleCount > 0) {
        float avgWeight = sumWeight / sampleCount;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Final Weight:");
        lcd.setCursor(0, 1);
        lcd.print(avgWeight, 2);
        lcd.print(" g");

        Serial.print("✅ Object Weight: ");
        Serial.print(avgWeight, 2);
        Serial.println(" g");
      }

      // Reset for next item
      sumWeight = 0;
      sampleCount = 0;
      delay(1000);
      lcd.clear();
      lcd.print("Waiting...");
    }

    newDataReady = false;
    lastUpdateTime = millis();
  }

  // --- Serial commands (tare & calibration) ---
  if (Serial.available() > 0) {
    if (Serial.peek() == 't') {
      Serial.read();
      LoadCell.tareNoDelay();
      lcd.clear();
      lcd.print("Taring...");
      Serial.println("Taring...");
    } else {
      float knownWeight = Serial.parseFloat();
      if (knownWeight > 0) {
        lcd.clear();
        lcd.print("Calib:");
        lcd.setCursor(0, 1);
        lcd.print(knownWeight, 1);
        lcd.print(" g");

        Serial.print("Calibrating with ");
        Serial.print(knownWeight);
        Serial.println(" g...");

        LoadCell.refreshDataSet();
        float newCal = LoadCell.getNewCalibration(knownWeight);
        calibration_factor = newCal;
        LoadCell.setCalFactor(newCal);

        lcd.clear();
        lcd.print("New Cal:");
        lcd.setCursor(0, 1);
        lcd.print(newCal, 2);
        Serial.print("✅ New Cal Factor: ");
        Serial.println(newCal, 2);
      }
      while (Serial.available() > 0) Serial.read();
    }
  }

  // --- Show when tare done ---
  if (LoadCell.getTareStatus()) {
    lcd.clear();
    lcd.print("Tare Complete");
    Serial.println("Tare Complete");
  }
}
