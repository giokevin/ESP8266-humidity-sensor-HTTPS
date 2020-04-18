
/*
  HelloServerBearSSL - Simple HTTPS server example

  This example demonstrates a basic ESP8266WebServerSecure HTTPS server
  that can serve "/" and "/inline" and generate detailed 404 (not found)
  HTTP respoinses.  Be sure to update the SSID and PASSWORD before running
  to allow connection to your WiFi network.

  Adapted by Earle F. Philhower, III, from the HelloServer.ino example.
  This example is released into the public domain.
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID ""
#define STAPSK  ""
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const char* www_realm = "";
const char* www_username = "";
const char* www_password = "";
String authFailResponse = "Authentication Failed";

int WiFiStrength = 0;
double analogValue = 0.0;
double analogVolts = 0.0;

BearSSL::ESP8266WebServerSecure server(443);

static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
// Your server certificate
-----END CERTIFICATE-----

)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----
// Your server key
-----END RSA PRIVATE KEY-----
)EOF";


void handleRoot() {
  
    if (!server.authenticate(www_username, www_password))
  {
    return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
  }

  WiFiStrength = WiFi.RSSI(); // get dBm from the ESP8266
  analogValue = analogRead(A0); // read the analog signal

  // convert the analog signal to voltage
  // the ESP2866 A0 reads between 0 and ~3 volts, producing a corresponding value
  // between 0 and 1024. The equation below will convert the value to a voltage value.
 
  analogVolts = (analogValue * 3.08) / 1024;

  // now get our chart value by converting the analog (0-1024) value to a value between 0 and 100.
  // the value of 400 was determined by using a dry moisture sensor (not in soil, just in air).
  // When dry, the moisture sensor value was approximately 400. This value might need adjustment
  // for fine tuning of the chartValue.

  int chartValue = ((analogValue -369 ) * 100) / 400;

  // now reverse the value so that the value goes up as moisture increases
  // the raw value goes down with wetness, we want our chart to go up with wetness
  chartValue = 100 - chartValue;

  // set a "blink" time interval in milliseconds.
  // for example, 15000 is 15 seconds. However, the blink will not always be 15 seconds due to other
  // delays set in the code.

  // Serial data
  Serial.print("Analog raw: ");
  Serial.println(analogValue);
  Serial.print("Analog V: ");
  Serial.println(analogVolts);
  Serial.print("ChartValue: ");
  Serial.println(chartValue);
  Serial.print("millis(): ");
  Serial.println(millis());
  Serial.print("WiFi Strength: ");
  Serial.print(WiFiStrength); Serial.println("dBm");
  Serial.println(" ");

  const size_t capacity = JSON_OBJECT_SIZE(1);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.createObject();
  root["analograw"] = analogValue;
  root["analogvolts"] = analogVolts;
  root["humidity_percent"] = chartValue;
  root["wifistrength_dbm"] = WiFiStrength;

  String json;
  root.prettyPrintTo(json);
  
  server.send(200, "text/json", json);
  
}

void setup(void){
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTPS server started");
}

void loop(void){
  server.handleClient();
  MDNS.update();
}
