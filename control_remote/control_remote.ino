#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi
const char *ssid = "Udin Tamvan";  // Enter your WiFi name
const char *password = "punyaave";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *mqtt_username = "qwerty";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

String topic = "";

WiFiClient espClient;
PubSubClient client(espClient);

int btnForward = 14;
int btnBackward = 0;
int btnRight = 5;
int btnLeft = 4;
int pot = A0;

void sendCommand(String cmd) {
  if (cmd != topic) { // Hindari spam perintah sama
    client.publish("rc/control", cmd.c_str());
    Serial.println("Kirim: " + cmd);
    topic = cmd;
  }
}

void setup() {
  // put your setup code here, to run once:
  client.loop();
  Serial.begin(9600);
  pinMode(btnForward, INPUT_PULLUP);
  pinMode(btnBackward, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(500);
    }
  }
  // client.subscribe(topic);
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}


void loop() {
  // delay(200);
  // Baca tombol
  if (digitalRead(btnForward) == LOW) {
    sendCommand("FORWARD");
    Serial.println(topic);
  } else if (digitalRead(btnBackward) == LOW) {
    sendCommand("BACKWARD");
    Serial.println(topic);
  } else if (digitalRead(btnLeft) == LOW) {
    sendCommand("LEFT");
    Serial.println(topic);
  } else if (digitalRead(btnRight) == LOW) {
    sendCommand("RIGHT");
    Serial.println(topic);
  }
}
