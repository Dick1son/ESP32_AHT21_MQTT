#include <Adafruit_AHTX0.h>
#include "ESP32_WiFi_MQTT_Manager.h"
#include "time.h"

Adafruit_AHTX0 aht;
String msg;
sensors_event_t humidity, temp;
char ntpTime[100] = {0};

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
  }
  else{
    Serial.println("AHT10 or AHT20 found");
  }

  initLittleFS();
  
  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile (LittleFS, gatewayPath);
  mqttIP = readFile (LittleFS, mqttIPPath);
  mqttUsername = readFile (LittleFS, mqttUsernamePath);
  mqttPassword = readFile (LittleFS, mqttPasswordPath);

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);
  Serial.println(mqttIP);
  Serial.println(mqttUsername);
  Serial.println(mqttPassword);
  Serial.println(" ");

  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html");
    });
    server.serveStatic("/", LittleFS, "/");
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i = 0; i < params; i++){
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_5) {
            mqttIP = p->value().c_str();
            Serial.print("MQTT Server IP Address set to: ");
            Serial.println(mqttIP);
            // Write file to save value
            writeFile(LittleFS, mqttIPPath, mqttIP.c_str());
          }
          if (p->name() == PARAM_INPUT_6) {
            mqttUsername = p->value().c_str();
            Serial.print("Username for MQTT set to: ");
            Serial.println(mqttUsername);
            // Write file to save value
            writeFile(LittleFS, mqttUsernamePath, mqttUsername.c_str());
          }
          if (p->name() == PARAM_INPUT_7) {
            mqttPassword = p->value().c_str();
            Serial.print("Password for MQTT Server set to: ");
            Serial.println(mqttPassword);
            // Write file to save value
            writeFile(LittleFS, mqttPasswordPath, mqttPassword.c_str());
          }
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

void loop() {
  if (!client.connected()) {
    reconnect(mqttUsername, mqttPassword);
  }

  client.loop();

  if(getTime()){
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data

    msg = String(ntpTime) + " -> " + String(temp.temperature, 0) + "°C";

    //Publish an MQTT message on topic aht21/msg
    client.publish(MQTT_PUB_MSG, msg.c_str());                        
    Serial.println(msg);
  }
  
  delay(60000);
}

bool getTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return 0;
  }
  strftime(ntpTime, sizeof(ntpTime), "%A, %B %d %Y %H:%M:%S", &timeinfo);

  return 1;
}