# TempSensor
Temperature and Humidity sensor for ESP8266 for DHT11 and AM2315, with neopixel status/night light
![Sensor Image](/images/sensor.png)

## Parts
 * ESP8266
 * DHT-11 or AM2315 temperature/humidity sensor
 * Optional WS2812B neopixel LED
 * 3D printed case
   * DHT-11 - [TinkerCad](https://www.tinkercad.com/things/38Fy2z1DDXx)
   * AM2315 or other - [TinkerCad](https://www.tinkercad.com/things/fIHLzRDuMFG/)
 * USB Power Adapter (fits case) - [Amazon](https://www.amazon.com/gp/product/B07XMKHW8R/)
 * USB A to micro adapter (fits case) - [Amazon](https://www.amazon.com/gp/product/B00TAM0MZW/)
 
 ## Build and Install
 The comments in the source code give wiring instructions. I chose to wire-wrap the components since there
 are really no external forces or vibration on the sensor once it is plugged in.
 
 The DHT-11 case is designed to allow the sensor to slide (tight fit) into the bottom of the case. Solder or
 wire-wrap leads to the sensor before inserting into the case, making sure they are long enough to reach the
 ESP8266 pins before assembly. Connect the components to the ESP8266.
 
 I chose to cut a small square of anti-static foam and insert it into the bottom of the case. It provides some
 insulation between the temperature sensor and the ESP8266, and also provides some pressure to keep the
 two halves of the case tightly together.
 
 I hot-glued the neopixel onto the back side of the ESP8266 making that the "front" of the device.
 
 Slide the ESP8266 into the case with the proper orientation, plug the USB adapter into the ESP8266,
 plug the adapter into the power adapter, then slide the box for the adapter from back to front, sliding
 the tabs on the longer portion of the case into the box portion, locking the two together.
 
 The finished result can be seen above.