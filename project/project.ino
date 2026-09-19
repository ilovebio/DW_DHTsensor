/*
 * ESP32-C3 + DHT22 + SSD1306 OLED + WT3000 Voice Module (English TTS)
 * Platform: Arduino IDE
 *
 * Required libraries:
 *   - DHT sensor library (by Adafruit)
 *   - Adafruit Unified Sensor
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Board selection: ESP32C3 Dev Module
 * USB CDC On Boot: Enabled
 *
 * NOTE about WT3000 firmware versions:
 *   - Version "A" (Chinese-only): can synthesize Chinese and English LETTERS,
 *     but NOT English words. "Hello" would be spelled out "H-e-l-l-o".
 *   - Version "D" (Chinese + English): can synthesize both Chinese and English words.
 *   Use command 0x038D to query the firmware version and confirm.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

/* ------------------------ User configuration ------------------------ */
#define DHT_PIN         4          // DHT22 data pin
#define DHT_TYPE        DHT22

#define I2C_SDA_PIN     8          // OLED SDA pin
#define I2C_SCL_PIN     9          // OLED SCL pin

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_ADDR       0x3C       // Common addresses: 0x3C or 0x3D

#define KEY_PIN         3          // Push button pin
#define KEY_ACTIVE_LOW  true       // Button is active low (pressed = LOW)
#define KEY_DEBOUNCE_MS 50         // Debounce time in milliseconds

#define READ_INTERVAL   2000       // DHT22 read interval in ms (min 2000)

// WT3000 voice module serial pins (ESP32-C3 side)
#define WT3000_RX_PIN   5          // ESP32 RX -> WT3000 TX
#define WT3000_TX_PIN   6          // ESP32 TX -> WT3000 RX
/* -------------------------------------------------------------------- */

// DHT sensor object
DHT dht(DHT_PIN, DHT_TYPE);

// OLED display object
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Global variables: last valid readings
float g_temp = NAN;
float g_hum  = NAN;

// Timing control
unsigned long lastReadTime = 0;

// Function prototypes
void initOLED();
void initDHT();
void initWT3000();
void readDHT();
void updateDisplay();
void printSerial();
void handleKey();
void sendVoiceReport(float temp, float hum);
void sendWT3000Command(uint16_t cmd, const uint8_t *data, uint8_t len);
void sendTTS(const char *text);

/* ======================== setup ======================== */
void setup() {
  // Debug serial port
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("========================================");
  Serial.println("  ESP32-C3 Temp & Humidity + WT3000");
  Serial.println("========================================");

  // Initialize I2C bus
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize OLED display
  initOLED();

  // Initialize DHT22 sensor
  initDHT();

  // Initialize WT3000 voice module
  initWT3000();

  // Initialize push button
  pinMode(KEY_PIN, KEY_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  Serial.printf("Button pin: GPIO%d (%s trigger)\n", KEY_PIN,
                KEY_ACTIVE_LOW ? "low level" : "high level");

  Serial.println("Initialization complete. Starting loop...");
  Serial.println("----------------------------------------");
}

/* ======================== loop ======================== */
void loop() {
  unsigned long now = millis();

  // Read DHT22 every READ_INTERVAL milliseconds
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    readDHT();        // Read sensor
    updateDisplay();  // Refresh OLED
    printSerial();    // Output to serial monitor
  }

  // Check button (triggers voice broadcast)
  handleKey();
}

/* ==================== Initialization functions ==================== */

void initOLED() {
  Serial.print("Initializing OLED... ");

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("FAILED!");
    Serial.println("Please check:");
    Serial.println("  1. OLED address (try 0x3C or 0x3D)");
    Serial.println("  2. SDA/SCL wiring");
    Serial.println("  3. Module power supply");
    while (true) delay(1000);  // Halt for debugging
  }

  Serial.println("OK");
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Booting...");
  oled.display();
}

void initDHT() {
  Serial.print("Initializing DHT22... ");
  dht.begin();
  delay(100);  // Allow sensor to stabilize
  Serial.println("OK");
}

void initWT3000() {
  Serial.print("Initializing WT3000... ");
  Serial1.begin(9600, SERIAL_8N1, WT3000_RX_PIN, WT3000_TX_PIN);
  delay(300);  // Wait for module power-up

  // Keep default GB2312 encoding. ASCII (English letters, digits, symbols)
  // is transmitted unchanged under GB2312, so pure English text works fine.
  Serial.println("OK (default GB2312 mode, ASCII passthrough)");
}

/* ==================== Functional functions ==================== */

void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // Celsius

  if (isnan(h) || isnan(t)) {
    Serial.println("[WARN] DHT22 read failed, keeping last valid values");
    return;
  }

  // Sanity check
  if (t < -40.0 || t > 80.0 || h < 0.0 || h > 100.0) {
    Serial.println("[WARN] DHT22 reading out of range");
    return;
  }

  g_temp = t;
  g_hum  = h;
}

