#include <ESP8266WiFi.h>
#include <espnow.h>

// Pin relay motor
const int relayMotorKiri = 5;   // D1
const int relayMotorKanan = 4;  // D2

// Fungsi untuk menjalankan perintah motor
void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "FORWARD") {
    digitalWrite(relayMotorKiri, LOW);
    digitalWrite(relayMotorKanan, LOW);
    Serial.println("FORWARD");

  } else if (cmd == "BACKWARD") {
    digitalWrite(relayMotorKiri, HIGH);
    digitalWrite(relayMotorKanan, HIGH);
    Serial.println("BACKWARD");

  } else if (cmd == "LEFT") {
    digitalWrite(relayMotorKiri, HIGH);
    digitalWrite(relayMotorKanan, LOW);
    Serial.println("LEFT");

  } else if (cmd == "RIGHT") {
    digitalWrite(relayMotorKiri, LOW);
    digitalWrite(relayMotorKanan, HIGH);
    Serial.println("RIGHT");

  } else if (cmd.startsWith("ROTATE:")) {
    String val = cmd.substring(7);
    int angle = val.toInt();
    Serial.print("ROTATE Angle: ");
    Serial.println(angle);
    // Tambah kontrol servo di sini jika perlu

  } else {
    digitalWrite(relayMotorKiri, HIGH);
    digitalWrite(relayMotorKanan, HIGH);
    Serial.println("STOP");
  }
}

// Callback untuk menerima data ESP-NOW
void onReceiveData(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  char buf[100] = {0};
  memcpy(buf, incomingData, len);
  String cmd(buf);
  Serial.print("Received: ");
  Serial.println(cmd);
  handleCommand(cmd);
}

void setup() {
  Serial.begin(9600);

  // Setup pin relay
  pinMode(relayMotorKiri, OUTPUT);
  pinMode(relayMotorKanan, OUTPUT);
  digitalWrite(relayMotorKiri, HIGH);
  digitalWrite(relayMotorKanan, HIGH);

  // Setup WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Inisialisasi ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceiveData);

  Serial.println("ESP-NOW Receiver siap...");
}

void loop() {
  // Tidak perlu apa-apa di loop
}
