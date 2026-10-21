#include <Arduino.h>
#include "BH1745NUC.h"          //light measurement sensor library
#include <PDM.h>
#include "SPI.h"
#include <stdio.h>
#include "Wire.h"
#include <Adafruit_GFX.h>       //OLED display support library
#include <Adafruit_SSD1306.h>   //OLED display library
#include "bme68xLibrary.h"
#include <ctime>


//define PDM mic parameters
#define BIT_PER_SAMPLE 16

//-- defines OLED screen dimensions ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # 
#define SCREEN_ADDRESS 0x3C //OLED I2C address
#define NEW_GAS_MEAS (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK | BME68X_NEW_DATA_MSK)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//default number of output channels
static const char channels = 1;

//default PCM output frequency or sample rate
static const int frequency = 16000; //24000; 32000; 41200;

//Buffer to read samples into, each sample is 16-bits
short sampleBuffer[256]; //[512]

//Number of audio samples read
volatile int samplesRead;

// float temps[24];
// unsigned short lights[24][4];
// double sounds[24];

//track recording status
int recordingStatus = 0;

//create PDM microphone object with pin GP2 as data input and pin GP3 as clock output
PDMClass PDM(2,3);

//----defines light light measurement sensor parameters
#define BH1745NUC_DEVICE_ADDRESS_38 0x38    //light measurement sensor I2C address
BH1745NUC bh1745nuc = BH1745NUC();

float prev_temp = 0;

// climate sensor variable
Bme68x bme;  

void readLightMeasurements();
void readClimateValues();

void onPDMdata();
void getSoundSample();

void resetDisplay(int);

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed; check wiring and I2C address."));
    while (true) {
        delay(1000);
        }
    }

    resetDisplay(0);
    delay(1000);
    display.clearDisplay();

    //start light level measurements
    bh1745nuc.begin(BH1745NUC_DEVICE_ADDRESS_38);
    bh1745nuc.startMeasurement();

    // Configure the data receive callback
    PDM.onReceive(onPDMdata);
    //Initialize PDM with:
    //- one channel (mono mode) and 16 kHz sample rate 
    if (!PDM.begin(channels, frequency)) {
        Serial.println("Failed to start PDM!");
        display.print(F("Failed to start PDM!"));
        display.display();
    while (1);
    }
    PDM.setGain(1);

    bme.begin(0x76, Wire);

    if (bme.checkStatus()) {
    if (bme.checkStatus() == BME68X_ERROR) {
        Serial.println("Sensor error: " + bme.statusString());
        return;
    }
    else if (bme.checkStatus() == BME68X_WARNING) {
        Serial.println("Sensor Warning: " + bme.statusString());
    }
    }

    //sets the default configuration for temperature, pressure and humidity
    bme.setTPH();

    //sets sensor heater temperature in degree Celsius 
    uint16_t tempProf[10] = { 100, 200, 320 };
    //sets heating duration in milliseconds 
    uint16_t durProf[10] = { 150, 150, 150 };

    bme.setSeqSleep(BME68X_ODR_250_MS);
    bme.setHeaterProf(tempProf, durProf, 3);
    bme.setOpMode(BME68X_SEQUENTIAL_MODE);
}

void loop() {
    resetDisplay(0);

    readClimateValues();

    getSoundSample();

    readLightMeasurements();

    display.display();  
}

void readClimateValues() {
  bme68xData data;
  uint8_t nFieldsLeft = 0;

  if (bme.fetchData()) {
    do {
      nFieldsLeft = bme.getData(data);

      if (data.status == NEW_GAS_MEAS) {
        Serial.print(String(data.temperature-4.49) + ", ");
        prev_temp = data.temperature-4.49;
        if (data.gas_index == 2) delay(250);
        display.setCursor(0,0);
        display.print("Temperature: ");
        display.print(String(data.temperature - 4.49));
        display.println(F("'C"));

        
      }
    }

    while (nFieldsLeft);
  }
  else // If no data is found
  {
    if (prev_temp){
        display.print("Temperature: ");
        display.print(String(prev_temp));
        display.println(F("'C"));
    }
    else {
        display.setCursor(0,0);
        display.println("Temperature: No data!");
    }
  }
}

void readLightMeasurements() {
    if (!bh1745nuc.read()) {
        Serial.println("Light sensor failed to read measurement values!");
        delay(500);
        return;
    }

    unsigned short rgbc[4];
    rgbc[0] = bh1745nuc.red;
    rgbc[1] = bh1745nuc.green;
    rgbc[2] = bh1745nuc.blue;
    rgbc[3] = bh1745nuc.clear;

    //print light values on the serial monitor
    Serial.write("Red = ");
    Serial.print(rgbc[0]);
    Serial.write(" Green = ");
    Serial.print(rgbc[1]);
    Serial.write(" Blue = ");
    Serial.print(rgbc[2]);
    Serial.write(" Clear = ");
    Serial.println(rgbc[3]);  

    //display light values in OLED display
    display.setCursor(0,20);
    display.println(F("Light measurements.."));
    display.print(F("Red:   "));
    display.println(String(rgbc[0]));
    display.print(F("Green: "));
    display.println(String(rgbc[1]));
    display.print(F("Blue:  "));
    display.println(String(rgbc[2]));
    display.print(F("Clear: "));
    display.println(String(rgbc[3]));
}

//Callback function to process the data from the PDM microphone.
//NOTE: This callback is executed as part of an ISR.
//Therefore using `Serial` to print messages inside this function isn't supported.
void onPDMdata() {
    // Query the number of available bytes
    int bytesAvailable = PDM.available();

    // Read into the sample buffer
    PDM.read(sampleBuffer, bytesAvailable);
    // 16-bit, 2 bytes per sample
    samplesRead = bytesAvailable / 2;
}

//get sound sample
void getSoundSample() {

    //wait for audio samples to be read
    Serial.print("test");
    if (samplesRead) {
        long sumSquare = 0;
        int peakValue = 0;

        for (int i=0; i<samplesRead; i++) {
            int sample = sampleBuffer[i];

            int absSample = abs(sample);
            if (absSample > peakValue) {
                peakValue = absSample;
            }
            sumSquare += long(sample) * sample;
        }
        float rmsValue = sqrt((float)sumSquare / samplesRead);
        samplesRead = 0;  //clear the read count
        display.setCursor(0, 10);
        display.print(F("Sound level: "));
        display.println(rmsValue);
    }
}

void resetDisplay(int cursor) {
  display.clearDisplay();                 //clear OLED screen
  display.setTextSize(1);                 //Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);    //Draw white text
  display.setCursor(0,cursor);                 //Start at top-left corner (Col=0, Row=0)
}
