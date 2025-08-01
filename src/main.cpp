#include <Arduino.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_MLX90614.h>

// this is to combine sensor MAX30100 and MLX90614 (GY 906) on the same I2C line
// the sensor only canwork when one of the .begin disabled and the i2c restarted
// so in this code the Sensor take turn to turn on

PulseOximeter pox;
Adafruit_MLX90614 mlx;

#define REPORTING_PERIOD_MS 100
#define TOGGLE_INTERVAL_MS 1000


bool poxInitialized = false;
bool i2cEnabled = true;  // true = MAX30100, false = MLX
uint32_t tsLastReport = 0;
uint32_t lastToggleTime = 0;


void setup() {
  Serial.begin(115200);
  Wire.begin();
}

void onBeatDetected() {
  Serial.println("Beat!");
}
void loop() {
  uint32_t now = millis();

  // Static variables to store last known readings
  static float heartRate = 0.0;
  static float spo2 = 0.0;
  static float ambientTemp = 0.0;
  static float objectTemp = 0.0;

  // Toggle between MAX30100 and MLX every TOGGLE_INTERVAL_MS
  if (now - lastToggleTime > TOGGLE_INTERVAL_MS) {
    lastToggleTime = now;

    if (i2cEnabled) {
      // --- DISABLE MAX30100 ---
      Wire.end();
      Serial.println("DISABLE MAX30100");
      poxInitialized = false;
      i2cEnabled = false;

      // --- ENABLE MLX90614 ---
      Wire.begin();

      if (mlx.begin()) {
        Serial.println("[MLX] MLX90614 initialized.");
        tsLastReport = now; // Reset MLX reporting timer
      } else {
        Serial.println("[MLX] Initialization failed");
      }

    } else {
      // --- DISABLE MLX ---
      Wire.end();
      Serial.println("DISABLE MLX");

      // --- ENABLE MAX30100 ---
      Wire.begin();
      Serial.println("Reinitializing MAX30100...");

      if (pox.begin()) {
        Serial.println("MAX30100 initialized successfully.");
        poxInitialized = true;
      } else {
        Serial.println("MAX30100 initialization FAILED.");
        poxInitialized = false;
      }

      i2cEnabled = true;
      tsLastReport = now; // Reset MAX30100 reporting timer
    }
  }

  // MAX30100 operation
  if (i2cEnabled && poxInitialized) {
    pox.update();
    if (now - tsLastReport > REPORTING_PERIOD_MS) {
      heartRate = pox.getHeartRate();
      spo2 = pox.getSpO2();

      Serial.print("[MAX] Heart rate: ");
      Serial.print(heartRate);
      Serial.print(" bpm / SpO2: ");
      Serial.print(spo2);
      Serial.println(" %");

      tsLastReport = now;
    }

  } else if (!i2cEnabled) {
    // MLX operation
    if (now - tsLastReport > REPORTING_PERIOD_MS) {
      ambientTemp = mlx.readAmbientTempC();
      objectTemp = mlx.readObjectTempC();

      Serial.print("[MLX] Ambient = ");
      Serial.print(ambientTemp);
      Serial.print(" *C\tObject = ");
      Serial.print(objectTemp);
      Serial.println(" *C");

      tsLastReport = now;
    }
  }

  // Combined output
  // Serial.print("[COMBINED] HR: ");
  // Serial.print(heartRate, 1);
  // Serial.print(" bpm | SpO2: ");
  // Serial.print(spo2, 1);
  // Serial.print(" % | Ambient: ");
  // Serial.print(ambientTemp, 1);
  // Serial.print(" C | Object: ");
  // Serial.print(objectTemp, 1);
  // Serial.println(" C");

  delay(10);  // Small delay to keep loop responsive
}

