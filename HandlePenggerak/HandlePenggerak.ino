#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Ganti dengan informasi WiFi dan MQTT kamu
const char* ssid = "Aga";
const char* password = "iniwifiku";

const char *mqtt_broker = "broker.emqx.io";
const char *mqtt_username = "qwerty";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Pin relay
const int relayMotorKiri = D1;
const int relayMotorKanan = D2;

// Fungsi kendali arah
void handleArah(String arah) {
  arah.toUpperCase();
  if (arah == "FORWARD") {
    digitalWrite(relayMotorKiri, HIGH);
    digitalWrite(relayMotorKanan, HIGH);
  } else if (arah == "BACKWARD") {
    digitalWrite(relayMotorKiri, LOW);
    digitalWrite(relayMotorKanan, LOW);
  } else if (arah == "LEFT") {
    digitalWrite(relayMotorKiri, LOW);
    digitalWrite(relayMotorKanan, HIGH);
  } else if (arah == "RIGHT") {
    digitalWrite(relayMotorKiri, HIGH);
    digitalWrite(relayMotorKanan, LOW);
  } else { // STOP atau tidak dikenal
    digitalWrite(relayMotorKiri, LOW);
    digitalWrite(relayMotorKanan, LOW);
  }

  Serial.println("Arah: " + arah);
}

// Callback saat menerima pesan MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String pesan = "";
  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }
  Serial.print("Pesan diterima: ");
  Serial.println(pesan);
  handleArah(pesan);
}

// Reconnect MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP8266_PENGGERAK")) {
      Serial.println("Terhubung");
      client.subscribe("rc/control");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(relayMotorKiri, OUTPUT);
  pinMode(relayMotorKanan, OUTPUT);
  handleArah("STOP");

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
