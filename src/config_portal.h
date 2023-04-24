#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>

//#include <HTTPClient.h>
#include "ESPAsyncWebSrv.h"
//#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
//#include <WiFiClientSecure.h>


String getWifiSSID();

String getWifiPass();

String getOwner();

int getReboots();

void prefsBegin();

void ResetPrefs();

void setReboots(int);

void notFound(AsyncWebServerRequest *request);

void webserverloop();

void setupCaptivePortal();





#endif