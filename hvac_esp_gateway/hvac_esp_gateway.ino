#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "secrets.h"

SoftwareSerial link(D5, D6);

void setup() {
  Serial.begin(115200);
  link.begin(9600);
  Serial.println("ALIVE/r/n");
}

void loop() {
  while(link.available()) {
    uint8_t byte = link.read();
    Serial.write(byte);
  }
}
