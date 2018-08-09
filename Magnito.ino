#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//WIFI
const char* wifi_ssid = "your_ssid";
const char* wifi_password = "wifi_password";

//MQTT
const char* mqtt_server = "192.168.0.xxxx";
const char* mqtt_user = "mqtt_user";
const char* mqtt_password = "mqtt_password";
const char* clientID = "Magnito";

//VARS
const char* magnito_topic = "home/doorbell/magnito";
const int digital = D8;
int magnitoState = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(digital, INPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  //Turn off Access Point
  WiFi.mode(WIFI_STA);
  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println(" ");
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  Serial.print("Status: ");

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
    Serial.print("Connecting ");
    Serial.print(clientID);
    Serial.println(" to MQTT");
    // Attempt to connect
    if (client.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.print("MQTT connected to ");
      Serial.println(mqtt_server);
      client.publish(magnito_topic, "Magnito connected to MQTT");
      client.publish(magnito_topic, "off", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  magnitoState = digitalRead(digital);

  if ( magnitoState == HIGH ) {
    // Put your code here.  e.g. connect, send, disconnect.
    Serial.println("Magnito is activated!");
    client.publish(magnito_topic, "on", false);

    delay( 2000 );
    client.publish(magnito_topic, "off", false);
  }
}
