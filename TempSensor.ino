/*
 *  Temperature/Humidity Sensor for ESP8266 with web services/admin and webhook push
 *  and color LED for status and night light.
 *  
 *  For a NodeMCU device, wired to GPIO12 (D6) for a neopixel LED and GPIO4 (D2) for DHT11 signal pin
 *  (If using an AM2315 or similar, the I2C lines at D1/D2 (GPIO5/4) need to be used for the sensor and the LED should stay on GPIO12)
 */
 
#include <FS.h>                   //this needs to be first, or it all crashes and burns...


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// for updating a new (or out of date) board, set the following to "true", build and deploy. Then set back to false and repeat. This clears
// the SPIFFS file system and resets all the internal settings to the latest JSON format
#define RESET_EVERYTHING false

//pick one of the following #defines, depending on which temp/humidity sensor is connected
//DHT11 is assumed to be on GPIO4/D2. Change the pin number in sensors.h as necessary.
//AM2315 uses I2C

#define SENSOR_DHT11 1
//#define SENSOR_AM2315 1

#include "times.h"
#include "sensors.h"
#include "leds.h"

ESP8266WebServer server (80);

bool do_reset = false;
bool pushData = false;

//define your default values here, if there are different values in config.json, they are overwritten.
#define CONFIG_FILE_NAME "/config.json"

char station_name [40] = "inside";
char update_freq [6] = "10";
char hook_url[256] = "http://ha.local/pd/webhook";
char hook_guid [64] = "weather_pd";
char hook_report [32] = "temp_and_humidity";
char cold_temp [4] =  "68";
char hot_temp [4] =  "78";
char night_start [4] =  "19";
char night_end [4] =  "7";
char gmt_offset [4] =  "-4";
char light_color [8] = "FFFFFF";

char temp_color [8] = "00FF00"; //not a setting -- used to hold the current effect's color

//flag for saving data
bool shouldSaveConfig = false;

#define IDLE_DELAY 1000
unsigned long next_time = 0;
unsigned long sensor_time = 0;
unsigned long update_delay = 5;
int ledState = 0;  //0=undefined, 1=beat out the temperature, 2=turn on the night light, 3=show solid color
int tempState = 0; //1 hot, 0 ok, -1 cold

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void _SaveConfigFile (String js) {
  Serial.println ("_SaveConfigFile saving:\n" + js + "\n");
  
  File configFile = SPIFFS.open(CONFIG_FILE_NAME, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }
  else {
    int ct = configFile.print (js);
    configFile.close();
    Serial.printf ("Wrote %d chars to config file\n", ct);
  }
}

//writes config variables from memory to JSON file
void SaveConfig () {
    Serial.println("saving config");
    DynamicJsonDocument json (2048);

    json["station_name"] = station_name;
    json["update_freq"] = update_freq;
    json["hook_url"] = hook_url;
    json["hook_guid"] = hook_guid;
    json["hook_report"] = hook_report;
    json["gmt_offset"] = gmt_offset;
    json["cold_temp"] = cold_temp;
    json["hot_temp"] = hot_temp;
    json["night_start"] = night_start;
    json["night_end"] = night_end;
    json["light_color"] = light_color;

    File configFile = SPIFFS.open(CONFIG_FILE_NAME, "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
    Serial.println ("\nSaved /config.json");
    //end save  
}

//read the JSON file into a String
String GetConfigFile () {
  String json;
  
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(CONFIG_FILE_NAME)) {
      //file exists, reading and loading
      Serial.println("GetConfigFile: reading config file");
      File configFile = SPIFFS.open(CONFIG_FILE_NAME, "r");
      if (configFile) {
        Serial.println("opened config file");

        while (configFile.available()) {
          json += (char) configFile.read();
        }
        configFile.close();
        
        Serial.println ("*** Read config:\n" + json);
        return json;
      }
      else {
        Serial.println("failed to load json config");
      }
    }
    else {
      Serial.println ("No config file found.");
    }
  }
  else {
    Serial.println("failed to mount FS");
  }
}


//loads config variables from JSON file to memory
void LoadConfig () {
  
  String js = GetConfigFile ();

  if (js.length() == 0) {
    Serial.println ("LoadConfig Error: JSON data from config file is null/empty");
    return;
  }
  
  //parse the JSON into the memory vars
  DynamicJsonDocument json (2048);
  
  DeserializationError err = deserializeJson (json, js);
  if (err) {
    Serial.print ("LoadConfig JSON err "); Serial.println (err.c_str());
  }
  else {
    Serial.println("\nLoadConfig parsed json");

    strcpy(station_name, json["station_name"]);
    strcpy(update_freq, json["update_freq"]);
    update_delay = atol (update_freq);
    strcpy(hook_url, json["hook_url"]);
    strcpy(hook_guid, json["hook_guid"]);
    strcpy(hook_report, json["hook_report"]);
    strcpy(cold_temp, json["cold_temp"]);
    strcpy(hot_temp, json["hot_temp"]);
    strcpy(night_start, json["night_start"]);
    strcpy(night_end, json["night_end"]);
    strcpy(gmt_offset, json["gmt_offset"]);
    SetGMTOffset (gmt_offset);
    strcpy(light_color, json["light_color"]);
    ledState = 0;
    
    Serial.println ("Settings loaded from config file.");
  } 
}

