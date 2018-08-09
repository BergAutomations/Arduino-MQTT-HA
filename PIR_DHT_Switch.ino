/*
HomeAssistant:
Switch:
  - platform: mqtt
    name: "Sensor 1"
    icon: mdi:lightbulb
    state_topic: "/home/sensor1/switch/set"
    command_topic: "/home/sensor1/switch"
    qos: 0
    payload_on: "1"
    payload_off: "0"
    retain: true
Binary PIR Sensor:
  - platform: mqtt  
    state_topic: "home/sensor1/motion"  
    name: "Sensor 1 PIR"  
    payload_on: "on"
    payload_off: "off"
    device_class: motion
Sensor:
  - platform: mqtt  
    state_topic: "home/sensor1/temp"  
    name: "Sensor 1 Temperature"  
    unit_of_measurement: "°C"  
    device_class: temperature
  - platform: mqtt  
    state_topic: "home/sensor1/humidity"  
    name: "Sensor 1 Humidity"  
    unit_of_measurement: "%"   
    device_class: humidity
  - platform: mqtt  
    state_topic: "berg/sensor1/button"  
    name: "Button 1"  

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

void callback(char* topic, byte* payload, unsigned int length);

#define wifi_ssid "your_ssid"
#define wifi_password "wifi_password"

#define MQTT_SERVER "192.168.0.xxxx"  //your MQTT IP Address
#define mqtt_user "mqtt_user" 
#define mqtt_password "mqtt_password"
#define mqtt_port 1883
#define ESP8266Client "ESP8266Client1" //remember to change this for every sensor

#define motion_topic "home/sensor1/motion"
#define button_topic "home/sensor1/button"
#define humidity_topic "home/sensor1/humidity"
#define temp_topic "home/sensor1/temp"

#define motion_pin D5
#define button_pin D1

#define dht_type DHT22
#define dht_pin D7

const int switchPin1 = D6;

char const* switchTopic1 = "home/sensor1/switch";
char const* switchconfirmTopic1 = "home/sensor1/switch/set";

int motionState = 0;
int buttonState = 2;
int buttonCount = 0;

DHT dht(dht_pin, dht_type, 11); // 11 works fine for ESP8266
 
float humidity, temp_f;  // Values read from sensor
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 5000;              // interval at which to read sensor in seconds


WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, mqtt_port, callback, wifiClient);

void setup() {
  Serial.begin(115200);
  dht.begin();           // initialize temperature sensor
  setup_wifi();          // connect to wifi

  //pinMode(0, OUTPUT);
  pinMode(motion_pin, INPUT);
  pinMode(button_pin, INPUT);
  pinMode(switchPin1, OUTPUT); // Relay Switch 1
  digitalWrite(switchPin1, LOW);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ESP8266Client, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(switchTopic1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Read the sensor
  int currentRead = digitalRead(motion_pin);
  // If motion is detected we don't want to 'spam' the service
  if(currentRead != motionState) {
    motionState = currentRead;
    String message = motionState ? "on" : "off";
    Serial.print("PIR Status:");
    Serial.println(String(message).c_str());
    client.publish(motion_topic, String(message).c_str(), true);
  }
  int currentRead2 = digitalRead(button_pin);   
  if(currentRead2 == buttonState && buttonState == 0 && buttonCount < 1) {
    buttonCount = buttonCount + 1;
    String message = "controlOn";
    Serial.print("New Button:");
    Serial.println(String(message + " : " + buttonCount).c_str());
    client.publish(button_topic, String(message).c_str(), true);
  }
  if(currentRead2 != buttonState) {
    buttonState = currentRead2;
    if (buttonCount > 0) {
      buttonCount = 0;
    }
  }

  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    gettemperature();
    String message = String(humidity);
    Serial.print("New Humidity:");
    Serial.println(String(message).c_str());
    client.publish(humidity_topic, String(message).c_str(), true);

    message = String(temp_f);
    Serial.print("New Temp C:");
    Serial.println(String(message).c_str());
    client.publish(temp_topic, String(message).c_str(), true);
  }
}

void gettemperature() {
  humidity = dht.readHumidity();          // Read humidity (percent)
  temp_f = dht.readTemperature();     // Read temperature as Fahrenheit
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temp_f)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(500);
    gettemperature();
    return;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = topic; 
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
     
    if (topicStr == switchTopic1) 
    {

     if (payload[0] == '1'){
       digitalWrite(switchPin1, HIGH);
       client.publish(switchconfirmTopic1, "1");
       }

 else if (payload[0] == '0'){
   digitalWrite(switchPin1, LOW);
   client.publish(switchconfirmTopic1, "0");
   }
 }
 }
