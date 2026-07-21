#include <HX711_ADC.h>
#include <LiquidCrystal.h>

// -------- hardware pins --------
const int HX711_dout = 4;
const int HX711_sck  = 5;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// LCD (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// -------- calibration & thresholds --------
float calibration_factor = 420.0;     // adjust if needed / save to EEPROM later
const float ENTRY_THRESHOLD = 20.0;  // when object is considered arrived (grams)
const float EXIT_THRESHOLD  = 15.0;  // hysteresis threshold for leaving (grams)

// -------- sampling / averaging parameters (tweakable) --------
const int SAMPLE_INTERVAL_MS    = 50;   // 1 sample every 50 ms (20 Hz)
const int MAX_SAMPLES           = 150;  // circular buffer size (150 * 50ms = 7.5s capacity)
const int CONSECUTIVE_BELOW     = 6;    // how many consecutive below-EXIT readings mean "left"
const int TRIM_PERCENT          = 10;   // trimmed mean: drop top/bottom 10%
const unsigned long POST_DELAY_MS = 500; // ignore new objects for this ms after result

// -------- internal buffers & state --------
float samples[MAX_SAMPLES];
int writeIndex = 0;
int storedCount = 0;           // number of samples currently in buffer (<= MAX_SAMPLES)

bool objectOnScale = false;
int consecutiveBelow = 0;
unsigned long lastSampleMillis = 0;
unsigned long ignoreUntil = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  lcd.begin(16, 2);
  lcd.print("Load Cell Init");

  LoadCell.begin();
  lcd.setCursor(0,1);
  lcd.print("Taring...");
  LoadCell.start(2000, true);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    lcd.clear();
    lcd.print("HX711 ERROR!");
    while (1); // halt
  }

  LoadCell.setCalFactor(calibration_factor);
  lcd.clear();
  lcd.print("Ready...");
  Serial.println("Conveyor mode ready");
  Serial.println("Send 't' to tare or a number (g) to calibrate");
}

void loop() {
  // keep HX711 updated
  LoadCell.update();

  // sampling timing
  if (millis() - lastSampleMillis >= SAMPLE_INTERVAL_MS) {
    lastSampleMillis = millis();
    float weight = LoadCell.getData();
    if (weight < 0.0) weight = 0.0;

    // if we're in a short ignore period (after a result), don't detect
    if (millis() < ignoreUntil) {
      // do nothing (you might still want to keep calling LoadCell.update() above)
    } else {
      // Detect object arrival
      if (!objectOnScale) {
        if (weight >= ENTRY_THRESHOLD) {
          // start collecting for a new object
          objectOnScale = true;
          storedCount = 0;
          writeIndex = 0;
          consecutiveBelow = 0;
          lcd.clear();
          lcd.print("Object Detected");
          Serial.println("Object detected - collecting samples...");
        }
      }

      // Collect data while object is present
      if (objectOnScale) {
        // store into circular buffer
        samples[writeIndex] = weight;
        writeIndex++;
        if (writeIndex >= MAX_SAMPLES) writeIndex = 0;
        if (storedCount < MAX_SAMPLES) storedCount++;

        // check for leaving condition
        if (weight < EXIT_THRESHOLD) {
          consecutiveBelow++;
        } else {
          consecutiveBelow = 0;
        }

        // if object left (several consecutive low readings)
        if (consecutiveBelow >= CONSECUTIVE_BELOW) {
          // compute final weight now
          float finalWeight = computeTrimmedMean(samples, storedCount);
          // display
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Final Weight:");
          lcd.setCursor(0,1);
          lcd.print(finalWeight, 2);
          lcd.print(" g");

          Serial.print("===> Object Weight (trimmed mean): ");
          Serial.print(finalWeight, 2);
          Serial.println(" g");

          // prepare for next object: reset buffer & state
          storedCount = 0;
          writeIndex = 0;
          objectOnScale = false;
          consecutiveBelow = 0;

          // block detection for a short time (post-processing debounce)
          ignoreUntil = millis() + POST_DELAY_MS;
        }
      }
    }
  }

  // Serial controls: tare ('t') or known weight for calibration
  if (Serial.available() > 0) {
    if (Serial.peek() == 't') {
      Serial.read();
      LoadCell.tareNoDelay();
      lcd.clear();
      lcd.print("Taring...");
      Serial.println("Taring...");
    } else {
      float known = Serial.parseFloat();
      if (known > 0) {
        lcd.clear();
        lcd.print("Calib:");
        lcd.setCursor(0,1);
        lcd.print(known, 1);
        lcd.print(" g");
        Serial.print("Calibrating with ");
        Serial.print(known);
        Serial.println(" g ...");
        LoadCell.refreshDataSet();
        float newCal = LoadCell.getNewCalibration(known);
        calibration_factor = newCal;
        LoadCell.setCalFactor(newCal);
        Serial.print("New cal factor: ");
        Serial.println(newCal, 2);
        lcd.clear();
        lcd.print("New Cal:");
        lcd.setCursor(0,1);
        lcd.print(newCal, 2);
      }
      // clear serial buffer
      while (Serial.available() > 0) Serial.read();
    }
  }

  // Show tare completion if it happens in background
  if (LoadCell.getTareStatus()) {
    lcd.clear();
    lcd.print("Tare Complete");
    Serial.println("Tare Complete");
  }
}

// ----------------- helper: trimmed mean -----------------
// sorts the storedCount items in place and returns trimmed mean
float computeTrimmedMean(float arr[], int n) {
  if (n <= 0) return 0.0;

  // simple in-place selection sort (n is small/limited by MAX_SAMPLES)
  for (int i = 0; i < n - 1; i++) {
    int idxMin = i;
    for (int j = i + 1; j < n; j++) {
      if (arr[j] < arr[idxMin]) idxMin = j;
    }
    if (idxMin != i) {
      float tmp = arr[i];
      arr[i] = arr[idxMin];
      arr[idxMin] = tmp;
    }
  }

  // trimming
  int trim = (n * TRIM_PERCENT) / 100;  // number of elements to drop from each side
  if (trim * 2 >= n) {
    // if trimming would drop everything, just return plain mean
    float s = 0;
    for (int i = 0; i < n; i++) s += arr[i];
    return s / n;
  }

  int start = trim;
  int end = n - trim; // exclusive
  float sum = 0;
  for (int i = start; i < end; i++) sum += arr[i];
  float mean = sum / (end - start);
  return mean;
}
