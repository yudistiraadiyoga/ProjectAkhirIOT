#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

// --- Pin Relay Motor ---
const int relayMotorKiri = 5;   // D1
const int relayMotorKanan = 4;  // D2

// --- Servo ---
const int servoPin = 15;  // D8
Servo myServo;

// --- Struktur ESP-NOW ---
typedef struct struct_message {
  char command[10];
  int rotation;
} struct_message;

struct_message incomingData;

// --- Serial Buffer untuk data jarak ---
String serialBuffer = "";
int lastReceivedDistance = -1;

// --- State penghindaran otomatis ---
bool autoAvoid = false;
bool belokKiri = true;  // Bergantian kiri-kanan

void handleCommand(String cmd, int rot) {
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

  } else {
    digitalWrite(relayMotorKiri, HIGH);
    digitalWrite(relayMotorKanan, HIGH);
    Serial.println("STOP");
  }

  Serial.print("Set Servo to: ");
  Serial.println(rot);
  myServo.write(rot);
}

// Callback ESP-NOW
void onReceiveData(uint8_t *mac, uint8_t *incoming, uint8_t len) {
  memcpy(&incomingData, incoming, sizeof(incomingData));
  String cmd(incomingData.command);
  int rot = incomingData.rotation;

  Serial.print("Received Command: ");
  Serial.println(cmd);
  Serial.print("Received Rotation: ");
  Serial.println(rot);

  // Hanya eksekusi jika tidak dalam mode hindar
  if (!autoAvoid) {
    handleCommand(cmd, rot);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("HandlePenggerak Siap...");

  // Motor
  pinMode(relayMotorKiri, OUTPUT);
  pinMode(relayMotorKanan, OUTPUT);
  digitalWrite(relayMotorKiri, HIGH);
  digitalWrite(relayMotorKanan, HIGH);

  // Servo
  myServo.attach(servoPin);
  myServo.write(90);

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceiveData);
  Serial.println("ESP-NOW Receiver siap...");
}

void loop() {
  // --- Baca jarak dari Serial ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        int dist = serialBuffer.toInt();
        if (dist > 0 && dist < 500) {
          lastReceivedDistance = dist;

          Serial.print("Jarak Diterima: ");
          Serial.print(dist);
          Serial.println(" cm");

          if (dist < 10) {
            autoAvoid = true;

            // Belok kiri atau kanan
            if (belokKiri) {
              digitalWrite(relayMotorKiri, HIGH);
              digitalWrite(relayMotorKanan, LOW);
              Serial.println("AUTO: BEL0K KIRI");
            } else {
              digitalWrite(relayMotorKiri, LOW);
              digitalWrite(relayMotorKanan, HIGH);
              Serial.println("AUTO: BEL0K KANAN");
            }

            belokKiri = !belokKiri;

            delay(500);  // Tunggu sebentar setelah belok
            digitalWrite(relayMotorKiri, HIGH);
            digitalWrite(relayMotorKanan, HIGH);
            Serial.println("AUTO: STOP");
            delay(300);

            autoAvoid = false;  // Kembali ke kontrol ESP-NOW
          }
        }
      }
      serialBuffer = "";  // Reset buffer
    } else {
      serialBuffer += c;  // Tambah ke buffer
    }
  }
}
