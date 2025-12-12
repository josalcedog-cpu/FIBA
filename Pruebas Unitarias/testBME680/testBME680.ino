#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// --- REQUIRED LIBRARIES ---
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

/* ==========================================================================
   CONNECTIONS & PINOUT
   ==========================================================================
   BME688 (I2C): SDA(21), SCL(22) -> Address 0x76 or 0x77
   TFT (SPI):
     - SDA/MOSI: 23
     - SCL/SCLK: 18
     - CS: 5
     - DC: 2
     - RST: 15
   BUZZER: 4
   ========================================================================== */

#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 4

// TFT Pins
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  15

// --- COLORS ---
#define ST77XX_BLACK   0x0000
#define ST77XX_WHITE   0xFFFF
#define ST77XX_RED     0xF800
#define ST77XX_GREEN   0x07E0
#define ST77XX_BLUE    0x001F
#define ST77XX_CYAN    0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW  0xFFE0
#define ST77XX_ORANGE  0xFC00

// --- OBJECTS ---
Adafruit_BME680 bme;
// Use Hardware SPI (Faster & Standard for ESP32 on pins 23/18)
// Constructor: CS, DC, RST. Implicitly uses VSPI
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- CONFIGURATION ---
#define SEALEVELPRESSURE_HPA (1013.25)
// --- CALIBRACION ---
// El BME688 se calienta internamente por el sensor de gas (320 grados!).
// Esto causa lecturas de T mayores a la real. Ajusta este offset segun otro termometro.
const float TEMP_OFFSET = 11.0; // Ajustado para Bogotá (Lectura ~21C - Real ~10C)

const long UPDATE_INTERVAL = 2500; // ms
unsigned long lastCheck = 0;

// --- HELPER FUNCTION: ESTIMATE CO2 ---
// Uses Gas Resistance to approximate CO2 ppm (eCO2 proxy).
// NOTE: BME688 provides raw resistance. Without BSEC library (complex),
// we use a simple mapping: High Resistance = Clean Air (Low CO2).
// Range approx: 50kOhm+ (Clean) -> 5kOhm (Bad)
int estimateCO2(float gasResOhms) {
    if (gasResOhms >= 50000) return 400; // Baseline fresh air
    if (gasResOhms <= 5000) return 2000; // Very stale air
    
    // Linear interpolation
    // Slope m = (2000 - 400) / (5000 - 50000) = 1600 / -45000 = -0.0355
    // y = mx + b -> y - y1 = m(x - x1)
    return (int)(400 + (-0.0355 * (gasResOhms - 50000)));
}

void drawStaticUI() {
    tft.fillScreen(ST77XX_BLACK);
    
    // Header
    tft.fillRect(0, 0, 320, 30, 0x2124); // Dark Grey Background
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(60, 5);
    tft.print("PRUEBAS BME688");

    // Labels Area: BME688
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    
    // Row 1: Temp & Hum
    tft.setCursor(10, 60); tft.print("T:");
    tft.setCursor(160, 60); tft.print("H:");

    // Row 2: Press & Alt
    tft.setCursor(10, 110); tft.print("P:");
    tft.setCursor(160, 110); tft.print("Alt:");

    // Row 3: Gas & CO2
    tft.setCursor(10, 160); tft.print("Gas:");
    tft.setCursor(160, 160); tft.print("CO2:");

    // Divider
    tft.drawFastHLine(0, 200, 320, ST77XX_WHITE);
}

void setup() {
    Serial.begin(115200);
    
    // 1. BUZZER
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // 2. TFT DISPLAY
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH); delay(50);
    digitalWrite(TFT_RST, LOW); delay(100);
    digitalWrite(TFT_RST, HIGH); delay(100);

    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(30, 100);
    tft.print("INICIANDO SENSOR...");

    // 3. I2C Sensors
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000); // Standard speed

    // BME688 Init
    bool bmeFound = false;
    if (bme.begin(0x76, &Wire)) {
        bmeFound = true;
        Serial.println("BME680 Found at 0x76");
    } else if (bme.begin(0x77, &Wire)) {
        bmeFound = true;
        Serial.println("BME680 Found at 0x77");
    }

    if (!bmeFound) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_RED);
        tft.setCursor(20, 100);
        tft.print("ERROR: BME NO DETECTADO");
        Serial.println("BME680 Not Found");
        while(1); // Halt
    }

    // BME Config
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms

    // Draw UI
    drawStaticUI();
}

void loop() {
    if (millis() - lastCheck > UPDATE_INTERVAL) {
        lastCheck = millis();

        // --- READ BME688 ---
        if (bme.performReading()) {
            float temp = bme.temperature - TEMP_OFFSET; // Corregir temperatura
            float hum = bme.humidity;
            float pressHpa = bme.pressure / 100.0;
            float gasK = bme.gas_resistance / 1000.0;
            float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
            int estCO2 = estimateCO2(bme.gas_resistance);

            // Update UI
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); // Black bg to overwrite
            tft.setTextSize(2);

            // Tmp / Hum
            tft.setCursor(45, 60); tft.print(temp, 1); tft.print("C  ");
            tft.setCursor(200, 60); tft.print(hum, 0); tft.print("%  ");

            // Press / Alt
            tft.setCursor(45, 110); tft.print(pressHpa, 0); tft.print("hPa ");
            tft.setCursor(215, 110); tft.print(altitude, 0); tft.print("m ");

            // Gas / CO2
            tft.setCursor(65, 160); tft.print(gasK, 1); tft.print("K  ");
            tft.setCursor(215, 160); tft.print(estCO2); tft.print("ppm ");
        }
    }
}
