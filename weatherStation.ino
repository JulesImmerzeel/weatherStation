#include <Wire.h>         // adds I2C library 
#include <BH1750.h>       // adds BH1750 library file
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ArduinoHA.h>
#include "Secret.h"
#include <SoftwareSerial.h>


#define DHTPIN 14
#define DHTTYPE DHT11

//For the anemometer
#define RX        0    //Serial Receive pin
#define TX        2    //Serial Transmit pin
#define RTS_pin   1    //RS485 Direction control
#define RS485Transmit    HIGH
#define RS485Receive     LOW

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
SoftwareSerial RS485Serial(RX, TX);

float lux;
float hum;
float temp;
String ws;
float signalStrength;


//Initialize WiFi
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

//Define the sensors and/or devices
//The string must not contain any spaces!!! Otherwise the sensor will not show up in Home Assistant
HASensor sensorOwner("Owner");
HASensor sensorLong("Long");
HASensor sensorLat("Lat");
HASensor sensorTemperature("Temperature");
HASensor sensorHumidity("Humidity");
HASensor sensorSignalstrength("Signal_strength");
HASensor sensorLux ("BH1750");
HASensor sensorWind ("Anemometer"); 

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");

  // Unique ID must be set!
  byte mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  // Connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500); // waiting for the connection
  }
  Serial.println();
  Serial.println("Connected to the network");


  // Set sensor and/or device names
  // String conversion for incoming data from Secret.h
  String student_id = STUDENT_ID;
  String student_name = STUDENT_NAME;

  //Add student ID number with sensor name
  String stationNameStr = student_name + "'s Home Station";
  String ownerNameStr = student_id + " Station owner";
  String longNameStr = student_id + " Long";
  String latNameStr = student_id + " Lat";
  String temperatureNameStr = student_id + " Temperature";
  String humidityNameStr = student_id + " Humidity";
  String signalstrengthNameStr = student_id + " Signal Strength";
  String luxNameStr = student_id + " Lux";
  String windspeedNameStr = student_id + " Windspeed";
    
  //Convert the strings to const char*
  const char* stationName = stationNameStr.c_str();
  const char* ownerName = ownerNameStr.c_str();
  const char* longName = longNameStr.c_str();
  const char* latName = latNameStr.c_str();
  const char* temperatureName = temperatureNameStr.c_str();
  const char* humidityName = humidityNameStr.c_str();
  const char* signalstrengthName = signalstrengthNameStr.c_str();
  const char* luxName = luxNameStr.c_str();
  const char* windspeedName = windspeedNameStr.c_str();

  //Set main device name
  device.setName(stationName);
  device.setSoftwareVersion(SOFTWARE_VERSION);
  device.setManufacturer(STUDENT_NAME);
  device.setModel(MODEL_TYPE);

  sensorOwner.setName(ownerName);
  sensorOwner.setIcon("mdi:account");

  sensorLong.setName(longName);
  sensorLong.setIcon("mdi:crosshairs-gps");
  sensorLat.setName(latName);
  sensorLat.setIcon("mdi:crosshairs-gps");
  
  sensorTemperature.setName(temperatureName);
  sensorTemperature.setDeviceClass("temperature");
  sensorTemperature.setUnitOfMeasurement("°C");

  sensorHumidity.setName(humidityName);
  sensorHumidity.setDeviceClass("humidity");
  sensorHumidity.setUnitOfMeasurement("%");
  
  sensorSignalstrength.setName(signalstrengthName);
  sensorSignalstrength.setDeviceClass("signal_strength");
  sensorSignalstrength.setUnitOfMeasurement("dBm");

  sensorLux.setName(luxName);
  sensorLux.setDeviceClass("illuminance");
  sensorLux.setUnitOfMeasurement("lux");
  
  sensorWind.setName(windspeedName);
  sensorWind.setUnitOfMeasurement("m/s");
  
  mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
  
  while (!mqtt.isConnected()) {
      mqtt.loop();
      Serial.print(".");
      delay(500); // waiting for the connection
  }

  Serial.println();
  Serial.println("Connected to MQTT broker");
  
  sensorOwner.setValue(STUDENT_NAME);
  sensorLat.setValue(LAT, (uint8_t)15U);
  sensorLong.setValue(LONG, (uint8_t)15U);

  //------
  
  pinMode(13, OUTPUT); //signal for relais
  pinMode(15, OUTPUT); //signal for relais
  digitalWrite(15, LOW);
  Wire.begin();
  lightMeter.begin();
  dht.begin();
  RS485Serial.begin(9600);
}

// the loop function runs over and over again forever
void loop() {
  mqtt.loop();
  digitalWrite(RTS_pin, RS485Transmit);     // init Transmit
  byte Anemometer_request[] = {0x01, 0x03, 0x00, 0x16, 0x00, 0x01, 0x65, 0xCE}; // inquiry frame
  RS485Serial.write(Anemometer_request, sizeof(Anemometer_request));
  RS485Serial.flush();
  digitalWrite(RTS_pin, RS485Receive);      // Init Receive
  byte Anemometer_buf[8];
  RS485Serial.readBytes(Anemometer_buf, 8);
  ws = String(Anemometer_buf[4]);

  lux = lightMeter.readLightLevel();
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  signalStrength = WiFi.RSSI();

  if (isnan(hum)) {
    hum = 1;
  }
  
  if (isnan(temp)) {
    temp = 0;
  }

  if (isnan(lux)) {
    lux = 0;
  }
  

  sensorTemperature.setValue(temp);
  sensorHumidity.setValue(hum);
  sensorSignalstrength.setValue(signalStrength);
  sensorLux.setValue(lux);
  sensorWind.setValue(Anemometer_buf[4]);
  
  Serial.println(lux);
  Serial.println(temp);
  Serial.println(hum);
  Serial.println(ws);
  Serial.println(" ");

  if (lux < 10)
  {
    digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
  }
  else
  {
    digitalWrite(13, HIGH);
  }
  delay(2000);                       // wait for two second
}
