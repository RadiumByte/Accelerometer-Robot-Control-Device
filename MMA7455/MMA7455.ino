#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include "MMA7455.h"

MMA7455 sensor(FOUR_G, MEASURE_MODE);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;

double baseThrottle;
double baseSteering;
bool needToSetup;

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Robolab", "robolab011a");
  http.setReuse(true);
  Serial.println("Remote contol with MMA7455 accelerometer starting...");

  baseThrottle = 0.0;
  baseSteering = 0.0;
  needToSetup = true;
}

void CreateCommands(double dX, double dY, String& throttle, String& steering)
{
  throttle = "";
  steering = "";

  if (dY > baseThrottle)
    throttle += "F";
  else
    throttle += "B";

  double throttleDelta = fabs(dY - baseThrottle);
  int throttleConverted = (int)(throttleDelta * 100.0);

  if (throttleConverted > 100)
    throttleConverted = 100;
  else if (throttleConverted < 0)
    throttleConverted = 0;

  throttle += String(throttleConverted);

  int steeringPercent = (int)(fabs(dX - baseSteering) * 100.0);
  int steeringConverted = map(steeringPercent, 0, 100, 0, 50);
  steering += "S";

  if (dX > baseSteering)
  {
    int steeringToSend = 50 - steeringConverted;

    if (steeringToSend > 100)
      steeringToSend = 100;
    else if (steeringToSend < 0)
      steeringToSend = 0;

    steering += String(steeringToSend);
  }
  else
  {
    int steeringToSend = 50 + steeringConverted;

    if (steeringToSend > 100)
      steeringToSend = 100;
    else if (steeringToSend < 0)
      steeringToSend = 0;

    steering += String(steeringToSend);
  }
}

// Core driving function - gets values from MMA and calculates commands
void CarControl(String& throttle, String& steering)
{
  uint16_t x, y, z, error;
  double dX, dY, dZ;
  dX = 0.0;
  dY = 0.0;

  error = sensor.measure(&x, &y, &z);
  dX = (int16_t) x / 64.0;
  dY = (int16_t) y / 64.0;

  if (needToSetup)
  {
    needToSetup = false;
    baseThrottle = dY;
    baseSteering = dX;
    return;
  }

  CreateCommands(dX, dY, throttle, steering);
  Serial.print(throttle);
  Serial.print("  ");
  Serial.println(steering);
  // if unblock button pressed
  /*
    if (!digitalRead(NEXT_BTN))
    {
    // TO DO: send throttle
    // TO DO: send steering
    }
    else
    {
    // TO DO: send throttle = 0
    }
  */
}

void loop()
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    WiFiClient client;

    String url = "http://192.168.183.101:8080/";
    String throttle = "";
    String steering = "";
    CarControl(throttle, steering);

    if (http.begin(client, url + throttle))
    {
      int httpCode = http.PUT("Command");

      if (httpCode > 0)
      {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
    if (http.begin(client, url + steering))
    {
      int httpCode = http.PUT("Command");

      if (httpCode > 0)
      {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }

  delay(500);
}
