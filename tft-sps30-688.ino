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

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#define TFT_BLK 15

// --- DEFINICIONES DE COLOR CORREGIDAS PARA ST7789 (BGR/RGB swap) ---
// Estos valores remapean los colores para que se vean correctamente en tu pantalla.
// Por ejemplo: El valor original de ST77XX_GREEN se usa ahora para el VERDE Corregido.

#define ST77XX_BLACK   0x0000
#define ST77XX_WHITE   0xFFFF

// Colores Base Corregidos (Intercambiando Rojo y Verde)
// Lo que era Rojo (F800) debe ser Verde (07E0) y viceversa.
#define CORR_VERDE      0xF81F     // El valor que DEBERÍA ser ROJO, ahora es VERDE
#define CORR_ROJO       0x07FF        // El valor que DEBERÍA ser VERDE, ahora es ROJO
#define CORR_AMARILLO   0x001F        // Naranja
#define CORR_CYAN       0xFD20       // Cyan (Debe ser Azul+Verde)

// Colores IBOCA (Clasificación de Calidad del Aire)
#define COLOR_BAJO      CORR_VERDE       // Verde (Bajo)
#define COLOR_MODERADO  CORR_AMARILLO    // Amarillo (Moderado)
#define COLOR_REGULAR   0xFF00    // Naranja (Ajuste ligero)
#define COLOR_ALTO      CORR_ROJO        // Rojo (Alto)
#define COLOR_PELIGROSO 0x07E0          // Púrpura/Morado (Peligroso)


// Ajusta esto a la presión real de tu ciudad para tener altitud exacta (1013.25 es nivel del mar)
#define SEALEVELPRESSURE_HPA (1013.25)

SensirionI2cSps30 sps30;
Adafruit_BME680 bme;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

static int16_t error;

// Estructura para almacenar el resultado de la evaluación
struct AirQualityResult {
  String estado;
  uint16_t color;
};

// Función de evaluación genérica basada en rangos (ICA/IBOCA)
AirQualityResult evaluarContaminante(float valor, bool isPM25) {
    AirQualityResult result;
    
    // RANGOS ICA/IBOCA (Valores en ug/m3)
    // PM2.5: (0-12) BAJO, (12.1-37) MODERADO, (37.1-62) REGULAR, (62.1-93) ALTO, (>93) PELIGROSO
    // PM10:  (0-54) BAJO, (55-154) MODERADO, (155-254) REGULAR, (255-354) ALTO, (>354) PELIGROSO

    if (isPM25) { // Rangos para PM2.5
        if (valor <= 12.0) { result.estado = "BAJO"; result.color = COLOR_BAJO; }
        else if (valor <= 35.0) { result.estado = "MODERADO"; result.color = COLOR_MODERADO; }
        else if (valor <= 55.0) { result.estado = "REGULAR"; result.color = COLOR_REGULAR; }
        else if (valor <= 151.0) { result.estado = "ALTO"; result.color = COLOR_ALTO; }
        else { result.estado = "PELIGROSO"; result.color = COLOR_PELIGROSO; } 
    } else { // Rangos para PM10
        if (valor <= 27.0) { result.estado = "BAJO"; result.color = COLOR_BAJO; }
        else if (valor <= 63.0) { result.estado = "MODERADO"; result.color = COLOR_MODERADO; }
        else if (valor <= 95.0) { result.estado = "REGULAR"; result.color = COLOR_REGULAR; }
        else if (valor <= 246.0) { result.estado = "ALTO"; result.color = COLOR_ALTO; }
        else { result.estado = "PELIGROSO"; result.color = COLOR_PELIGROSO; }
    }
    return result;
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
  tft.setTextColor(0xFF40); // Usamos CYAN corregido
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("MONITOR AMBIENTAL");
  tft.drawLine(0, 35, 320, 35, ST77XX_WHITE);

  // --- Zona BME688 (Clima Completo) ---
  tft.setTextColor(CORR_CYAN); // Usamos AMARILLO corregido
  tft.setTextSize(2);
  
  // Fila 1: Temp | Hum | Alt
  tft.setCursor(5, 45);    tft.print("Tmp:");
  tft.setCursor(110, 45);  tft.print("Hum:");
  tft.setCursor(215, 45);  tft.print("Alt:");
  
  // Fila 2: Pres | Gas
  tft.setCursor(5, 75);    tft.print("Pre:");
  tft.setCursor(160, 75);  tft.print("Gas(VOC):");
  
  tft.drawLine(0, 105, 320, 105, ST77XX_WHITE);

  // --- Zona SPS30 (Calidad Aire - PM2.5 y PM10) ---
  
  // Encabezados
  tft.setTextColor(CORR_ROJO ); // Usamos VERDE corregido
  tft.setTextSize(2);
  tft.setCursor(25, 115); tft.print("PM 2.5"); 
  tft.setCursor(195, 115); tft.print("PM 10"); 
  
  // Texto y marco para PM 2.5
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 185); tft.print("ESTADO PM 2.5:");
  tft.drawRect(5, 195, 150, 35, ST77XX_WHITE); // Marco para el estado de PM2.5
  
  // Texto y marco para PM 10
  tft.setCursor(165, 185); tft.print("ESTADO PM 10:");
  tft.drawRect(165, 195, 150, 35, ST77XX_WHITE); // Marco para el estado de PM10

  if (!bmeFound) { tft.setTextColor(CORR_ROJO); tft.setCursor(80, 50); tft.print("ERR"); }
  if (!sps30Ready) { tft.setTextColor(CORR_ROJO); tft.setCursor(10, 150); tft.print("ERROR SPS30"); }
}

