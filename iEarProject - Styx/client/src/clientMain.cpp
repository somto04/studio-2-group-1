#include <Arduino.h>
#include "BH1745NUC.h"          //light measurement sensor library
#include <PDM.h>
#include "SPI.h"
#include <stdio.h>
#include "Wire.h"
#include <Adafruit_GFX.h>       //OLED display support library
#include <Adafruit_SSD1306.h>   //OLED display library
#include "bme68xLibrary.h"
#include <WiFiClient.h>         
#include <WiFi.h>            
#include "LEAmDNS.h" 
#include "SparkFun_LSM6DSV16X.h" //inertial sensor library not available in PlatformIO libraries.
#include <ctime>
#include <string>
#include <WiFiNTP.h>

//define PDM mic parameters
#define BIT_PER_SAMPLE 16

//-- defines OLED screen dimensions ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # 
#define SCREEN_ADDRESS 0x3C //OLED I2C address
#define NEW_GAS_MEAS (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK | BME68X_NEW_DATA_MSK)
#define pin28 28

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//default number of output channels
static const char channels = 1;

//default PCM output frequency or sample rate
static const int frequency = 16000; //24000; 32000; 41200;

//Buffer to read samples into, each sample is 16-bits
short sampleBuffer[256]; //[512]

//Number of audio samples read
volatile int samplesRead;

//track recording status
int recordingStatus = 0;

//create PDM microphone object with pin GP2 as data input and pin GP3 as clock output
PDMClass PDM(2,3);

//----defines light light measurement sensor parameters
#define BH1745NUC_DEVICE_ADDRESS_38 0x38    //light measurement sensor I2C address
BH1745NUC bh1745nuc = BH1745NUC();

double prev_temp = 0;

// climate sensor variable
Bme68x bme;  

const char* ssid      = "Team_1";
const char* password  = "123456789";
const char* host      = "192.168.4.169";
WiFiClient client;                         
String sensorData;                     //variable to hold sensor data for posting
float  temp, sound;
unsigned short rgbc[4];
bool objectAccessed;

#define SensorNumber 1                 //enables you to identify devices uniquely
#define ZoneNumber 1

//initialize gyroscope values
float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;
float gOffX = 0.0;
float gOffY = 0.0;
float gOffZ = 0.0;

float prevGyroX = 0.0;
float prevGyroY = 0.0;
float prevGyroZ = 0.0;

SparkFun_LSM6DSV16X myLSM;  //creates inertial sensor object

// Structs for X,Y,Z data
sfe_lsm_data_t gyroData;

//declare the functions implemented in program
void initializeSensor();
void connectToServer();
void readSensorMeasurement();     
void structureSensorData();
void sendHTTPSensorData();
void readLightMeasurements();
void readClimateValues();
void onPDMdata();
void getSoundSample();
void resetDisplay(int);
void initialiseSensors();
void calibrateGyro();
void display3DGyroValues();
void syncClock();
String getTimeString();

void setup() {
    Serial.begin(115200);
    Wire.begin();
    WiFi.mode(WIFI_STA);                     //set WiFi as station (i.e., client)
    WiFi.begin(ssid,password);
    Serial.println();
    delay(10);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed; check wiring and I2C address."));
    while (true) {
        delay(1000);
        }
    }

    resetDisplay(0);
    delay(1000);
    display.clearDisplay();
    connectToServer();
    initialiseSensors();
    syncClock();
}

void loop() {
    resetDisplay(0);
    readClimateValues();
    getSoundSample();
    readLightMeasurements();

    structureSensorData();
    sendHTTPSensorData();
    
    display3DGyroValues(); 
    display.display();  
}

void syncClock() { // for getting accurate time 
  NTP.begin("pool.ntp.org", "time.nist.gov");
  if (!NTP.waitSet(15000)) {
      Serial.println("NTP time sync failed");
  }

  // UK time, including daylight saving time
  setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0/2", 1);
  tzset();
}

void initialiseSensors() {
  //start light level measurements
    bh1745nuc.begin(BH1745NUC_DEVICE_ADDRESS_38);
    bh1745nuc.startMeasurement();

    // sound
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

    // temperature
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

    // gyroscope
        if (!myLSM.begin()) //starts inertial measurement sensor
    {
        Serial.println("Did not begin, check your wiring and/or I2C address!");
        while (1);
    }

    // Reset the device to default settings. This if helpful is you're doing multiple
    // uploads testing different settings.
    myLSM.deviceReset();

    // Wait for it to finish resetting
    while (!myLSM.getDeviceReset())
    {
        delay(1);
    }

    // Accelerometer and Gyroscope registers will not be updated
    // until read.
    myLSM.enableBlockDataUpdate();

    // Sets the output data rate and precision of the accelerometer
    myLSM.setAccelDataRate(LSM6DSV16X_ODR_AT_7Hz5);
    myLSM.setAccelFullScale(LSM6DSV16X_16g);

    // Sets the output data rate and precision of the gyroscope
    myLSM.setGyroDataRate(LSM6DSV16X_ODR_AT_15Hz);
    myLSM.setGyroFullScale(LSM6DSV16X_2000dps);

    // Enables filter settling.
    myLSM.enableFilterSettling();

    // Turns on the accelerometer's filter and apply settings.
    myLSM.enableAccelLP2Filter();
    myLSM.setAccelLP2Bandwidth(LSM6DSV16X_XL_STRONG);

    // Turns on the gyroscope's filter and apply settings.
    myLSM.enableGyroLP1Filter();
    myLSM.setGyroLP1Bandwidth(LSM6DSV16X_GY_ULTRA_LIGHT);
    
    //calibrate gyroscope
    calibrateGyro(); 
}

