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
