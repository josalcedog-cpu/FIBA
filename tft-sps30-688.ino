#include <Arduino.h>
#include <Wire.h>
#include <SPI.h> 

// --- LIBRERÍAS REQUERIDAS ---
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SensirionI2cSps30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

/* ==========================================================================
   DIAGRAMA DE CONEXIÓN GENERAL (SISTEMA COMPLETO)
   ==========================================================================
   1. SENSORES (I2C) -> Pines 21 y 22
   -----------------------------------------------------
   [SPS30] VDD (Rojo)   -> ESP32 VIN (5V)
   [SPS30] GND (Negro)  -> ESP32 GND
   [SPS30] SEL (Blanco) -> ESP32 GND
   [SPS30] SDA (Verde)  -> ESP32 GPIO 21
   [SPS30] SCL (Amarillo)-> ESP32 GPIO 22
   
   [BME688] VIN         -> ESP32 3.3V
   [BME688] GND         -> ESP32 GND
   [BME688] SDA         -> ESP32 GPIO 21
   [BME688] SCL         -> ESP32 GPIO 22
   [BME688] SDO         -> GND (Dirección 0x76)
   [BME688] CS          -> 3.3V (Modo I2C)

   2. PANTALLA (SPI) -> Pines 18, 23, 5, 2, 4
   -----------------------------------------------------
   [ST7789] VCC         -> ESP32 3.3V
   [ST7789] GND         -> ESP32 GND
   [ST7789] SDA (MOSI)  -> ESP32 GPIO 23
   [ST7789] SCL (SCK)   -> ESP32 GPIO 18
   [ST7789] CS          -> ESP32 GPIO 5
   [ST7789] DC          -> ESP32 GPIO 2
   [ST7789] RES         -> ESP32 GPIO 4
   ========================================================================== */

#define SDA_PIN 21
#define SCL_PIN 22

#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BLK   15

// Definimos colores personalizados
#define COLOR_ORANGE 0xFD20
#define COLOR_PURPLE 0x780F

// Ajusta esto a la presión real de tu ciudad para tener altitud exacta (1013.25 es nivel del mar)
#define SEALEVELPRESSURE_HPA (1013.25)

SensirionI2cSps30 sps30;
Adafruit_BME680 bme; 
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

static int16_t error;

// Función para determinar el estado del aire basado en PM2.5
uint16_t evaluarCalidadAire(float pm25, String &textoEstado) {
    if (pm25 <= 12.0) {
        textoEstado = "EXCELENTE";
        return ST77XX_GREEN;
    } else if (pm25 <= 35.4) {
        textoEstado = "MODERADO";
        return ST77XX_YELLOW;
    } else if (pm25 <= 55.4) {
        textoEstado = "SENSIBLE"; 
        return COLOR_ORANGE;
    } else if (pm25 <= 150.4) {
        textoEstado = "DAÑINO";   
        return ST77XX_RED;
    } else {
        textoEstado = "PELIGROSO"; 
        return COLOR_PURPLE;
    }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  // --- 1. INICIO PANTALLA ---
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH); delay(50);
  digitalWrite(TFT_RST, LOW); delay(100);
  digitalWrite(TFT_RST, HIGH); delay(100);
  
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH); 

  tft.init(240, 320);           
  tft.setRotation(1);           
  tft.fillScreen(ST77XX_BLACK);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 140);
  tft.print("CALIBRANDO...");

  // --- 2. INICIO SENSORES ---
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); 

  // SPS30
  sps30.begin(Wire, SPS30_I2C_ADDR_69);
  sps30.stopMeasurement();
  delay(100);
  
  int8_t serialNumber[32] = {0};
  bool sps30Ready = (sps30.readSerialNumber(serialNumber, 32) == 0);
  
  if (sps30Ready) {
      sps30.writeAutoCleaningInterval(345600UL); // 4 días
      sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
  }

  // BME688
  bool bmeFound = false;
  if (bme.begin(0x76, &Wire)) bmeFound = true;
  else if (bme.begin(0x77, &Wire)) bmeFound = true;

  if (bmeFound) {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); 
  }

  // --- 3. DIBUJAR INTERFAZ ESTÁTICA ---
  tft.fillScreen(ST77XX_BLACK);
  
  // Título
  tft.setTextColor(ST77XX_CYAN); 
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("MONITOR AMBIENTAL");
  tft.drawLine(0, 35, 320, 35, ST77XX_WHITE);

  // --- Zona BME688 (Clima Completo) ---
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  
  // Fila 1: Temp | Hum | Alt
  tft.setCursor(5, 45);    tft.print("Tmp:");
  tft.setCursor(110, 45);  tft.print("Hum:");
  tft.setCursor(215, 45);  tft.print("Alt:");
  
  // Fila 2: Pres | Gas
  tft.setCursor(5, 75);    tft.print("Pre:");
  tft.setCursor(160, 75);  tft.print("Gas(VOC):");
  
  tft.drawLine(0, 105, 320, 105, ST77XX_WHITE);

  // --- Zona SPS30 (Calidad Aire) ---
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 115); tft.print("PM 2.5");  
  tft.setCursor(180, 115); tft.print("PM 10");  
  
  // Recuadro para estado
  tft.drawRect(5, 180, 310, 40, ST77XX_WHITE);

  if (!bmeFound) { tft.setTextColor(ST77XX_RED); tft.setCursor(80, 50); tft.print("ERR"); }
  if (!sps30Ready) { tft.setTextColor(ST77XX_RED); tft.setCursor(10, 150); tft.print("ERROR SPS30"); }
}

