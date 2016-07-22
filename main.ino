
//nRF24L01 - ESP8266 12
//1 GRND
//2 VCC
//3 CE   - GPIO4
//4 CSN  - GPIO15
//5 SCK  - GPIO14
//6 MOSI - GPIO13
//7 MISO - GPIO12
//8 IRQ  - NC
//
//nRF24L01 - MSP430 Launchpad
//3 CE   - P2.0:
//4 CSN  - P2.1:
//5 SCK  - P1.5:
//6 MOSi - P1.7:
//7 MISO - P1.6:
//8 IRQ  - P2.2:


#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <Hash.h>

#define DEVICE_ID 2
#define CHANNEL 120 //MAX 127

// SPI pin configuration figured out from here:
// http://d.av.id.au/blog/esp8266-hardware-spi-hspi-general-info-and-pinout/
RF24 radio(4, 15);    // Set up nRF24L01 radio on SPI bus plus pins 4 for CE and 15 for CSN

// Topology
const uint64_t pipes[2] = { 0xDEADBEEF01LL, 0xFFFFFFFFFFLL };
const char* ssid = "";     // your network SSID (name)
const char* ssid1 = "ESP_telemetry";     // your network SSID (name)
const char* password = "";  // your network password
const char *str_change = "CHANGE";

const String key = "some key";
String TempMeasurements[12], token, SHA1hash[3], sessionID;
String PresMeasurements[12];
String HumMeasurements[12];
String printTempMeasurements, printHumMeasurements, printPresMeasurements;
String Temperature, Pressure, Humitiy;
String users [3] = {"User1", "User2", "Guest"};
int x = 0, h = 0, i = 0, k = 0, j = 0, f = 0, storeTempTelmetry = 11, storePresTelmetry = 11, storeHumTelmetry = 11;
int retry = 0;
char inbuf[6];

ESP8266WebServer server(80);

// Onboard LED pin
const int led = LED_BUILTIN;
const int relay = 16;


void handleRoot() {
  String message = "Home Telemetry";
  message += "\n\n";

  message += "ESP_ID = ";
  message += ESP.getChipId();
  message += "\n";

  message += "Restart reason = ";
  message += ESP.getResetReason();
  message += "\n";

  message += "Free Heap = ";
  message += ESP.getFreeHeap();
  message += "\n";

  byte numSsid = WiFi.scanNetworks();
  message += "SSID = ";
  message += WiFi.SSID();
  message += "\n";

  long rssi = WiFi.RSSI();
  message += "RSSI = ";
  message += rssi;
  message += "\n";

  message += "IP = ";
  message += WiFi.localIP();
  message += "\n";

  message += "\n";
  message += "Key = ";
  message += key;
  message += "\n";
  message += "Token = ";
  message += sessionID;
  message += "Hash(Token + key + uname) : ";
  message += "\n";
  message += SHA1hash[0];
  message += "\n";
  message += SHA1hash[1];
  message += "\n";
  message += SHA1hash[2];
  message += "\n";
  message += users[0];
  message += "\n";
  message += users[1];
  message += "\n";
  message += users[2];

  server.send(200, "text/plain", message);
  //digitalWrite(led, HIGH);
}

void handleNotFound() {
  //digitalWrite(led, LOW);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  //digitalWrite(led, HIGH);
}

