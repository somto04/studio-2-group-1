#include <Arduino.h>
#include <Arduino.h>
#include "WiFi.h"
#include <WebServer.h>      
#include <WiFiClient.h>   
//use domain name service library to enable you to worksaround changing IP addresses
#include "LEAmDNS.h"
#include <W5100lwIP.h>   
#include <lwip/dhcp.h> 

//configure parameters for Access Point

const char* ssid_ap = "Team_1";
const char* password_ap = "123456789";

IPAddress local_ip(192,168,4,169); // arbitrary IP address (user-defined IP address). 
                                 // You can change last digit so that it's not the same 
                                 // as other teams, 192,168,4,5; 192,168,4,7; 192,168,4,17; etc. )

IPAddress gateway(192,168,4,169);  // same as local_ip
IPAddress subnet(255,255,255,0); // keep unchanged

//create a WebServer object on port 80
WebServer server(80);

//function declarations
void handleRoot();
void handleNotFound();
bool ReceivedAnySensor();
void handleSensors();

void setup(){
  Serial.begin(115200); // Serial port for debugging purposes

  //*uncomment if you are using Pico0 as Access Point
  WiFi.mode(WIFI_AP);
  delay(10);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid_ap, password_ap);
  IPAddress myIP = WiFi.softAPIP();
 
  while (WiFi.status() != WL_CONNECTED) { // Wait for connection
    delay(500);
    Serial.println("Waiting for WiFi connection ...");
  }

  Serial.println("\nConnected to "+WiFi.SSID()); // Report which SSID has been used
  Serial.print("IP address:\t");             
  Serial.println(WiFi.localIP());            // And the IP address being used, although http://ServerLogicalName.local will access the device
  Serial.println("");
  
  
  //server request commands
  server.on("/", handleRoot);
  server.on("/test", [](){ server.send(200, "text/plain", "The server status is OK"); }); //test the server by giving a status response
  server.on("/sensor", handleSensors);   //associate the handler function to the path, note /sensor only refreshes the data
  server.onNotFound(handleNotFound);     //handle when a client requests an unknown URI for example something other than "/")
         
  Serial.println("HTTP server started");
  server.begin();
}

void loop(){
  server.handleClient();   //listen for client connections
  //MDNS.update();
}

// Functions from here...
void handleRoot() {
  server.send(200, "text/plain", "hello from the ESP8266 Sensor Server");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); //send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

//Server request handler
void handleSensors() { 
  //webpage = "";
  Serial.print("Received from: ");
  for (int i = 0; i <= server.args(); i++) {
    Serial.print(server.argName(i)+" "+server.arg(i))+" "; //display received arguments from sensor
    Serial.print(" ");
  }
  Serial.println();
}
