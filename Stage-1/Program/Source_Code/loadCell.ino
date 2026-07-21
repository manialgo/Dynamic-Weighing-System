#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

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

  LoadCell.begin();
  unsigned long stabilizingtime = 2000; // Stabilization time for better precision
  boolean _tare = true; // Set to false to skip tare during startup
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    delay(5000); // Wait before continuing instead of infinite loop
    Serial.println("Continuing with default calibration...");
  } else {
    // Load calibration value from EEPROM if available
    float storedCalValue;
    EEPROM.get(calVal_eepromAdress, storedCalValue);
    if (storedCalValue != 0 && !isnan(storedCalValue)) {
      LoadCell.setCalFactor(storedCalValue);
      Serial.print("Loaded calibration value from EEPROM: ");
      Serial.println(storedCalValue);
    } else {
      LoadCell.setCalFactor(1.0); // Default calibration value
      Serial.println("No valid calibration value in EEPROM, using default: 1.0");
    }
    Serial.println("Startup is complete");
  }

  while (!LoadCell.update());
  calibrate(); // Start calibration procedure
}

void loop() {
  static boolean newDataReady = false;
  const int serialPrintInterval = 0; // Increase to slow down serial print

  // Check for new data/start next conversion
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from the dataset
  if (newDataReady && millis() > t + serialPrintInterval) {
    float i = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(i);
    newDataReady = false;
    t = millis();
  }

  // Receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell.tareNoDelay();
      Serial.println("Tare command received");
    } else if (inByte == 'r') {
      calibrate();
    } else if (inByte == 'c') {
      changeSavedCalFactor();
    }
    // Clear serial buffer
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  // Check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell on a level, stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  unsigned long timeout = millis() + 30000; // 30-second timeout
  while (_resume == false && millis() < timeout) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        LoadCell.tareNoDelay();
        Serial.println("Tare command received");
      }
      // Clear serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for tare input, calibration aborted.");
    return;
  }

  Serial.println("Now, place your known mass on the load cell.");
  Serial.println("Then send the weight of this mass (e.g., 100.0) from serial monitor.");

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
        _resume = true;
      } else {
        Serial.println("Invalid mass value, please enter a positive number.");
      }
      // Clear serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for mass input, calibration aborted.");
    return;
  }

  LoadCell.refreshDataSet(); // Refresh dataset to ensure accurate measurement
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass);

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM address ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

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
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
      // Clear serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for EEPROM save input, calibration aborted.");
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}

void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  Serial.println("***");
  Serial.print("Current calibration value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, e.g., 696.0");

  boolean _resume = false;
  float newCalibrationValue;
  unsigned long timeout = millis() + 30000; // 30-second timeout
  while (_resume == false && millis() < timeout) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue > 0 && !isnan(newCalibrationValue)) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      } else {
        Serial.println("Invalid calibration value, please enter a positive number.");
      }
      // Clear serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for calibration input, change aborted.");
    return;
  }

  Serial.print("Save this value to EEPROM address ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

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
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
      // Clear serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }
  if (millis() >= timeout) {
    Serial.println("Timeout waiting for EEPROM save input, change aborted.");
  }

  Serial.println("End change calibration value");
  Serial.println("***");
}