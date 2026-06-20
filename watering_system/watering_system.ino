// =============================================================================
// HARDWARE PIN CONFIG
// =============================================================================
// GPIO34 is input-only and ADC-capable — good for the analog moisture sensor.
// GPIO26 drives the MOSFET gate; any digital output GPIO works here.
#define MOISTURE_SENSOR_PIN 34
#define PUMP_PIN            26

// =============================================================================
// BEHAVIOR CONFIG  — tune these without touching the logic below
// =============================================================================
// ADC reads 0–4095. Dry soil reads HIGH (near 4095), wet soil reads LOW.
// Calibrate by reading your specific sensor via Serial Monitor:
//   - Insert probe into very dry soil, note the value → set as DRY_THRESHOLD
//   - Insert probe into just-watered soil, note the value (should be much lower)
#define MOISTURE_DRY_THRESHOLD 2500   // water when reading is ABOVE this value

#define PUMP_RUN_MS       5000UL    // how long the pump runs per watering cycle (ms)
#define CHECK_INTERVAL_MS 30000UL   // how often to read the sensor (ms)
#define COOLDOWN_MS       120000UL  // wait after watering before checking again (ms)

// ADC samples averaged per reading — reduces ESP32 ADC noise
#define ADC_SAMPLES 5

// =============================================================================
// STATE
// =============================================================================
static unsigned long lastCheckTime       = 0;
static unsigned long lastWaterTime       = 0;
static unsigned long pumpStartTime       = 0;
static unsigned long lastCooldownLogTime = 0;
static bool          isPumping           = false;

// =============================================================================
// HELPERS
// =============================================================================
int readMoisture() {
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(MOISTURE_SENSOR_PIN);
    delay(10);
  }
  return (int)(sum / ADC_SAMPLES);
}

void startPump() {
  isPumping     = true;
  pumpStartTime = millis();
  digitalWrite(PUMP_PIN, HIGH);
  Serial.println("[PUMP] Started");
}

void stopPump() {
  isPumping     = false;
  lastWaterTime = millis();
  digitalWrite(PUMP_PIN, LOW);
  Serial.println("[PUMP] Stopped");
}

// =============================================================================
// STARTUP HARDWARE CHECK
// =============================================================================
// Pulses the pump briefly and reads the sensor so you can verify wiring via
// Serial Monitor before the main loop begins.
#define PUMP_TEST_MS 500   // short pulse duration — enough to hear/feel the pump

void startupCheck() {
  Serial.println("--- Hardware Check ---");

  // --- Pump test ---
  Serial.println("[CHECK] Pump: pulsing for 500 ms — listen/feel for activation...");
  digitalWrite(PUMP_PIN, HIGH);
  delay(PUMP_TEST_MS);
  digitalWrite(PUMP_PIN, LOW);
  Serial.println("[CHECK] Pump: pulse done. Did it run? If not, check MOSFET wiring and 5V rail.");

  // --- Sensor test ---
  int moisture = readMoisture();
  Serial.print("[CHECK] Sensor ADC = "); Serial.print(moisture);
  if (moisture < 100 || moisture > 4000) {
    Serial.println("  <-- WARNING: value out of expected range (100–4000). Check sensor wiring.");
  } else {
    Serial.println("  <-- OK");
  }

  Serial.println("--- Hardware Check Done ---");
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500); // let Serial stabilize

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // ensure pump is off at boot

  Serial.println("=== Watering System Starting ===");
  Serial.print("Dry threshold : "); Serial.println(MOISTURE_DRY_THRESHOLD);
  Serial.print("Pump run time : "); Serial.print(PUMP_RUN_MS / 1000); Serial.println("s");
  Serial.print("Check interval: "); Serial.print(CHECK_INTERVAL_MS / 1000); Serial.println("s");
  Serial.print("Cooldown      : "); Serial.print(COOLDOWN_MS / 1000); Serial.println("s");
  Serial.println("================================");

  startupCheck();
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  unsigned long now = millis();

  // 1. Stop pump if it has run long enough
  if (isPumping && (now - pumpStartTime >= PUMP_RUN_MS)) {
    stopPump();
  }

  // 2. Check moisture if not pumping and not in cooldown and interval has passed
  bool inCooldown = (now - lastWaterTime < COOLDOWN_MS);
  bool intervalDue = (now - lastCheckTime >= CHECK_INTERVAL_MS);

  if (!isPumping && !inCooldown && intervalDue) {
    lastCheckTime = now;

    int moisture = readMoisture();
    Serial.print("[SENSOR] Moisture ADC = "); Serial.println(moisture);

    if (moisture > MOISTURE_DRY_THRESHOLD) {
      Serial.println("[INFO] Soil is dry — starting pump");
      startPump();
    } else {
      Serial.println("[INFO] Soil is moist — no action");
    }
  }

  // 3. Periodic status log while idle — once per second during cooldown
  if (!isPumping && inCooldown && (now - lastCooldownLogTime >= 1000UL)) {
    lastCooldownLogTime = now;
    unsigned long remaining = COOLDOWN_MS - (now - lastWaterTime);
    Serial.print("[INFO] Cooldown active — "); Serial.print(remaining / 1000); Serial.println("s remaining");
  }

  // ==========================================================================
  // FUTURE: WiFi / MQTT / Home Assistant
  // ==========================================================================
  // When adding connectivity, this loop is already non-blocking so you can:
  //   - Call mqttClient.loop() here
  //   - Publish moisture readings and pump state to HA via MQTT
  //   - Subscribe to a topic for manual pump trigger from HA
  // Recommended libraries: PubSubClient (MQTT), WiFi.h (built-in)
  // Recommended HA integration: MQTT Discovery (auto-adds device to HA)
  // ==========================================================================
}
