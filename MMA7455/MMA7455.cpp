#include <Arduino.h>
#include "MMA7455.h"

MMA7455::MMA7455()
{
  MMA7455(TWO_G, MEASURE_MODE);
}

MMA7455::MMA7455(int g_mode, int device_mode)
{
  int error;
  uint8_t c;
  
  Wire.begin();
  error = init(g_mode, device_mode);

  read(MMA7455_STATUS, &c, 1);

  read(MMA7455_WHOAMI, &c, 1);

  read(MMA7455_TOUT, &c, 1);
}

MMA7455::~MMA7455(){}

// Initialize the sensor
  //
  // Sensitivity:
  //    2g : GLVL0
  //    4g : GLVL1
  //    8g : GLVL1 | GLVL0
  // Mode:
  //    Standby         : 0
  //    Measurement     : MODE0
  //    Level Detection : MODE1
  //    Pulse Detection : MODE1 | MODE0
int MMA7455::init(int g_mode, int device_mode)
{
  uint16_t x, y, z;
  int error;
  xyz_union xyz;
  uint8_t c1, c2;

  c1 = bit(g_mode) | bit(device_mode);
  error = write(MMA7455_MCTL, &c1, 1);
  if (error != 0)
    return (error);

  error = read(MMA7455_MCTL, &c2, 1);
  if (error != 0)
    return (error);

  if (c1 != c2)
    return (-99);

  xyz.value.x = xyz.value.y = xyz.value.z = 0;
  error = write(MMA7455_XOFFL, (uint8_t *) &xyz, 6);
  if (error != 0)
    return (error);

  delay(100);

#define USE_INTERNAL_OFFSET_REGISTERS
#ifdef USE_INTERNAL_OFFSET_REGISTERS

  error = measure(&x, &y, &z);
  if (error != 0)
    return (error);

  xyz.value.x = 2 * -x;        
  xyz.value.y = 2 * -y;
  xyz.value.z = 2 * -(z - 64); 

  error = write(MMA7455_XOFFL, (uint8_t *) &xyz, 6);
  if (error != 0)
    return (error);
    
  delay(200);

  error = measure(&x, &y, &z);    
  if (error != 0)
    return (error);

  xyz.value.x += 2 * -x;       
  xyz.value.y += 2 * -y;
  xyz.value.z += 2 * -(z - 64);

  error = write(MMA7455_XOFFL, (uint8_t *) &xyz, 6);
  if (error != 0)
    return (error);

#endif

  return 0; 
}

int MMA7455::write(int start, const uint8_t *pData, int size)
{
  int n, error;

  Wire.beginTransmission(MMA7455_I2C_ADDRESS);
  n = Wire.write(start);        
  if (n != 1)
    return (-20);

  n = Wire.write(pData, size);  
  if (n != size)
    return (-21);

  error = Wire.endTransmission(true);
  if (error != 0)
    return (error);

  return (0);                   
}

int MMA7455::read(int start, uint8_t *buffer, int size)
{
  int i, n, error;

  Wire.beginTransmission(MMA7455_I2C_ADDRESS);
  n = Wire.write(start);
  if (n != 1)
    return (-10);

  n = Wire.endTransmission(false);
  if (n != 0)
    return (n);

  Wire.requestFrom(MMA7455_I2C_ADDRESS, size, true);
  i = 0;
  while (Wire.available() && i < size)
  {
    buffer[i++] = Wire.read();
  }
  if ( i != size)
    return (-11);

  return (0);                 
}

int MMA7455::measure( uint16_t *pX, uint16_t *pY, uint16_t *pZ)
{
  xyz_union xyz;
  int error;
  uint8_t c;

  do
  {
    error = read(MMA7455_STATUS, &c, 1);
  } while ( !bitRead(c, MMA7455_DRDY) && error == 0);
  if (error != 0)
    return (error);

  error = read(MMA7455_XOUTL, (uint8_t *) &xyz, 6);
  if (error != 0)
    return (error);

  if (xyz.reg.x_msb & 0x02)    
    xyz.reg.x_msb |= 0xFC;     
  else
    xyz.reg.x_msb &= 0x3;

  if (xyz.reg.y_msb & 0x02)
    xyz.reg.y_msb |= 0xFC;
  else
    xyz.reg.y_msb &= 0x3;

  if (xyz.reg.z_msb & 0x02)
    xyz.reg.z_msb |= 0xFC;
  else
    xyz.reg.z_msb &= 0x3;

  *pX = xyz.value.x;          
  *pY = xyz.value.y;
  *pZ = xyz.value.z;
  
  return 0;                
}

void MMA7455::update_mode(int g_mode, int device_mode)
{
  uint8_t c;
  int error = 0;
  
  error = init(g_mode, device_mode);

  read(MMA7455_STATUS, &c, 1);
  read(MMA7455_WHOAMI, &c, 1);
  read(MMA7455_TOUT, &c, 1);

}