//reports temp, humidity, and station name
void handleRoot () {
  char buff [10];
  int h = round (getHumidity ());
  float tf = getTemperatureF();
  float tc = getTemperatureC();

  dtostrf (tf, 3, 1, buff);
  String f = String (buff);

  dtostrf (tc, 3, 1, buff);
  String c = String (buff);

  String json = String ("{\"temperature\":\"") + f + "\",\"temperature_c\":\"" + c +"\",\"humidity\":\"" + h + "\",\"station\":\"" + station_name + "\"}";

  server.sendHeader ("Access-Control-Allow-Origin", "*");
  server.send (200, "application/json", json);
  Serial.println (station_name);
}

// sets the internal configuration field using the data supplied, JSON blob
// curl -XPOST -i -H "Content-type: application/json" -d '{"station_name":"inside","update_freq":"10","hook_url":"http://ha.local/pd/webhook","hook_guid":"weather_pd","hook_report":"temp_and_humidity","gmt_offset":"-4", "night_start":"19", "night_end" : "7", "cold_temp":"68", "hot_temp":"78","light_color":"FFFFFF"}' 'http://inside_temp.local/set'
void handleSet () {
  //get the json payload
  String body = server.arg("plain");
  Serial.println (" handled request for /set and got " + body);
  //save it to the config file
  _SaveConfigFile (body);
  //reload it from the config file to update settings
  LoadConfig();
  server.sendHeader ("Access-Control-Allow-Origin", "*");
  server.send (200, "application/json", "{\"status\": \"OK\"}");
}

// gets the internal configuration field as a JSON blob
void handleGet () {
  Serial.println ("Handled /get");
  server.sendHeader ("Access-Control-Allow-Origin", "*");
  server.send (200, "application/json", GetConfigFile ());
}

// causes the node to reset its persistent storage and reset to defaults
void handleReset () {
  server.sendHeader ("Access-Control-Allow-Origin", "*");
  server.send (200, "text/plain", "Reset initated.");
  Serial.print (station_name);
  Serial.println (" handled request for /reset");
  do_reset = true;
}

// forces the node to push its data to the webhook
void handlePush () {
  server.sendHeader ("Access-Control-Allow-Origin", "*");
  server.send (200, "text/plain", "Data pushed.");
  Serial.print (station_name);
  Serial.println (" handled request for /push");
  pushData = true;
}

//
//========================= SETUP ========================
//

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay (250);
  Serial.println(" ");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println ("\n\n*** Starting up....\n");
  
  //clean FS, for testing
  if (RESET_EVERYTHING) {
    Serial.println ("Resetting everything...");
    SPIFFS.format();
  }

  LoadConfig ();

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_station_name("station_name", "station name", station_name, 40);
  WiFiManagerParameter custom_update_freq("update_freq", "update freq", update_freq, 6);
  WiFiManagerParameter custom_hook_url("hook_url", "webhook url", hook_url, 256);
  WiFiManagerParameter custom_hook_guid("hook_guid", "webhook PD GUID", hook_guid, 64);
  WiFiManagerParameter custom_hook_report("hook_report", "report webhook name", hook_report, 32);
  WiFiManagerParameter custom_cold_temp("cold_temp", "cold temp", cold_temp, 4);
  WiFiManagerParameter custom_hot_temp("hot_temp", "hot temp", hot_temp, 4);
  WiFiManagerParameter custom_gmt_offset("gmt_offset", "config webhook name", gmt_offset, 4);
  WiFiManagerParameter custom_night_start("night_start", "night start", night_start, 4);
  WiFiManagerParameter custom_night_end("night_end", "night end", night_end, 4);
  WiFiManagerParameter custom_light_color("light_color", "config light color", light_color, 8);

  //WiFiManageraut
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_station_name);
  wifiManager.addParameter(&custom_update_freq);
  wifiManager.addParameter(&custom_hook_url);
  wifiManager.addParameter(&custom_hook_guid);
  wifiManager.addParameter(&custom_hook_report);
  wifiManager.addParameter(&custom_cold_temp);
  wifiManager.addParameter(&custom_hot_temp);
  wifiManager.addParameter(&custom_gmt_offset);
  wifiManager.addParameter(&custom_night_start);
  wifiManager.addParameter(&custom_night_end);
  wifiManager.addParameter(&custom_light_color);

  //reset settings - for testing
  if (RESET_EVERYTHING) {
    wifiManager.resetSettings();
  }

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(1000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(station_name, custom_station_name.getValue());
  strcpy(update_freq, custom_update_freq.getValue());
  update_delay = atol (update_freq);
  strcpy(hook_url, custom_hook_url.getValue());
  strcpy(hook_guid, custom_hook_guid.getValue());
  strcpy(hook_report, custom_hook_report.getValue());
  strcpy(cold_temp, custom_cold_temp.getValue());
  strcpy(hot_temp, custom_hot_temp.getValue());
  strcpy(gmt_offset, custom_gmt_offset.getValue());
  strcpy(night_start, custom_night_start.getValue());
  strcpy(night_end, custom_night_end.getValue());
  strcpy(light_color, custom_light_color.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    SaveConfig ();
  }

  Serial.println("\nlocal ip");
  Serial.println(WiFi.localIP());
  
  if (!MDNS.begin(String(station_name) + "_temp")) {
    Serial.println("Error setting up MDNS responder!");
  }
  else {
    Serial.print("mDNS responder started for " ); Serial.print (station_name); Serial.println ("_temp");

    server.on ("/", handleRoot);
    server.on ("/reset", handleReset);
    server.on ("/push", handlePush);
    server.on ("/get", handleGet);
    server.on ("/set", handleSet);
    
    server.begin();
    Serial.println("HTTP server started");

    // Add service to MDNS-SD
    MDNS.addService("temperature", "tcp", 80);
  }

  timeSetup ();
  SetGMTOffset (gmt_offset);
  
  sensorSetup ();
  led_setup ();
  
  digitalWrite(LED_BUILTIN, HIGH);
  LEDBeat (light_color,3000);
  Serial.println ("Ready!");
} //setup

