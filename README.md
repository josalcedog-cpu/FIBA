# FIBA
Repositorio donde se almacenaran  los archivos, documentación  y respectiva planeación del proyecto de sensor de calidad de aire "FIBA""

Para el SPS30: 
El archivo .INO para Arduino IDE, siendo la versión inicial y el ejemplo de los paquetes instalados del repositorio:<br>

<a href= "https://github.com/Sensirion/arduino-i2c-sps30">REPO SENSIRION</a>(MIRAR ESTE UNICAMENTE PARA RELIZAR CODIGO O AVANCES)<br>
<a href= "https://github.com/Sensirion/arduino-avr-legacy-i2c-sps30">REPO SENSIRION-AUX</a>(OBSOLETO, SOLO INFO, SOLO PARA ESTUDIO)<br>

Para poder utilizar el SPS30 en Arduino IDE es necesario descargar la libreria (Sensirion I2C SPS30)
Datasheet del SENSIRIÓN SPS30: <a href="https://sensirion.com/media/documents/8600FF88/64A3B8D6/Sensirion_PM_Sensors_Datasheet_SPS30.pdf">DATASHEET SENSIRION SPS30</a> <br>
Resumen breve del funcionamiento del sensor: <a href= "https://www.instructables.com/How-To-Sensirion-SPS30"> HowTo SENSIRION SPS30</a>


<h3>Sobre los pones a la pantalla BME688: <h3>

  VIN -> 5V<br> 
  3Vo -> NO CONECTAR A NADA<br>
  GND -> GND<br>
  SCK -> GPIO22<br>
  SDO -> NO CONECTAR A NADA<br>
  SDI -> GPIO 21<br>
  CS -> NO CONECTAR A NADA<br> 
<h3>Sobre los pones a la pantalla SPS30: <h3>
  
![Imagen de WhatsApp 2025-12-04 a las 20 47 47_5759e3aa](https://github.com/user-attachments/assets/a03591c8-4843-4a4e-b5e4-83c7f3a5f88e)

<h3>Sobre los pones al Buzzer: <h3>
  GND -> GND<br>
  ANODO -> GPIO4<br>
<h3>Sobre los pones a la pantalla TfT: <h3>

  TFT | ESP32<br>
  CS -> GPIO5<br>
  DC -> GPIO2<br>
  RST -> GPIO15<br>
  SDA -> GPIO23<br>
  SCK -> GPIO18<br>
  VCC -> 3.3V<br>
  GND -> GND
