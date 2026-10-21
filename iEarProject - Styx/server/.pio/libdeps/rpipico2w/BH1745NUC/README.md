# BH1745NUC: Arduino library for ambient light sensor over I2C

Device: [BH1745NUC by Rohm Semiconductor](https://media.digikey.com/pdf/Data%20Sheets/Rohm%20PDFs/BH1745NUC.pdf)

Interface: I2C

Library: C++ for Arduino

## Usage

```c++
#include <Arduino.h>
#include <BH1745NUC.h>

BH1745NUC bh1745nuc = BH1745NUC();

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  if (!bh1745nuc.begin())
  {
    Serial.println("BH1745NUC not detected or failed to initialize!");
    while (1) { ;; }
  }

  // optional: configure non-default I2C address:
  //   bh1745nuc.begin(0x39)
  // optional: set measurement time
  //   bh1745nuc.setMeasurementTime(bh1745nuc.Time160ms);
  // optional: set gain
  //   bh1745nuc.setGain(bh1745nuc.Gain1X);
  // optional: start or stop measurement (power down mode)
  //   bh1745nuc.startMeasurement();
  //   bh1745nuc.stopMeasurement();
}

void loop()
{
  if (!bh1745nuc.read())
  {
    Serial.println("BH1745NUC: failed to read measurement values!");
    delay(500);
    return;
  }
  Serial.printf("Red: %hu, Green: %hu, Blue: %hu, Clear: %hu\n", red, green, blue, clear);
  delay(500);
}
```

## License

This project is made available under the MIT License. For more details, see the LICENSE file in the repository.

## Author

This project was created by Thomas Kriechbaumer.
