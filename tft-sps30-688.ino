#include <Arduino.h>
#include <Wire.h>
#include <SPI.h> 

// --- LIBRERÍAS ---
// Instala estas desde el Gestor de Librerías de Arduino:
// 1. "Adafruit ST7789" (y "Adafruit GFX")
// 2. "Sensirion I2C SPS30"
// 3. "Adafruit BME680 Library"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SensirionI2cSps30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

/* ==========================================
   CONFIGURACIÓN DE PINES (Revisar con cuidado)
   ========================================== */

// --- PINES I2C (SPS30 y BME688) ---
#define SDA_PIN 21
#define SCL_PIN 22

// --- PINES SPI (Pantalla ST7789) ---
// ¡OJO! Estos son los errores más comunes de "Pantalla Blanca":
// 1. Pantalla SDA (o MOSI) -> ESP32 GPIO 23
// 2. Pantalla SCL (o SCK)  -> ESP32 GPIO 18
// 3. Pantalla RES (Reset)  -> ESP32 GPIO 4
// 4. Pantalla DC           -> ESP32 GPIO 2
// 5. Pantalla CS           -> ESP32 GPIO 5 (Si tu pantalla no tiene pin CS, ponlo a -1)

#define TFT_MOSI  23 // Dato
#define TFT_SCLK  18 // Reloj
#define TFT_CS    5  // Chip Select
#define TFT_DC    2  // Data/Command
#define TFT_RST   4  // Reset


// Configuración de Altitud
#define SEALEVELPRESSURE_HPA (1013.25)

// --- OBJETOS ---
SensirionI2cSps30 sps30;
Adafruit_BME680 bme; 

// Inicialización explícita con pines SPI para asegurar que usa los correctos
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Variables auxiliares
static char errorMessage[64];
static int16_t error;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  Serial.println(F("\n--- DIAGNÓSTICO DE INICIO ---"));

  // ---------------------------------------------------------
  // 1. FORZAR REINICIO DE PANTALLA (Solución Pantalla Blanca)
  // ---------------------------------------------------------
  // A veces la librería no resetea bien al inicio, lo hacemos manual:
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(50);
  digitalWrite(TFT_RST, LOW);  // Apagar pantalla (Reset)
  delay(100);
  digitalWrite(TFT_RST, HIGH); // Encender pantalla
  delay(100);


  // ---------------------------------------------------------
  // 2. INICIALIZAR PANTALLA
  // ---------------------------------------------------------
  // Si tu pantalla sigue blanca, prueba cambiar SPI_MODE2 a SPI_MODE3 o quitarlo
  tft.init(240, 320);           
  tft.setRotation(1);           
  tft.fillScreen(ST77XX_BLACK); // Si esto funciona, la pantalla se pondrá NEGRA (no blanca)
  
  // Prueba visual inmediata
  tft.fillScreen(ST77XX_RED);   // Pantalla ROJA por 1 segundo
  delay(500);
  tft.fillScreen(ST77XX_GREEN); // Pantalla VERDE por 1 segundo
  delay(500);
  tft.fillScreen(ST77XX_BLACK); // Pantalla NEGRA (Listo)

  // Mensaje de carga inicial
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 140);
  tft.print("INICIANDO...");

  // ---------------------------------------------------------
  // 3. INICIALIZAR SENSORES (I2C)
  // ---------------------------------------------------------
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); 

  // --- SPS30 ---
  sps30.begin(Wire, SPS30_I2C_ADDR_69);
  sps30.stopMeasurement(); 
  delay(100);
  
  bool sps30Ready = false;
  int8_t serialNumber[32] = {0};
  error = sps30.readSerialNumber(serialNumber, 32);
  if (error == 0) {
      Serial.println(F("SPS30: OK"));
      sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
      sps30Ready = true;
  } else {
      Serial.println(F("SPS30: ERROR DE CONEXIÓN"));
  }

  // --- BME688 ---
  bool bmeFound = false;
  if (bme.begin(0x76, &Wire)) bmeFound = true;
  else if (bme.begin(0x77, &Wire)) bmeFound = true;

  if (bmeFound) {
    Serial.println(F("BME688: OK"));
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); 
  } else {
    Serial.println(F("BME688: ERROR DE CONEXIÓN"));
  }

  // ---------------------------------------------------------
  // 4. DIBUJAR INTERFAZ
  // ---------------------------------------------------------
  tft.fillScreen(ST77XX_BLACK); 

  if (!sps30Ready && !bmeFound) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(3);
    tft.setCursor(10, 100);
    tft.print("ERROR TOTAL");
    tft.setTextSize(2);
    tft.setCursor(10, 140);
    tft.print("Revise energia/cables");
    while(1); 
  }

  // Cabecera
  tft.setTextColor(ST77XX_CYAN); 
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("CALIDAD DEL AIRE");
  tft.drawLine(0, 35, 320, 35, ST77XX_WHITE);

  // Etiquetas BME688 (Amarillo)
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 50);  tft.print("Temp:");
  tft.setCursor(170, 50); tft.print("Hum:");
  tft.setCursor(10, 90);  tft.print("Pres:");
  tft.setCursor(170, 90); tft.print("Alt:"); 
  tft.setCursor(10, 130); tft.print("Gas :");

  if (!bmeFound) {
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(80, 50); tft.print("ERR");
  }

  tft.drawLine(0, 165, 320, 165, ST77XX_WHITE);

  // Etiquetas SPS30 (Verde)
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 180);  tft.print("PM 2.5");
  tft.setCursor(170, 180); tft.print("PM 10");

  if (!sps30Ready) {
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(10, 210); tft.print("ERROR CONEXION");
  }
}

void loop() {
  // ------------------------------------------------
  // LECTURA Y ACTUALIZACIÓN BME688
  // ------------------------------------------------
  if (bme.performReading()) {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
    tft.setTextSize(2);

    // Temperatura
    tft.setCursor(80, 50); 
    tft.print(bme.temperature, 1); tft.print("C");

    // Humedad
    tft.setCursor(220, 50); 
    tft.print(bme.humidity, 0); tft.print("%");

    // Presión
    tft.setCursor(80, 90);
    tft.print(bme.pressure / 100.0, 0); tft.print("hPa");

    // Altitud
    tft.setCursor(220, 90);
    tft.print(bme.readAltitude(SEALEVELPRESSURE_HPA), 0); tft.print("m");

    // Gas
    tft.setCursor(80, 130);
    tft.print(bme.gas_resistance / 1000.0, 1); tft.print("K");
    
    // Debug Serial
    Serial.print("T:"); Serial.print(bme.temperature);
    Serial.print(" H:"); Serial.println(bme.humidity);
  }

  // ------------------------------------------------
  // LECTURA Y ACTUALIZACIÓN SPS30
  // ------------------------------------------------
  uint16_t dataReadyFlag = 0;
  if (sps30.readDataReadyFlag(dataReadyFlag) == 0 && dataReadyFlag) {
    uint16_t mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typicalPartSize;
    
    error = sps30.readMeasurementValuesUint16(mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typicalPartSize);
    
    if (error == 0) {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      
      // PM 2.5
      tft.setCursor(10, 205);
      tft.setTextSize(3); 
      tft.print(mc2p5 / 10.0, 1); 
      tft.setTextSize(2); tft.print("ug");

      // PM 10
      tft.setCursor(170, 205);
      tft.setTextSize(3);
      tft.print(mc10p0 / 10.0, 1); 
      tft.setTextSize(2); tft.print("ug");
      
      Serial.print("PM2.5: "); Serial.println(mc2p5 / 10.0);
    }
  }

  delay(2000); }