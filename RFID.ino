#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include "MFRC522.h"
/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = D3
SDA(SS) = D8 
MOSI    = D7
MISO    = D6
SCK     = D5
GND     = GND
3.3V    = 3.3V

RGB LED Setup
Red LED   = D1
Ground    = GND
Gr. LED   = D2
Blue LED  = D0

alarm_control_panel:
  - platform: manual_mqtt
    state_topic: home/alarm
    command_topic: home/alarm/set
    name: Hus Alarm
    code: !secret alarm_password (your password)

sensor:
  - platform: mqtt
    state_topic: "homerfid"
    name: RFID
    pending_time: 15
*/
#define RST_PIN  D3 // RST-PIN GPIO4 
#define SS_PIN  D8  // SDA-PIN GPIO2 
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
// Wifi Connection details
const char* ssid = "your_ssid";
const char* password = "wifi_password";

const char* mqtt_server = "192.168.0.xxxx";
const char* mqtt_user = "mqtt_user";
const char* mqtt_password = "mqtt_password";
const char* clientID = "RFID";
const char* rfid_topic = "home/rfid";
const int redPin = D1;
const int greenPin = D2;
const int idPin = D0;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
void setup() {
  Serial.begin(115200);
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522
  pinMode(redPin, OUTPUT); // Red LED
  pinMode(redPin, HIGH); 
  pinMode(greenPin, OUTPUT); // Green :LED
  pinMode(greenPin, HIGH);
  pinMode(idPin, OUTPUT); // ID :LED
  pinMode(idPin, HIGH);
  delay(2);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
// Connect to Wifi
void setup_wifi() {
  //Turn off Access Point
  WiFi.mode(WIFI_STA);
  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
// Check for incoming messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("");
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  char s[20];
  
  sprintf(s, "%s", payload);

    if ( (strcmp(topic,"home/alarm")==0))
  {
    payload[length] = '\0';
    String sp = String((char*)payload);

  // Alarm is off
    if (sp == "disarmed")
    {
    Serial.println();
    Serial.print("Alarm is set to Disarmed");
    Serial.println();
    off_both_led();    
  }   
  // Alarm is Armed Home    
    else if (sp == "armed_home")
    {
    Serial.println();
    Serial.print("Alarm is set to Armed Home");
    Serial.println();
    on_green_led();     
  } 
  // Alarm is arming
      else if (sp == "pending")
    {
    Serial.println();
    Serial.print("Alarm set to Pending");
    Serial.println();
    on_both_led();    
  }   
  // Alarm is Armed Away
    else if (sp == "armed_away")
    {
    Serial.println();
    Serial.print("Alarm set to Armed Away");
    Serial.println();
    on_green_led();    
  } 
  // Alarm is Triggered
    else if (sp == "triggered")
    {
    Serial.println();
    Serial.print("Alarm is triggered!!");
    Serial.println();
    on_red_led();    
  }
}}
/* interpret the ascii digits in[0] and in[1] as hex
* notation and convert to an integer 0..255.
*/
int hex8(byte *in)
{
   char c, h;

   c = (char)in[0];

   if (c <= '9' && c >= '0') {  c -= '0'; }
   else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
   else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
   else return(-1);

   h = c;

   c = (char)in[1];

   if (c <= '9' && c >= '0') {  c -= '0'; }
   else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
   else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
   else return(-1);

   return ( h<<4 | c);
}
// Reconnect to wifi if connection lost
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("berg/rfid", "connected");
      // ... and resubscribe
      client.subscribe("home/alarm");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
// Main functions
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }
  // Show some details of the PICC (that is: the tag/card)
  Serial.println("");
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  digitalWrite(idPin, HIGH);
  delay(500);
  digitalWrite(idPin, LOW);
  delay(500);
  // Send data to MQTT
  String rfidUid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfidUid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    rfidUid += String(mfrc522.uid.uidByte[i], HEX);
  }
  const char* id = rfidUid.c_str();
  client.publish("berg/rfid", id);
  delay(3000);
}
// LED Loop
void off_both_led(){
  Serial.println("Turning off all led");
  digitalWrite(greenPin, LOW);
  digitalWrite(redPin, LOW);
}
void flash_green_led(){
  Serial.println("Flashing Green LED");
  digitalWrite(greenPin, LOW);
  // Flash 15 times:
  for(int i = 0; i < 14; i++)
  {
  digitalWrite(greenPin, HIGH);
  delay(1000);
  digitalWrite(greenPin, LOW);
  delay(500);
}}
void flash_red_led(){
  Serial.println("Flashing Red LED");
  digitalWrite(redPin, LOW);
  // Flash 20 times:
  for(int i = 0; i < 19; i++)
  {
  digitalWrite(redPin, HIGH);
  delay(1000);
  digitalWrite(redPin, LOW);
  delay(500);
}}
void on_green_led(){
  Serial.println("Turning on green led");
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, HIGH);
}
void on_red_led(){
  Serial.println("Turning on red led");
  digitalWrite(greenPin, LOW);
  digitalWrite(redPin, HIGH);
}
void on_both_led(){
  Serial.println("Turning on both led");
  digitalWrite(greenPin, HIGH);
  digitalWrite(redPin, HIGH);
}
// Helper routine to dump a byte array as hex values to Serial
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

