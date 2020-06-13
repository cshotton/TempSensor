#ifndef LEDS_H

#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#define DATA_PIN    D6
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    1
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND  60

typedef void (*PatternFunc)();
void _rainbow ();
void _blinker ();

PatternFunc pattern = _rainbow;

CRGB color_1;
CRGB color_2;

void led_setup () {
  Serial.println ("Configuring LEDs...");
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}


uint8_t gHue = 0; // rotating "base color" used by many of the patterns
unsigned long next_led_time = 0;
bool bstate = false;
unsigned long the_delay = 10;

unsigned long _hextocolor (String color) {
  return strtol (color.c_str(), NULL, 16);
}

void _rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  FastLED.show();
  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
}


void _blinker () {
  EVERY_N_MILLISECONDS( the_delay ) {
    if (bstate) {
      leds [0] = color_1;
    }
    else {
      leds [0] = color_2;
    }
    bstate = !bstate;
    FastLED.show();
  }
}

void _beater () {
  EVERY_N_MILLISECONDS( the_delay ) {
    leds [0] = color_1;
  }
  leds [0].fadeToBlackBy (48);
  FastLED.show();
}

void _color () {
  if (bstate) {
    leds [0] = color_1;
    FastLED.show();
    bstate = false;
  }
}

void LEDBlink (String c1, unsigned long speed) {
  color_1 = _hextocolor (c1);
  color_2 = CRGB::Black;
  pattern = _blinker;
  the_delay = speed;
  bstate = true;
}

void LEDBeat (String c1, unsigned long speed) {
  color_1 = _hextocolor (c1);
  color_2 = CRGB::Black;
  pattern = _beater;
  the_delay = speed;
}

void LEDFlash (String c1, String c2, unsigned long speed) {
  color_1 = _hextocolor (c1);
  color_2 = _hextocolor (c2);
  pattern = _blinker;
  the_delay = speed;
  bstate = true;
}

void LEDColor (String c1) {
  color_1 = _hextocolor (c1);
  color_2 = CRGB::Black;
  pattern = _color;
  bstate = true;
  pattern();
}

void led_loop () {
  unsigned long m = millis();
  if (next_led_time < m) {
    next_led_time = m + 1000/FRAMES_PER_SECOND;
    pattern ();
  }
}

#define LEDS_H 1
#endif
