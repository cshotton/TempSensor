#ifndef TIMES_H

#include <WiFiUdp.h>
#include <NTPClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void SetGMTOffset (String offset) {
  int off = atoi (offset.c_str()) * 60 * 60;
  timeClient.setTimeOffset (off);
}

unsigned long GetTicks () {
  return timeClient.getEpochTime ();  
}

int GetHours () {
  return timeClient.getHours ();
}

int GetMinutes () {
  return timeClient.getMinutes ();
}

int GetSeconds () {
  return timeClient.getSeconds ();
}

String GetTime () {
  return timeClient.getFormattedTime ();
}

void timeSetup () {
  timeClient.begin();
  timeClient.update();
}

void timeLoop () {
  timeClient.update();
}

#define TIMES_H 1
#endif
