//70 line working code

#include <HX711_ADC.h>

// Pin connections for HX711
const int HX711_dout = 4;  // DOUT pin
const int HX711_sck  = 5;  // SCK pin

// Create HX711 object
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Calibration factor (adjust to match your load cell)
float calibration_factor = 1000.0;  

// Timing
unsigned long lastPrintTime = 0;
const int printInterval = 500; // print every 500ms

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println("HX711 Load Cell Example");

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
  Serial.print("Calibration factor set to: ");
  Serial.println(calibration_factor);

  Serial.println("Place a known weight and adjust calibration_factor until reading matches.");
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

  // Serial commands
  if (Serial.available() > 0) {
    char c = Serial.read();

    if (c == 't') {  // Tare command
      LoadCell.tareNoDelay();
      Serial.println("Tare started...");
    }
  }

  // Report tare completion
  if (LoadCell.getTareStatus()) {
    Serial.println("Tare complete");
  }
}
