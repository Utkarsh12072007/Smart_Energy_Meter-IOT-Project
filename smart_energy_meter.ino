#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ============================================================================
// 1. WI-FI & TELEGRAM CREDENTIALS (Update before flashing to board)
// ============================================================================
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID   "YOUR_TELEGRAM_CHAT_ID"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ============================================================================
// 2. HARDWARE PIN CONFIGURATION & CALIBRATION CONSTANTS
// ============================================================================
#define VOLTAGE_PIN 4   // ZMPT101B OUT -> GPIO 4 (ADC1_CH3, 3.3V supply)
#define CURRENT_PIN 5   // ACS712 OUT   -> GPIO 5 (ADC1_CH4, 5V supply)

float VOLTAGE_CALIBRATION = 520.0;  // Scales ZMPT101B RMS voltage to ~230V AC
float CURRENT_SENSITIVITY = 0.185;  // 0.185 V/A for 5A ACS712 module at 5V

const float TARIFF_PER_KWH = 7.00;  // Domestic electricity tariff (INR/kWh)
const float OVERLOAD_WATTS = 150.0; // Threshold for Telegram overload alert

// ============================================================================
// 3. TELEMETRY & TIMING VARIABLES
// ============================================================================
float totalEnergy_kWh = 0.0;
unsigned long lastSampleMillis = 0;
unsigned long lastEnergyMillis = 0;
unsigned long lastAlertMillis  = 0;
unsigned long lastReportMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Configure 12-bit ADC (0-4095) across 0-3.3V input range
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("============================================================");
  Serial.println("IoT Smart Energy Monitoring System (ESP32-S3)");
  Serial.println("Sensors: ZMPT101B (GPIO 4) + ACS712 5A (GPIO 5)");
  Serial.println("============================================================");

  // Attempt 2.4 GHz Wi-Fi connection (non-blocking fallback to Serial mode)
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected! IP Address: " + WiFi.localIP().toString());
    secured_client.setInsecure();
    bot.sendMessage(CHAT_ID, "✅ Smart Energy Meter Online!", "");
  } else {
    Serial.println("\nWi-Fi Offline — Running in Local Serial Monitor Mode.");
  }

  lastEnergyMillis = millis();
  lastSampleMillis = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMillis < 2000) return; // Update interval: 2 seconds
  lastSampleMillis = now;

  // --- Step 1: Capture 1500 AC waveform samples (~150ms = 7.5 cycles at 50Hz) ---
  const int samples = 1500;
  static int vRaw[samples];
  static int iRaw[samples];
  double vSum = 0, iSum = 0;

  for (int n = 0; n < samples; n++) {
    vRaw[n] = analogRead(VOLTAGE_PIN);
    iRaw[n] = analogRead(CURRENT_PIN);
    vSum += vRaw[n];
    iSum += iRaw[n];
    delayMicroseconds(100);
  }

  // --- Step 2: Dynamic DC Midpoint Offset Calculation ---
  double vOffset = vSum / samples;
  double iOffset = iSum / samples;

  // --- Step 3: True RMS Voltage & Current Calculation ---
  double vSqSum = 0, iSqSum = 0;
  for (int n = 0; n < samples; n++) {
    double vAc = ((vRaw[n] - vOffset) / 4095.0) * 3.3;
    double iAc = ((iRaw[n] - iOffset) / 4095.0) * 3.3;
    vSqSum += vAc * vAc;
    iSqSum += iAc * iAc;
  }

  float voltageRMS = sqrt(vSqSum / samples) * VOLTAGE_CALIBRATION;
  float currentRMS = sqrt(iSqSum / samples) / CURRENT_SENSITIVITY;

  // Suppress zero-load ADC noise floor
  if (voltageRMS < 30.0) voltageRMS = 0.0;
  if (currentRMS < 0.04) currentRMS = 0.0;

  // --- Step 4: Active Power (W), Energy (kWh), and Cost (INR) ---
  float powerWatts = voltageRMS * currentRMS;

  float elapsedHours = (now - lastEnergyMillis) / 3600000.0;
  lastEnergyMillis = now;

  totalEnergy_kWh += (powerWatts / 1000.0) * elapsedHours;
  float totalCost_INR = totalEnergy_kWh * TARIFF_PER_KWH;

  // --- Step 5: Print Live Telemetry to Serial Monitor ---
  Serial.printf("Voltage: %.1f V | Current: %.3f A | Power: %.1f W | Energy: %.5f kWh | Cost: Rs %.4f\n",
                voltageRMS, currentRMS, powerWatts, totalEnergy_kWh, totalCost_INR);

  // --- Step 6: Automated Telegram Alerts (Active when Wi-Fi is connected) ---
  if (WiFi.status() == WL_CONNECTED) {
    // Immediate Overload Warning (30-second cooldown)
    if (powerWatts > OVERLOAD_WATTS && (now - lastAlertMillis > 30000)) {
      lastAlertMillis = now;
      String alertMsg = "⚠️ OVERLOAD ALERT!\nPower: " + String(powerWatts, 1) +
                        " W\nVoltage: " + String(voltageRMS, 1) +
                        " V\nCurrent: " + String(currentRMS, 3) + " A";
      bot.sendMessage(CHAT_ID, alertMsg, "");
    }

    // Periodic Energy Consumption Summary every 2 minutes during active load
    if (powerWatts > 10.0 && (now - lastReportMillis > 120000)) {
      lastReportMillis = now;
      String summaryMsg = "📊 Energy Meter Update:\nVoltage: " + String(voltageRMS, 1) +
                          " V\nCurrent: " + String(currentRMS, 3) +
                          " A\nPower: " + String(powerWatts, 1) +
                          " W\nEnergy: " + String(totalEnergy_kWh, 5) +
                          " kWh\nEst. Bill: Rs " + String(totalCost_INR, 2);
      bot.sendMessage(CHAT_ID, summaryMsg, "");
    }
  }
}
