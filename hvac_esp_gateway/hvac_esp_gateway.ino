#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "secrets.h"
#include "page.h"

SoftwareSerial link(D5, D6);
ESP8266WebServer server(80);

static char buf[64];
static uint8_t idx = 0;
static bool clean = true;

struct Telem {
  long room;
  long duct;
  int  state;
  long set;
  bool valid;        
};
static Telem latest = { 0, 0, 0, 0, false };

void handleData() {
  if (!latest.valid) {
    server.send(503, "application/json", "{}");
    return;
  }
  char json[96];
  snprintf(json, sizeof(json), "{\"room\":%ld,\"duct\":%ld,\"state\":%d,\"set\":%ld}", latest.room, latest.duct, latest.state, latest.set);
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void setup() {
  Serial.begin(115200);
  link.begin(9600);
  Serial.println("ALIVE\r\n");

  WiFi.mode(WIFI_STA); 
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/data", handleData);
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  while(link.available()) {
    char c = link.read(); // Read data

    // Write data into JSON, throw away data if idx counts too high
    if(c == '\n') {
      if(clean) {
        buf[idx] = '\0';

        long r, d, s;
        int  st;
        if(sscanf(buf, "T %ld, %ld, %d, %ld", &r, &d, &st, &s) == 4) {
          latest.room  = r;
          latest.duct  = d;
          latest.state = st;
          latest.set   = s;
          latest.valid = true;
          Serial.printf("Stats %ld, %ld, %d, %ld\n", latest.room, latest.duct, latest.state, latest.set);
        }
      }
      idx = 0;
      clean = true;
    } else {
      if(idx >= (sizeof(buf) - 1)) {
        clean = false;
      }
      else {
        buf[idx++] = c;
      }
    }
  }
  server.handleClient();
}
