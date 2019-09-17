#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include "MMA7455.h"

#define BLOCK_BTN D4

MMA7455 sensor(FOUR_G, MEASURE_MODE);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;

double baseThrottle;
double baseSteering;
bool needToSetup;

int prevThrottle;
int prevSteering;

void setup()
{
  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Robolab", "robolab011a");
  http.setReuse(true);
  Serial.println("Remote contol with MMA7455 accelerometer starting...");

  pinMode(BLOCK_BTN, INPUT_PULLUP);

  baseThrottle = 0.0;
  baseSteering = 0.0;
  needToSetup = true;

  prevThrottle = 0;
  prevSteering = 0;
}

void loop()
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    WiFiClient client;

    String url = "http://192.168.183.100:8080/";

    String throttle = "";
    String steering = "";

    // Get gravity vector
    uint16_t x, y, z, error;
    double dX, dY, dZ;
    dX = 0.0;
    dY = 0.0;
    error = sensor.measure(&x, &y, &z);
    dX = (int16_t) x / 64.0;
    dY = (int16_t) y / 64.0;

    // Initial iteration - orientation of device is set as base
    if (needToSetup)
    {
      needToSetup = false;
      baseThrottle = dY;
      baseSteering = dX;
      return;
    }

    // Determine the direction of throttle
    // F stands for Forward
    // B stands for Backward
    if (dY > baseThrottle)
      throttle += "F";
    else
      throttle += "B";

    // Calculate the throttle
    double throttleDelta = fabs(dY - baseThrottle);
    int throttleConverted = (int)(throttleDelta * 100.0);

    // Check exceeding values
    if (throttleConverted > 100)
      throttleConverted = 100;
    else if (throttleConverted < 0)
      throttleConverted = 0;

    // If block button pressed
    if (digitalRead(BLOCK_BTN))
    {
      throttleConverted = 0;
    }

    // Sensivity check for device
    // If this check failed - no need to send new command to server
    if (abs(throttleConverted  - prevThrottle) > 4)
    {
      // Throttle changed significantly
      prevThrottle = throttleConverted;
      throttle += String(throttleConverted);
      Serial.println(throttle);

      if (http.begin(client, url + throttle))
      {
        int httpCode = http.PUT("Command");

        if (httpCode > 0)
        {
          Serial.printf("[HTTP] PUT... code: %d\n", httpCode);
        }
        else
        {
          Serial.printf("[HTTP] PUT... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      }

    }

    // Calculate the steering
    int steeringPercent = (int)(fabs(dX - baseSteering) * 100.0);
    int steeringConverted = map(steeringPercent, 0, 100, 0, 50);

    steering += "S";
    int steeringToSend = 50;

    // Steering left
    if (dX > baseSteering)
      steeringToSend -= steeringConverted;
    // Steering right
    else
      steeringToSend += steeringConverted;

    if (steeringToSend > 100)
      steeringToSend = 100;
    else if (steeringToSend < 0)
      steeringToSend = 0;

    // Sensivity check
    // If it was failed - no need to send steering again
    if (abs(steeringToSend  - prevSteering) > 4)
    {
      prevSteering = steeringToSend;
      steering += String(steeringToSend);
      Serial.println(steering);

      if (http.begin(client, url + steering))
      {
        int httpCode = http.PUT("Command");

        if (httpCode > 0)
        {
          Serial.printf("[HTTP] PUT... code: %d\n", httpCode);
        }
        else
        {
          Serial.printf("[HTTP] PUT... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      }

    }
  }

  delay(250);
}