void setup() {
  randomSeed(analogRead(0));
  yield();
  SPI.setHwCs(true);
  pinMode(led, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  digitalWrite(led, HIGH);

  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("starting......");
  restartRadio();                    // turn on and configure radio
  Serial.println("restarting radio");
  radio.startListening();
  radio.printDetails();
  Serial.println("Listening for sensor values...");

  WiFi.begin(ssid, password);
  Serial.setDebugOutput(true);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (retry == 60) {
      WiFi.disconnect();
      WiFi.softAP(ssid1, password);
      break;
    }
    retry ++;
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("espHTserver")) {
    Serial.println("MDNS responder started");
  }

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"1400");

  server.on("/", handleRoot);

  server.on("/balkony_ligh", []() {
    digitalWrite(relay, !digitalRead(relay));
    String message = "{";
    message += "\n\"Lights\": {";
    message += "\n\"Light_status\": { \"-balkony\": \"";
    message += (String) !digitalRead(relay);
    message += "\"}";
    message += "\n }";
    message += "\n}";
    server.send(200, "text/json", message);

  });

  server.on("/light_status", []() {
    String message = "{";
    message += "\n\"Lights\": {";
    message += "\n\"Light_status\": { \"-balkony\": \"";
    message += (String) !digitalRead(relay);
    message += "\"}";
    message += "\n }";
    message += "\n}";
    server.send(200, "text/json", message);

  });

  server.on("/wifi_connect", []() {
    WiFi.begin(ssid, password);
  });

  server.on("/in_case_of_zombie_apocalypse_hit_this", []() {
    ESP.restart();
  });

  server.on("/try", []() {
    String message;

    if (server.arg(0) == SHA1hash[0] | server.arg(0) == SHA1hash[1]) {
      message = "full access";
      server.send(200, "text/plain", message);
    }
    else if (server.arg(0) == SHA1hash[2]) {
      message = "telemetry only";
      server.send(200, "text/plain", message);
    }
    else {
      message = "wrong session Id";
      server.send(404, "text/plain", message);
    }


  });

  server.on("/get_token", []() {
    if (server.arg(0) == users[0]) {

      sessionID  = String(random(65535));
      SHA1hash[0] = sha1(sessionID + key + server.arg(0));
      String message = "{";
      message += "\n\"Token\": {";
      message += "\n\"Session_ID\": { \"-ID\": \"";
      message += sessionID;
      message += "\"}";
      message += "\n }";
      message += "\n}";
      server.send(200, "text/json", message);

    } else if (server.arg(0) == users[1]) {

      sessionID = String(random(65535));
      SHA1hash[1] = sha1(sessionID + key + server.arg(0));
      String message = "{";
      message += "\n\"Token\": {";
      message += "\n\"Session_ID\": { \"-ID\": \"";
      message += sessionID;
      message += "\"}";
      message += "\n }";
      message += "\n}";
      server.send(200, "text/json", message);

    } else if (server.arg(0) == users[2]) {
      sessionID = String(random(65535));
      SHA1hash[2] = sha1(sessionID + key + server.arg(0));
      String message = "{";
      message += "\n\"Token\": {";
      message += "\n\"Session_ID\": { \"-ID\": \"";
      message += sessionID;
      message += "\"}";
      message += "\n }";
      message += "\n}";
      server.send(200, "text/json", message);
    } else {
      String message = "Username is not in database";
      server.send(404, "text/plain", message);
    }
  });

  server.on("/telemetry", []() {

    printTempMeasurements = "";
    for (k = 0; k < 12; k++) {
      printTempMeasurements += '"' + TempMeasurements[k] + '"';
      if (k < 11) {
        printTempMeasurements += ",";
      }
    }

    printPresMeasurements = "";
    for (f = 0; f < 12; f++) {
      printPresMeasurements += '"' + PresMeasurements[f] + '"';
      if (f < 11) {
        printPresMeasurements +=  ",";
      }
    }

    printHumMeasurements = "";
    for (x = 0; x < 12; x++) {
      printHumMeasurements += '"' + HumMeasurements[x] + '"';
      if (x < 11) {
        printHumMeasurements +=  ",";
      }
    }


    String message = "{";
    message += "\n\"Home_Telemetry\": {";
    message += "\n\"Temperature\": { \"-outside\": \"";
    message += Temperature;
    message += "\",";
    message += "\n\"-index\": ";
    message += i;
    message += ",";
    message += "\n\"-Last12Hours\": [";
    message += printTempMeasurements;
    message += "]},";
    message += "\n\"Humidity\": { \"-outside\": \"";
    message += Humitiy;
    message += "\",";
    message += "\n\"-index\": ";
    message += h;
    message += ",";
    message += "\n\"-Last12Hours\": [";
    message += printHumMeasurements;
    message += "]},";
    message += "\n\"Pressure\": { \"-outside\": \"";
    message += Pressure;
    message += "\",";
    message += "\n\"-index\": ";
    message += j;
    message += ",";
    message += "\n\"-Last12Hours\": [";
    message += printPresMeasurements;
    message += "]}";
    message += "\n }";
    message += "\n}";
    server.send(200, "text/json", message);

  });

  server.onNotFound(handleNotFound);

  server.begin();

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("HTTP server started");

}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  if (radio.available()) {
    while (radio.available()) {
      yield();
      radio.read(inbuf, sizeof(inbuf));
      Serial.print("Received packet: ");
      Serial.println(inbuf);
      if (!strcmp(inbuf, str_change)) {
        digitalWrite(relay, !digitalRead(relay));
      }
      if (String(inbuf).startsWith("T") == true)
      {

        Temperature = String(inbuf);
        Temperature.replace("T", "");
        Temperature.remove(4);
        storeTempTelmetry++;
        if (storeTempTelmetry == 12) {
          storeTempTelmetry = 0;
          i++;
          if (i == 12) {
            i = 0;
          }
          TempMeasurements[i] = Temperature;
        }

        Serial.println("Temp =" +  Temperature);


      }

      if (String(inbuf).startsWith("H") == true)
      {

        Humitiy = String(inbuf);
        Humitiy.replace("H", "");
        Humitiy.remove(2);
        storeHumTelmetry++;
        if (storeHumTelmetry == 12) {
          storeHumTelmetry = 0;
          h++;
          if (h == 12) {
            h = 0;
          }
          HumMeasurements[h] = Humitiy;
        }

        Serial.println("Humitiy =" +  Humitiy);


      }
      if (String(inbuf).startsWith("P") == true)
      {

        Pressure = String(inbuf);
        Pressure.replace("P", "");
        Pressure.remove(3);

        storePresTelmetry++;
        if (storePresTelmetry == 12) {
          storePresTelmetry = 0;
          j++;
          if (j == 12) {
            j = 0;
          }
          PresMeasurements[j] = Pressure;
        }

        Serial.println("Presure =" + Pressure);


      }
    }
  }
}

void restartRadio() {
  yield();
  radio.begin(); // Start up the radio
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.setCRCLength(RF24_CRC_8);
  //radio.setAutoAck(false);
  //radio.disableCRC();
  radio.setChannel(CHANNEL);
  radio.enableDynamicPayloads();
  //radio.openReadingPipe(1,pipes[1]);
  //radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[0]);
  radio.openWritingPipe(pipes[1]);
  radio.stopListening();
}


