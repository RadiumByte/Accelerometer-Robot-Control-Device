#include "MMA7455.h"

MMA7455 sensor(FOUR_G, MEASURE_MODE);

void setup()
{
  Serial.begin(9600);
  Serial.println("Remote contol with MMA7455 accelerometer starting...");
  pinMode(13, INPUT_PULLUP);
}

void loop()
{
  uint16_t x, y, z, error;
  double dX, dY, dZ;
  dX = 0.0;
  dY = 0.0;

  error = sensor.measure(&x, &y, &z); 
  dX = (int16_t) x / 64.0;          
  dY = (int16_t) y / 64.0;
  
  Serial.print("error = ");
  Serial.print(error, DEC);
  Serial.print(", xyz g-forces = ");
  Serial.print(dX, 3);
  Serial.print(", ");
  Serial.print(dY, 3);
  Serial.println("");

  delay(300);
 
  Serial.println(digitalRead(13));
  delay(300);
}
