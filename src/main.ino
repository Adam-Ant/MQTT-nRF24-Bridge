#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <ArduinoJson.h>
#include "FS.h"

#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>

const char* ssid = "XXXX";
const char* password = "XXXX";

/***** Configure the chosen CE,CS pins *****/
RF24 radio(4,5);
RF24Network network(radio);
RF24Mesh mesh(radio,network);

ESP8266WebServer http(80);

String webpage = "";


void setup() {
  Serial.begin(115200);

  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  Serial.println(mesh.getNodeID());
  Serial.println("Starting......  ");
  // Connect to the mesh
  mesh.begin();

  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.println(WiFi.status());
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup the web server
  http.on("/clients", [](){
    webpage = "<h1>Connected Clients:</h1><br>";
    for(int i=0; i<mesh.addrListTop; i++){
      webpage += "<b>NodeId: </b>";
      webpage += mesh.addrList[i].nodeID;
      webpage += "    <b>MeshAddress: </b>";
      webpage += mesh.addrList[i].address;
    }
    http.send(200, "text/html", webpage);
  });

  http.serveStatic("/", SPIFFS, "/index.html");

  http.begin();

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

//TODO: Add SPiffs web site. JQuery to jsonify and post. Post JSON data, parse and validate, and store. Call load function to load it into arrays. MQTT message routing. Connected clients on wesbite, js to update


}


void loop() {
  // Handle the HTTP Server
  http.handleClient();

  // Call mesh.update to keep the network updated
  mesh.update();

  // In addition, keep the 'DHCP service' running on the master node so addresses will
  // be assigned to the sensor nodes
  mesh.DHCP();

  // Check for incoming data from the sensors
  if(network.available()){
    RF24NetworkHeader header;
    network.peek(header);

    uint32_t dat=0;
    switch(header.type){
      // Display the incoming millis() values from the sensor nodes
      case 'M': network.read(header,&dat,sizeof(dat)); Serial.println(dat); break;
      default: network.read(header,0,0); Serial.println(header.type);break;
    }
  }
}
