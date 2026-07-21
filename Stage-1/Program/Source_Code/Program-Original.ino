#include <HX711_ADC.h>

// Pin connections for HX711
const int HX711_dout = 4;  // DOUT pin
const int HX711_sck  = 5;  // SCK pin

// Create HX711 object
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Default calibration factor (rough guess, will be corrected)
float calibration_factor = 420.0;  

// Timing
unsigned long lastPrintTime = 0;
const int printInterval = 500; // print every 500ms

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println("HX711 Load Cell Example with Auto Calibration");

  // Initialize load cell
  LoadCell.begin();

  Serial.println("Taring... remove all weight from the scale");
  LoadCell.start(2000, true);  // 2 second stabilization + tare
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Error: HX711 not connected properly.");
    while (1); // stop execution
  }
  Serial.println("Tare complete");

  // Set calibration factor
  LoadCell.setCalFactor(calibration_factor);
  Serial.print("Initial calibration factor: ");
  Serial.println(calibration_factor);

  Serial.println();
  Serial.println("👉 Place a known weight on the scale.");
  Serial.println("👉 Then type its value in grams into Serial Monitor and press Enter.");
  Serial.println("👉 Type 't' to tare anytime.");
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
    Serial.print("Weight: ");
    Serial.print(weight, 2); // 2 decimal places
    Serial.println(" g");
    newDataReady = false;
    lastPrintTime = millis();
  }

  // Handle Serial commands
  if (Serial.available() > 0) {
    if (Serial.peek() == 't') {
      Serial.read(); // clear 't'
      LoadCell.tareNoDelay();
      Serial.println("Tare started...");
    } else {
      // Assume user entered a number (known weight)
      float knownWeight = Serial.parseFloat();
      if (knownWeight > 0) {
        Serial.print("Calibrating using known weight: ");
        Serial.print(knownWeight);
        Serial.println(" g");

        // Refresh readings for accuracy
        LoadCell.refreshDataSet();
        // Calculate new calibration factor
        float newCal = LoadCell.getNewCalibration(knownWeight);
        calibration_factor = newCal;
        LoadCell.setCalFactor(newCal);

        Serial.print("✅ New calibration factor set to: ");
        Serial.println(newCal, 2);
        Serial.println("Place another weight to verify calibration.");
      }
      // Clear buffer
      while (Serial.available() > 0) Serial.read();
    }
  }

  // Report tare completion
  if (LoadCell.getTareStatus()) {
    Serial.println("Tare complete");
  }
}