void display3DGyroValues()
{
  if (myLSM.checkStatus())
  {
    objectAccessed = false;
    myLSM.getGyro(&gyroData);
    gyroX = gyroData.xData - gOffX;
    gyroY = gyroData.yData - gOffY;
    gyroZ = gyroData.zData - gOffZ;

    // if there is a substantial different not just a bump
    if (abs(gyroX - prevGyroX) > 300 || abs(gyroY - prevGyroY) > 300 || abs(gyroZ - prevGyroZ) > 300) objectAccessed = true;

    Serial.print("GyroX: ");
    Serial.print(gyroX);
    Serial.println(" deg/s");
 
    Serial.print("GyroY: ");
    Serial.print(gyroY);
    Serial.println(" deg/s");

    Serial.print("GyroZ: ");
    Serial.print(gyroZ);
    Serial.println(" deg/s");

    Serial.println();

    prevGyroX = gyroX;
    prevGyroY = gyroY;
    prevGyroZ = gyroZ;
  }
}

void calibrateGyro()
{
  float gTempX = 0.0;
  float gTempY = 0.0;
  float gTempZ = 0.0;
  int i =0;

  display.clearDisplay();                 //clears OLED screen
  display.setTextSize(1);                 //Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);    //Draw white text
  
  display.setCursor(0,0); 
  display.println(F("Calibrating gyroscope"));
  display.setCursor(0,20);
  display.println(F("Please wait ...."));
  display.setCursor(0,40); 
  display.println(F("Keep device STILL"));
  display.println(F("& on a FLAT surface")); 
  display.display();

  while(i<200)
  {
    if (myLSM.checkStatus())
    {
      myLSM.getGyro(&gyroData);
      gTempX += gyroData.xData;
      gTempY += gyroData.yData;
      gTempZ += gyroData.zData;
      i += 1;
    }
  }
    gOffX = gTempX/200.0;
    gOffY = gTempY/200.0;
    gOffZ = gTempZ/200.0;
}

void readClimateValues() {
  bme68xData data;
  uint8_t nFieldsLeft = 0;

  if (bme.fetchData()) {
    do {
      nFieldsLeft = bme.getData(data);

      if (data.status == NEW_GAS_MEAS) {
        temp = data.temperature - 4.49;
        Serial.print(String(temp) + ", ");
        prev_temp = data.temperature-4.49;
        if (data.gas_index == 2) delay(250);
        display.setCursor(0,0);
        display.print("Temperature: ");
        display.print(String(temp));
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
        sound = rmsValue;
        samplesRead = 0;  //clear the read count
        display.setCursor(0, 10);
        display.print(F("Sound level: "));
        display.println(rmsValue);
    }
}

void connectToServer(){
  
  while(WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to connect to Server");
    display.clearDisplay();                 //clears OLED screen 
    display.setCursor(0,20);
    display.println("Attempting to connect");
     display.setCursor(0,30);
    display.println("to server ...."); 
    display.display();
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to server IP Address (AP): "); 
  Serial.println(WiFi.localIP());

  display.clearDisplay();                 //clears OLED screen 
  display.setCursor(0,5);
  display.print("Sensor No: ");
  display.println(String(SensorNumber));
  display.print("Zone No: ");
  display.println(String(ZoneNumber));
  display.setCursor(0,15);

  display.setCursor(0,35);
  display.println("Connected to server");
  display.setCursor(0,45);
  display.print("@IP: ");
  display.println(F(host));
  display.display();
}

String getTimeString() {
  time_t currentTime = time(NULL);
  struct tm *timeInfo = localtime(&currentTime);

  char timeString[9];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", timeInfo);

  return String(timeString);
}

void sendHTTPSensorData() {
  //if disconnected loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting connection...");
    if (client.connect(host,80)) {
      Serial.println("connected");
    } else {
      Serial.println(" try again in 500 ms");
      delay(500); //wait 5ms before trying again
    }
  }
  client.print("GET /");
  client.print(sensorData);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
  delay(500);
  client.stop();                             //stop client
}

void structureSensorData() {
  Serial.println("Structuring data request");
  sensorData  = "sensor?Sensor=" + String(SensorNumber);
  sensorData += "zone?Zone=" + String(ZoneNumber);
  sensorData += "&temp=" + String(temp);
  sensorData += "&sound=" + String(sound);
  sensorData += "&light=" + String(rgbc[0]) + "," + String(rgbc[1]) + "," + String(rgbc[2]) + "," +  String(rgbc[3]);
  sensorData += "&accessed=" + String(objectAccessed);
  sensorData += "&time=" + getTimeString();

  Serial.println("...Information to be posted: "+sensorData);
}

void resetDisplay(int cursor) {
  display.clearDisplay();                 //clear OLED screen
  display.setTextSize(1);                 //Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);    //Draw white text
  display.setCursor(0,cursor);                 //Start at top-left corner (Col=0, Row=0)
}
