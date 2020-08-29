#include "config.h"
#include<ArduinoJson.h>
#include<ESP8266WiFi.h>
#include<PubSubClient.h>

const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 255;

// Real values to write to the LEDs (ex. including brightness and state)
byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;

bool stateOn = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(CONFIG_PIN_RED, OUTPUT);
  pinMode(CONFIG_PIN_GREEN, OUTPUT);
  pinMode(CONFIG_PIN_BLUE, OUTPUT);
  //pinMode(CONFIG_PIN_WHITE, OUTPUT);

  analogWriteRange(255);

  Serial.begin(9600);

  setupWifi();
  client.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT);
  client.setCallback(callback);
}

void setupWifi(){
  delay(10);

  Serial.println();
  Serial.print("Connect to ");
  Serial.print(CONFIG_WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/*
SAMPLE PAYLOAD (RGBW):
    {
      "brightness": 250,
      "color": {
        "r": 255,
        "g": 100,
        "b": 100
      },
      "state": "ON"
    }
*/

void callback(char* topic, byte* payload, unsigned int length){
  Serial.println("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for(int i = 0; i < length; i++){
    message[i] = (char)payload[i];
  }

  message[length] = '\0';
  Serial.println(message);

  if(!processJson(message)){
    return;
  }

  if(stateOn){
    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
  }
  else{
    realRed = 0;
    realGreen = 0;
    realBlue = 0;
  }

  setColor(realRed, realGreen, realBlue);
}

bool processJson(char* message){
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(message);

  if(!root.success()){
    Serial.println("parseObject() failed");
    return false;
  }
  Serial.println("Parse object");
  if(root.containsKey("state")){
    if(strcmp(root["state"], "ON") == 0)
      stateOn = true;
    else if(strcmp(root["state"], "OFF") == 0)
      stateOn = false;
  }

  if(root.containsKey("brightness"))
    brightness = root["brightness"];

  if(root.containsKey("color")){
    red = root["color"]["r"];
    green = root["color"]["g"];
    blue = root["color"]["b"];
  }

  return true;
}

void reconnect(){
  //Loop until we're reconnected
  while(!client.connected()){
    Serial.print("Attempting MQTT connection...");
    //Attemp to connect
    if(client.connect(CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)){
      Serial.println("connected");
      client.subscribe(CONFIG_MQTT_TOPIC);
    }
    else{
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setColor(int r, int g, int b){
  Serial.println("Set color");
  analogWrite(CONFIG_PIN_RED, r);
  analogWrite(CONFIG_PIN_GREEN, g);
  analogWrite(CONFIG_PIN_BLUE, b);
}

void loop() {
  if(!client.connected()){
    reconnect();
  }

  client.loop();
}
