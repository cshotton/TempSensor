#ifndef SENSORS_H

#include <Wire.h>

#ifdef SENSOR_AM2315
#include <Adafruit_AM2315.h>
#endif

#ifdef SENSOR_DHT11
#include "DHT.h"
#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11
#endif

#include "math.h"

#define SENSOR_CHECK_INTERVAL 10000

float temperature=-99.99, humidity=0.0;

#ifdef SENSOR_AM2315
Adafruit_AM2315 am2315;
#endif

#ifdef SENSOR_DHT11
DHT dht(DHTPIN, DHTTYPE);
#endif

unsigned long _lastSensorCheck = 0;

void sensorSetup () {
  Serial.println ("Configuring sensors...");
#ifdef SENSOR_AM2315
  if (! am2315.begin()) {
     Serial.println("\nERR\nERR: Sensor not found, check wiring & pullups!\nERR\n");
  }
#endif

#ifdef SENSOR_DHT11
  dht.begin();
#endif

  delay (200);

  //let the AM2315 settle down and start returning data before moving on
  int retries = 0;
  bool ready = false;
    
  do {
#ifdef SENSOR_AM2315
    ready = am2315.readTemperatureAndHumidity(&temperature, &humidity) || retries>19;
#endif
#ifdef SENSOR_DHT11
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    ready = retries > 19 || temperature != -99.99;
#endif
    Serial.print ("Waiting on sensors..."); Serial.println (temperature);
    retries++;
    delay (100);
  } while (!ready);
  
  if (retries < 20) {
    Serial.println ("Sensors ready.");
  }
  else {
    Serial.println ("Sensors not ready. Retries exceeded.");
  }
}

float getTemperatureC () {
  return temperature;
}

float getTemperatureF () {
  return temperature * 9 / 5 + 32;
}

int getHumidity () {
  return humidity;
}


void sensorLoop () {
  if (_lastSensorCheck < millis()) {
    _lastSensorCheck = millis() + SENSOR_CHECK_INTERVAL;
    
#ifdef SENSOR_AM2315
    if (! am2315.readTemperatureAndHumidity(&temperature, &humidity)) {
      Serial.println("Failed to read data from AM2315");
    }
#endif

#ifdef SENSOR_DHT11
    humidity = dht.readHumidity();
    // Read temperature as C
    temperature = dht.readTemperature();
#endif
  }
}

#define SENSORS_H
#endif
