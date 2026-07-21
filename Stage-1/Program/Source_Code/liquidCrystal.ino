#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// LCD setup: address 0x27, 16 columns, 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pins
const int HX711_dout = 4; // MCU > HX711 dout pin
const int HX711_sck = 5;  // MCU > HX711 sck pin

// HX711 constructor
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(1000);

  LoadCell.begin();
  unsigned long stabilizingtime = 2000; // Stabilization time for better precision
  boolean _tare = true; // Set to false to skip tare during startup
  LoadCell.start(stabilizingtime, _tare);

  float calibrationValue = 696.0; // Default calibration value
  #if defined(ESP8266) || defined(ESP32)
    EEPROM.begin(512);
  #endif
  EEPROM.get(calVal_eepromAdress, calibrationValue);
  if (calibrationValue <= 0 || isnan(calibrationValue)) {
    calibrationValue = 696.0; // Fallback to default if invalid
    Serial.println("No valid calibration value in EEPROM, using default: 696.0");
  } else {
    Serial.print("Loaded calibration value from EEPROM: ");
    Serial.println(calibrationValue);
  }

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HX711 Timeout!");
    delay(5000); // Wait before continuing
    Serial.println("Continuing with default calibration...");
  } else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup is complete");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Startup Complete");
    delay(1000);
  }

  while (!LoadCell.update());
  calibrate(); // Start calibration procedure
}

void loop() {
  static boolean newDataReady = false;
  const int serialPrintInterval = 500; // Update every 500ms

  // Check for new data/start next conversion
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from the dataset
  if (newDataReady && millis() > t + serialPrintInterval) {
    float i = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(i);

    // LCD Print
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DIYEngineers.com");
    lcd.setCursor(0, 1);
    lcd.print("Weight(g): ");
    lcd.print(int(i));
    newDataReady = false;
    t = millis();
  }

  // Receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell.tareNoDelay();
      Serial.println("Tare command received");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Taring...");
    } else if (inByte == 'r') {
      calibrate();
    }
    // Clear serial buffer
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  // Check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tare Complete");
    delay(1000);
  }
}

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell on a level, stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration");
  lcd.setCursor(0, 1);
  lcd.print("Send 't' to tare");

  boolean _resume = false;
  unsigned long timeout = millis() + 30000; // 30-second timeout
  while (_resume == false && millis() < timeout) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        LoadCell.tareNoDelay();
        Serial.println("Tare command received");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Taring...");
      }
      while (Serial.available() > 0) {
        Serial.read(); // Clear serial buffer
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tare Complete");
      _resume = true;
      delay(1000);
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for tare input, calibration aborted.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    delay(2000);
    return;
  }

  Serial.println("Now, place your known mass on the load cell.");
  Serial.println("Then send the weight of this mass (e.g., 100.0) from serial monitor.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place known mass");
  lcd.setCursor(0, 1);
  lcd.print("Send weight");

  float known_mass = 0;
  _resume = false;
  timeout = millis() + 30000; // 30-second timeout
  while (_resume == false && millis() < timeout) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass > 0 && !isnan(known_mass)) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mass: ");
        lcd.print(known_mass);
        _resume = true;
      } else {
        Serial.println("Invalid mass value, please enter a positive number.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Mass!");
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("Place known mass");
        lcd.setCursor(0, 1);
        lcd.print("Send weight");
      }
      while (Serial.available() > 0) {
        Serial.read(); // Clear serial buffer
      }
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for mass input, calibration aborted.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    delay(2000);
    return;
  }

  LoadCell.refreshDataSet(); // Refresh dataset to ensure accurate measurement
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass);

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cal Value: ");
  lcd.setCursor(0, 1);
  lcd.print(newCalibrationValue);
  delay(2000);

  Serial.print("Save this value to EEPROM address ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Save to EEPROM?");
  lcd.setCursor(0, 1);
  lcd.print("Send y/n");

  _resume = false;
  timeout = millis() + 30000; // 30-second timeout
  while (_resume == false && millis() < timeout) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        #if defined(ESP8266) || defined(ESP32)
          EEPROM.begin(512);
        #endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
        #if defined(ESP8266) || defined(ESP32)
          EEPROM.commit();
        #endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Saved to EEPROM");
        _resume = true;
        delay(1000);
      } else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Not Saved");
        _resume = true;
        delay(1000);
      }
      while (Serial.available() > 0) {
        Serial.read(); // Clear serial buffer
      }
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for EEPROM save input, calibration aborted.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    delay(2000);
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("***");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration Done");
  delay(1000);
}