void updateDisplay() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Title bar
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Temp & Humidity");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperature
  oled.setTextSize(2);
  oled.setCursor(0, 18);
  oled.print("T:");
  if (isnan(g_temp)) {
    oled.print("--.-");
  } else {
    oled.print(g_temp, 1);
  }
  oled.setTextSize(1);
  oled.print(" C");

  // Humidity
  oled.setTextSize(2);
  oled.setCursor(0, 42);
  oled.print("H:");
  if (isnan(g_hum)) {
    oled.print("--.-");
  } else {
    oled.print(g_hum, 1);
  }
  oled.setTextSize(1);
  oled.print(" %");

  oled.display();
}

void printSerial() {
  Serial.printf("Temp: %.1f C  Humidity: %.1f %%\n", g_temp, g_hum);
}

/*
 * Non-blocking button debounce.
 * Samples the button every loop iteration and detects stable state changes.
 * Only triggers on a valid press (falling edge for active-low).
 */
void handleKey() {
  static bool lastReading = HIGH;          // Previous raw reading
  static bool stableState = HIGH;          // Debounced stable state
  static unsigned long lastDebounceTime = 0;

  bool currentReading = digitalRead(KEY_PIN);

  // If the raw reading changed, reset the debounce timer
  if (currentReading != lastReading) {
    lastDebounceTime = millis();
  }

  // If the reading has been stable for longer than KEY_DEBOUNCE_MS
  if ((millis() - lastDebounceTime) > KEY_DEBOUNCE_MS) {
    if (currentReading != stableState) {
      stableState = currentReading;

      // Check if this is a valid press
      bool pressed = KEY_ACTIVE_LOW ? (stableState == LOW)
                                    : (stableState == HIGH);
      if (pressed) {
        Serial.println("[Button] Voice broadcast triggered");
        if (!isnan(g_temp) && !isnan(g_hum)) {
          sendVoiceReport(g_temp, g_hum);
        } else {
          Serial.println("[Button] No valid data, cannot broadcast");
        }
      }
    }
  }

  lastReading = currentReading;
}

/*
 * Build the English broadcast text and send it to the WT3000.
 *
 * Notes:
 *   - "degrees Celsius" may be synthesized word-by-word on A-version firmware.
 *     On D-version firmware it will be synthesized as natural English speech.
 *   - Using the word "percent" instead of "%" avoids TTS misinterpretation.
 *   - Keep the string short (< 100 chars) to stay within frame buffer limits.
 */
void sendVoiceReport(float temp, float hum) {
  char buf[96];
  snprintf(buf, sizeof(buf),
           "Temperature %.1f degrees Celsius, humidity %.1f percent",
           temp, hum);
  Serial.printf("[Voice] Broadcasting: %s\n", buf);
  sendTTS(buf);
}

/*
 * Send a generic WT3000 command.
 * cmd: 16-bit command code (0x03E8 for TTS synthesis)
 * data: pointer to data payload
 * len: data length in bytes
 */
void sendWT3000Command(uint16_t cmd, const uint8_t *data, uint8_t len) {
  uint8_t frame[256];
  uint8_t idx = 0;

  frame[idx++] = 0x7E;                          // Start code
  uint16_t frameLen = 9 + len;                  // Frame length = 9 + payload
  frame[idx++] = (frameLen >> 8) & 0xFF;        // Frame length high byte
  frame[idx++] = frameLen & 0xFF;               // Frame length low byte

  static uint8_t seq = 0;
  seq++;
  frame[idx++] = seq;                           // Sequence number
  frame[idx++] = 0x00;                          // Response flag
  frame[idx++] = 0x03;                          // Data source: MCU

  frame[idx++] = (cmd >> 8) & 0xFF;             // Command high byte
  frame[idx++] = cmd & 0xFF;                    // Command low byte
  frame[idx++] = len;                           // Data length

  for (uint8_t i = 0; i < len; i++) {
    frame[idx++] = data[i];
  }

  // Checksum: sum from frame length high byte through end of data
  uint8_t checksum = 0;
  for (uint8_t i = 1; i < idx; i++) {
    checksum += frame[i];
  }
  frame[idx++] = checksum;                      // Checksum
  frame[idx++] = 0xEF;                          // End code

  Serial1.write(frame, idx);
}

/*
 * Send a TTS synthesis command (command code 0x03E8).
 *
 * Since the broadcast text is pure ASCII, we can pass it directly.
 * ASCII bytes are identical in UTF-8 and GB2312 (range 0x00-0x7F),
 * so no encoding conversion is required.
 */
void sendTTS(const char *text) {
  sendWT3000Command(0x03E8, (const uint8_t *)text, (uint8_t)strlen(text));
}