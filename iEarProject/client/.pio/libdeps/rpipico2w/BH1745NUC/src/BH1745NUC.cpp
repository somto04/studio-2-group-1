#include "BH1745NUC.h"

BH1745NUC::BH1745NUC() {}

bool BH1745NUC::begin(uint8_t addr)
{
  address = addr;

  uint8_t v;
  if (!readRegister(CMD_SYSTEM_CONTROL, &v))
  {
    return false;
  }
  if ((v & 0x1F) != PART_NUMBER)
  {
    return false;
  }

  if (!setMeasurementTime(Time160ms))
  {
    return false;
  }

  if (!setGain(Gain1X))
  {
    return false;
  }

  if (!writeRegister(CMD_MODE_CONTROL3, MODE_CONTROL3_DATA))
  {
    return false;
  }

  if (!startMeasurement())
  {
    return false;
  }

  return true;
}

bool BH1745NUC::setMeasurementTime(MeasurementTime time)
{
  return writeRegister(CMD_MODE_CONTROL1, time);
}

bool BH1745NUC::startMeasurement()
{
  uint8_t v;
  if (!readRegister(CMD_MODE_CONTROL2, &v))
  {
    return false;
  }

  // set the 5th bit to start measurements
  return writeRegister(CMD_MODE_CONTROL2, v | (1 << 4));
}

bool BH1745NUC::stopMeasurement()
{
  uint8_t v;
  if (!readRegister(CMD_MODE_CONTROL2, &v))
  {
    return false;
  }

  // unset the 5th bit to stop measurements
  return writeRegister(CMD_MODE_CONTROL2, v & ~(1 << 4));
}

bool BH1745NUC::setGain(Gain gain)
{
  uint8_t v;
  if (!readRegister(CMD_MODE_CONTROL2, &v))
  {
    return false;
  }
  v &= ~(1UL << 0);
  v &= ~(1UL << 1);
  return writeRegister(CMD_MODE_CONTROL2, v | gain);
}

bool BH1745NUC::read()
{
  Wire.beginTransmission(address);
  if (Wire.write(RED_DATA_LSB) != 1)
  {
    return false;
  }
  if (Wire.endTransmission() != 0)
  {
    return false;
  }

  uint8_t buffer[8]; // red, green, blue, clear with 2 bytes each = 8 bytes total sequentially starting at RED_DATA_LSB
  if (Wire.requestFrom(address, (uint8_t)sizeof(buffer)) != sizeof(buffer))
    ;

  uint8_t i = 0;
  while (Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }

  red = (buffer[1] << 8) + buffer[0];
  green = (buffer[3] << 8) + buffer[2];
  blue = (buffer[5] << 8) + buffer[4];
  clear = (buffer[7] << 8) + buffer[6];

  return true;
}

bool BH1745NUC::readRegister(uint8_t reg, uint8_t *value)
{
  Wire.beginTransmission(address);
  if (Wire.write(reg) != 1)
  {
    return false;
  }
  if (Wire.endTransmission() != 0)
  {
    return false;
  }
  if (Wire.requestFrom(address, (uint8_t)1) != 1)
  {
    return false;
  }
  *value = Wire.read();
  return true;
}

bool BH1745NUC::writeRegister(uint8_t reg, uint8_t data)
{
  Wire.beginTransmission(address);
  if (Wire.write(reg) != 1)
  {
    return false;
  }
  if (Wire.write(data) != 1)
  {
    return false;
  }
  if (Wire.endTransmission() != 0)
  {
    return false;
  }
  return true;
}
