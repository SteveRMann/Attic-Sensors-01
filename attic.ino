/*****
  E:\River Documents\SBC (Arduino, etc)\ESP8266\Projects\MQTT nodes\attic-143
 
 All the resources for this project:
 http://randomnerdtutorials.com/
 
*****/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#define DHTTYPE DHT11   // DHT 11

const char* ssid = "Kaywinnet";
const char* password = "806194edb8";
const char* mqtt_server = "192.168.1.124";

// Change the name assigned to the WiFiClient object to be unique.
WiFiClient espAttic;
PubSubClient client(espAttic);

const int DHTPin = 0;
const int LightSensor = 2;
int val = 0;

DHT dht(DHTPin, DHTTYPE);     // Initialize DHT sensor.

// Timer variables
long now = millis();
long lastMeasure = 0;

//Global variables
static char DHT_Humidity[7];
static char DHT_Temperature[7];
static char DHT_HeatIndex[7];




void static_ip() {
  // config static IP
  IPAddress ip(192, 168, 1, 143);
  IPAddress gateway(192, 168, 1, 1);
  Serial.print(F("Setting static ip to : "));
  Serial.println(ip);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet);
}	

// This function connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

  
// This functions reconnects your ESP8266 to your MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP8266Attic")) {    //Every ESP needs a distinct device ID.
      Serial.println("connected");  
    } 
  }
}



bool readDHT(){
  
    //Read the sensor
    float h = dht.readHumidity();
    float t = dht.readTemperature();      // Read temperature as Celsius (the default)
    float f = dht.readTemperature(true);  // Read temperature as Fahrenheit (isFahrenheit = true)

    delay(100);   //Give the sensor some time...

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return false;
    }

    dtostrf(h, 6, 2, DHT_Humidity);     //Convert double to 6-character String, 2 chrs after the decimal point.
    dtostrf(f, 6, 2, DHT_Temperature);

    /* Calculate the heat indexes, fahrenheit and celcius */
    //float heatIndex_C = dht.computeHeatIndex(t, h, false);  // Computes Heat Index temperature value in Celsius. (isFahreheit = false)
    //dtostrf(heatIndex_C, 6, 2, DHT_Temperature);  //Convert double to 6-character String, 2 chrs after the decimal point.
    
    float heatIndex_F = dht.computeHeatIndex(f, h);  // Computes Heat Index temperature value in Fahrenheit
    dtostrf(heatIndex_F, 6, 2, DHT_HeatIndex);

    return true;

}




void setup() {
  //pinMode(LightSensor, INPUT_PULLUP);
  //pinMode(LightSensor, OUTPUT);
  //digitalWrite(LightSensor, LOW);
  pinMode(LightSensor, INPUT);
  dht.begin();
  
  Serial.begin(115200);
  static_ip();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

/* *********************************************************************
 * Ensure that you ESP is connected to your broker, then process the inputs.
*/
 
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Attic");

  now = millis();
  if (now - lastMeasure > 30000) {   
    lastMeasure = now;

   /*
   * *******************************************************************
   * Check the light sensor on GPIO2
   * Sensor is a photoresistor on an LM303 comparator on pin 3 (GPIO2).
   * When dark, the photoresistor is near infinity (ohms) and GPIO2 is high.
   * When the room light is on, the photoresistor is 1K ohms or less, pulling the GPIO2 pin low.
   * 
   */
   
   val = digitalRead(LightSensor);
   if (val == HIGH) {       //Dark
     // Light is off:
     client.publish("attic/light_status", "off");
     Serial.println("Light is off\n");
     
   } else {                         //Sensor is low
     // Light is on.  Photoresistor is Lo-Z, GPIO2 is pulled down.
     client.publish("attic/light_status", "on");
     Serial.println("Light is on\n");
   }


    if (readDHT()){
      client.publish("attic/heatindex", DHT_HeatIndex);
      client.publish("attic/humidity", DHT_Humidity);
      client.publish("attic/temperature", DHT_Temperature);
      Serial.print("Attic Temperature is ");
      Serial.print(DHT_Temperature);
      Serial.println("\n");
    }

  }

}
