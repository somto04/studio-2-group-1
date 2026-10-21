#pragma once

#include "Arduino.h"
#include "Wire.h"

#define BH1745_DEFAULT_I2C_ADDR 0x38

#define CMD_SYSTEM_CONTROL 0x40
#define CMD_MODE_CONTROL1 0x41
#define CMD_MODE_CONTROL2 0x42
#define CMD_MODE_CONTROL3 0x44

#define RED_DATA_LSB 0x50
#define RED_DATA_MSB 0x51
#define GREEN_DATA_LSB 0x52
#define GREEN_DATA_MSB 0x53
#define BLUE_DATA_LSB 0x54
#define BLUE_DATA_MSB 0x55
#define CLEAR_DATA_LSB 0x56
#define CLEAR_DATA_MSB 0x57

#define PART_NUMBER 0x0B
#define MODE_CONTROL3_DATA 0x02

class BH1745NUC
{
public:
  enum MeasurementTime : uint8_t
  {
    Time160ms = 0x00,
    Time320ms = 0x01,
    Time640ms = 0x02,
    Time1280ms = 0x03,
    Time2560ms = 0x04,
    Time5120ms = 0x05,
  };

  enum Gain : uint8_t
  {
    Gain1X = 0x00,
    Gain2X = 0x01,
    Gain16X = 0x02,
  };

  BH1745NUC();

  bool begin(uint8_t addr = BH1745_DEFAULT_I2C_ADDR);
  bool setMeasurementTime(MeasurementTime time);
  bool setGain(Gain gain);
  bool startMeasurement();
  bool stopMeasurement();
  bool read();

  uint16_t red, green, blue, clear;

private:
  bool readRegister(uint8_t addr, uint8_t *value);
  bool writeRegister(uint8_t addr, uint8_t data);

  uint8_t address;
};