void loop() {
  // BME688
  if (bme.performReading()) {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
    tft.setTextSize(2);

    // Fila 1
    tft.setCursor(55, 45);   tft.print(bme.temperature, 0); tft.print("C");
    tft.setCursor(160, 45);  tft.print(bme.humidity, 0); tft.print("%");
    tft.setCursor(265, 45);  tft.print(bme.readAltitude(SEALEVELPRESSURE_HPA), 0); tft.print("m");

    // Fila 2
    tft.setCursor(55, 75);   tft.print(bme.pressure / 100.0, 0); tft.print("hPa");
    tft.setCursor(270, 75);  tft.print(bme.gas_resistance / 1000.0, 0); tft.print("K");
  }

  // SPS30
  uint16_t dataReadyFlag = 0;
  if (sps30.readDataReadyFlag(dataReadyFlag) == 0 && dataReadyFlag) {
    uint16_t mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typicalPartSize;
    if (sps30.readMeasurementValuesUint16(mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typicalPartSize) == 0) {
      
      float pm25Val = mc2p5 / 10.0;
      float pm10Val = mc10p0 / 10.0;
      
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      
      // PM 2.5
      tft.setTextSize(4); 
      tft.setCursor(10, 140); tft.print(pm25Val, 1);
      tft.setTextSize(2);
      tft.setCursor(110, 150); tft.print("ug");

      // PM 10
      tft.setTextSize(4);
      tft.setCursor(170, 140); tft.print(pm10Val, 1);
      tft.setTextSize(2);
      tft.setCursor(270, 150); tft.print("ug");

      // Estado Aire
      String estado = "";
      uint16_t colorEstado = evaluarCalidadAire(pm25Val, estado);
      
      tft.fillRect(6, 181, 308, 38, colorEstado); 
      tft.setTextColor(ST77XX_BLACK); 
      if (colorEstado == ST77XX_RED || colorEstado == COLOR_PURPLE) tft.setTextColor(ST77XX_WHITE);
      
      tft.setTextSize(2);
      tft.setCursor(20, 192); 
      tft.print(estado);

      // Tipo de Partícula
      tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft.setTextSize(1);
      tft.setCursor(10, 230);
      tft.print("TAMANO TIPICO: "); 
      tft.print(typicalPartSize / 100.0, 2);
      tft.print(" um");
      
      // Serial Debug Completo
      Serial.print("T:"); Serial.print(bme.temperature);
      Serial.print(" H:"); Serial.print(bme.humidity);
      Serial.print(" P:"); Serial.print(bme.pressure/100.0);
      Serial.print(" G:"); Serial.print(bme.gas_resistance/1000.0);
      Serial.print(" | PM2.5: "); Serial.print(pm25Val);
      Serial.print(" PM10: "); Serial.println(pm10Val);
    }
  }
  delay(2000); 
}