void loop() {
  // BME688
  if (bme.performReading()) {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2);

    // Fila 1
    tft.setCursor(55, 45);   tft.print(bme.temperature, 0); tft.print("C");
    tft.setCursor(160, 45);  tft.print(bme.humidity, 0); tft.print("%");
    tft.fillRect(265, 45, 50, 15, ST77XX_BLACK); 
    tft.setCursor(265, 45);  tft.print(bme.readAltitude(SEALEVELPRESSURE_HPA), 0); tft.print("m");

    // Fila 2
    tft.setCursor(55, 75);   tft.print(bme.pressure / 100.0, 0); tft.print("hPa");
    tft.fillRect(270, 75, 40, 15, ST77XX_BLACK); 
    tft.setCursor(270, 75);  tft.print(bme.gas_resistance / 1000.0, 0); tft.print("K");
  }

  // SPS30
  uint16_t dataReadyFlag = 0;
  if (sps30.readDataReadyFlag(dataReadyFlag) == 0 && dataReadyFlag) {
    uint16_t mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typicalPartSize;
    if (sps30.readMeasurementValuesUint16(mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typicalPartSize) == 0) {
      
      float pm25Val = mc2p5;
      float pm10Val = mc10p0;
      
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      
      // --- VALOR PM 2.5 ---
      tft.setTextSize(4);
      tft.fillRect(10, 140, 130, 30, ST77XX_BLACK); 
      tft.setCursor(10, 140); tft.print(pm25Val, 1);
      tft.setTextSize(2);
      tft.setCursor(110, 150); tft.print("ug");

      // --- VALOR PM 10 ---
      tft.setTextSize(4);
      tft.fillRect(170, 140, 130, 30, ST77XX_BLACK); 
      tft.setCursor(170, 140); tft.print(pm10Val, 1);
      tft.setTextSize(2);
      tft.setCursor(270, 150); tft.print("ug");

      // --- BARRA Y ESTADO PM 2.5 (Izquierda) ---
      AirQualityResult resultado25 = evaluarContaminante(pm25Val, true);
      tft.fillRect(6, 196, 148, 33, resultado25.color); 
      tft.setTextColor(ST77XX_BLACK);
      // Se usa blanco para texto en fondo oscuro (Alto o Peligroso)
      if (resultado25.color == COLOR_ALTO || resultado25.color == COLOR_PELIGROSO) tft.setTextColor(ST77XX_WHITE);
      tft.setTextSize(2);
      tft.setCursor(10, 203);
      tft.print(resultado25.estado);

      // --- BARRA Y ESTADO PM 10 (Derecha) ---
      AirQualityResult resultado10 = evaluarContaminante(pm10Val, false);
      tft.fillRect(166, 196, 148, 33, resultado10.color); 
      tft.setTextColor(ST77XX_BLACK);
      // Se usa blanco para texto en fondo oscuro (Alto o Peligroso)
      if (resultado10.color == COLOR_ALTO || resultado10.color == COLOR_PELIGROSO) tft.setTextColor(ST77XX_WHITE);
      tft.setTextSize(2);
      tft.setCursor(170, 203);
      tft.print(resultado10.estado);
      
      // Tipo de Partícula
      tft.setTextColor(CORR_CYAN, ST77XX_BLACK); // Usamos CYAN corregido
      tft.setTextSize(1);
      tft.setCursor(10, 235);
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