//------------------------------------

void SendSensorData () {
  digitalWrite(LED_BUILTIN, LOW);
  char buff [10];
  int h = round (getHumidity ());
  float tf = getTemperatureF();
  float tc = getTemperatureC();

  /*******
   * This section of code is dedicated to pushing data to a remote aggregator on a periodic basis. I use a platform called "Rebar" that is my company's low-code/no-code
   * platform for app development (see www.concluent.com)
   * 
   * This could be replaced with code to send a payload to IFTTT or ThingsBoard or MQTT or whatever you want.
   */

  pushData = false;
  
  dtostrf (tf, 3, 1, buff);
  String f = String (buff);

  dtostrf (tc, 3, 1, buff);
  String c = String (buff);

  // Make the payload
  String json = String ("{\"auth_key\":\"XXXXXX\",\"id\":\"" + String (hook_guid) + "\",\"trigger\":\"" + String(hook_report) + "\",\"args\":{\"temperature\":\"") + f + 
                          "\",\"temperature_c\":\"" + c +"\",\"humidity\":\"" + h + "\",\"station\":\"" + station_name + "\"}}";
  int jlen = json.length ();

  WiFiClient client;
  HTTPClient http;
  
  String req = String(hook_url) + "?t=" + f + "&tc=" + c + "&h=" + h;
  Serial.printf ("%s: Connecting to %s\n", GetTime().c_str(), req.c_str());
  Serial.println ("Sending " + json);
  
  http.begin (client, req);
  http.addHeader ("Content-Type", "application/json");
  int httpCode = http.POST (json);

  if (httpCode > 0) {
    Serial.printf ("HTTP response: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      const String& payload = http.getString();
      Serial.println ("Received:");
      Serial.println (payload);
    }
  }
  Serial.println("closing connection\n");
  http.end();
  digitalWrite(LED_BUILTIN, HIGH);
}

//------------------------------------

void DoReset () {
    SPIFFS.format();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    Serial.println ("everything has been reset.");
    delay (100);
    ESP.reset();
    delay (5000);  
}

//adjust LED animations, colors, etc. based on time of day and various values

void DoTimeBasedStuff () {
  int h = GetHours ();
  float f = 0.0;

  float thot = atof (hot_temp);
  float tcold = atof (cold_temp);
  int nstart = atoi (night_start);
  int nend = atoi (night_end);
   
  int lastState = ledState;
  int lastTemp = tempState;
  
  if (h<nend || h>=nstart) { //night time
    ledState = 2;
  }
  else {
    ledState = 1;
    
    //set the led color during the day based on the temperature
    f = getTemperatureF ();
    if (f>thot) {
      strcpy (temp_color, "FF0000");
      tempState = 1;
    }
    else if (f>tcold) {
      strcpy (temp_color, "00FF00");
      tempState = 0;
    }
    else {
      strcpy (temp_color, "0000FF");
      tempState = -1;
    }
    if (tempState != lastTemp) {
      lastState = 0; //force a color change
    }
  }

  if (lastState != ledState) {
    switch (ledState) {
      case 1:
        LEDBeat (temp_color, 3000);
        break;

      case 2:
        LEDColor (light_color);
        break;

      case 3:
        LEDColor (temp_color);
        break;
    }
  }
}

unsigned long sensor_delay = 0;

void DoPeriodic () {
  unsigned long m = millis ();
  
  next_time = m + IDLE_DELAY ; //(update_delay * 1000);

  if (sensor_delay < m) {
    SendSensorData ();
    sensor_delay = m + (update_delay * 1000);
  }

  DoTimeBasedStuff ();
}


void loop() {
  MDNS.update();
  server.handleClient();
  
  if (do_reset) {
    do_reset = false;
    DoReset ();
  }
  else {

    if (next_time < millis ()) {
      DoPeriodic ();
    }

    if (pushData) { // a web request wants us to force an update of sensor data
      SendSensorData ();
    }

    timeLoop ();
    sensorLoop ();
    led_loop ();
    
    delay (1);
  }
}
