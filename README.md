# 🍃 Monitor de Calidad del Aire (FIBA) 🍃

> **⚠️ NOTA IMPORTANTE:** La carpeta llamada `fiba` contiene la versión final y funcional del firmware de este dispositivo.

Este proyecto implementa un sistema de monitoreo ambiental basado en un **ESP32**, capaz de medir calidad del aire (PM2.5, PM10), temperatura, humedad y resistencia de gas (VOCs), con estimación de CO2. Los datos se visualizan en una pantalla TFT y se envían a **Firebase Realtime Database** para registro histórico.

## 📋 Requerimientos del Sistema

### 🛠️ Hardware
*   **Microcontrolador:** ESP32 (Dev Kit V1 o compatible). 🧠
*   **Sensor de Partículas:** Sensirion **SPS30**. 🌫️
*   **Sensor Ambiental:** Bosch **BME688** (o BME680). 🌡️
*   **Pantalla:** TFT LCD SPI (Driver **ST7789**, resolución 240x320). 🖥️
*   **Actuador:** Zumbador (Buzzer) activo o pasivo. 🔊
*   Protoboard y cables de conexión. 🔌

### 💻 Software
*   **IDE:** Arduino IDE. ♾️
*   **Librerías (Instalar desde el Gestor de Librerías):** 📚
    *   `Adafruit GFX Library`
    *   `Adafruit ST7789 Library`
    *   `Sensirion I2C SPS30`
    *   `Adafruit BME680 Library`
    *   `Adafruit Unified Sensor`
    *   `Firebase Arduino Client Library for ESP8266 and ESP32` (por Mobizt) 🔥
*   **Python (Para procesamiento de datos - Opcional):** 🐍
    *   El archivo de clave JSON (`serviceAccountKey.json`) para la autenticación de Firebase Admin SDK debe colocarse dentro de la carpeta **`dataFIBA`**.

## 🚀 Guía de Inicio Rápido

### 1. 🔌 Conexiones (Wiring)

Realiza las siguientes conexiones entre el ESP32 y los periféricos:

| Periférico | Pin del Periférico | Pin ESP32 | Protocolo |
| :--- | :--- | :--- | :--- |
| **SPS30** | SDA | GPIO 21 | I2C |
| | SCL | GPIO 22 | I2C |
| | VCC | 5V | Alimentación |
| | GND | GND | Tierra |
| **BME688** | SDA | GPIO 21 | I2C |
| | SCL | GPIO 22 | I2C |
| | VCC | 3.3V | Alimentación |
| | GND | GND | Tierra |
| **Pantalla TFT** | SDA (MOSI) | GPIO 23 | SPI |
| | SCL (SCLK) | GPIO 18 | SPI |
| | CS | GPIO 5 | SPI CS |
| | DC | GPIO 2 | Data/Command |
| | RST | GPIO 15 | Reset |
| | VCC | 3.3V | Alimentación |
| | GND | GND | Tierra |
| **Buzzer** | PIN (+) | GPIO 4 | Digital Out |
| | PIN (-) | GND | Tierra |

> **📝 Nota:** El SPS30 y el BME688 comparten el bus I2C (pines 21 y 22).

### 2. ⚙️ Configuración del Código

Antes de cargar el código, asegúrate de actualizar las credenciales en el archivo `fiba.ino`:

1.  **📶 WiFi:**
    Busca las líneas:
    ```cpp
    #define WIFI_SSID "TU_RED_WIFI"
    #define WIFI_PASSWORD "TU_CONTRASEÑA"
    ```
    Y coloca tus propios datos.

2.  **🔥 Firebase:**
    Configura tu proyecto en Firebase Realtime Database y obtén la **API Key** y la **URL de la base de datos**.
    ```cpp
    #define API_KEY "TU_API_KEY_DE_FIREBASE"
    #define DATABASE_URL "https://tu-proyecto.firebaseio.com"
    ```

### 3. ▶️ Cargar el Programa

1.  Conecta el ESP32 a tu computadora. 💻
2.  Selecciona la placa correcta en **Herramientas > Placa** (ej. *DOIT ESP32 DEVKIT V1*).
3.  Selecciona el puerto COM correspondiente.
4.  Compila y sube el código. 📤

### ✅ Funcionamiento

*   El sistema intentará conectarse a WiFi y Firebase al iniciar. 📡
*   En la pantalla verás los valores en tiempo real de PM2.5, PM10, Temperatura y Gas. 📊
*   Si la calidad del aire es **Mala** o **Peligrosa**, la pantalla mostrará alertas en rojo y sonará el buzzer. 🚨
*   Los datos se envían a la nube (Firebase) cada 60 segundos automáticamente. ☁️
