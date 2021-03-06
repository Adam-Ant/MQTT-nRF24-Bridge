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

bool saveData() {
  if (http.args() > 0) { // We have some arguments
    bool foundData = false;
    int argPos;

    // Verify the json, and load it into spiffs
    for ( uint8_t i = 0; i < http.args(); i++ ) {
      if (http.argName(i) == "mqtt0") { //TODO - Replace with actual post data field name
        argPos = i; //Log its place to load later
        foundData = true;
      }
    }

    if (!foundData) {
      Serial.println("POST argument not found");
      return false;
    }

    DynamicJsonBuffer jsonBuff;
    JsonObject& json = jsonBuff.parseObject(http.arg(argPos));
    if (!json.success()) {
      // Bad json. Dont write to file
      Serial.println("Bad JSON from web!");
      return false;
    }

    File routingFile = SPIFFS.open("/routes.json", "w");
    if (!routingFile) {
      Serial.println("Failed to open route file file for writing");
      return false;
    }

    json.printTo(routingFile);
    routingFile.close();

    json.prettyPrintTo(Serial);
    // Redirect back to the home page using a get request
    http.sendHeader("Location", "/", true);
    http.send(303, "text/plain", "");
  }

  else {
    Serial.println("No Arguments sent");
    http.send(200, "text/html", "No data"); //TODO -  better status code, or handle this outside of this function?
  }
}


void setup() {
  Serial.begin(115200);

  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  Serial.println(mesh.getNodeID());
  Serial.println("Starting......  ");
  // Connect to the mesh
  mesh.begin();

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

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
  http.on("/post", saveData);
  http.begin();


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
