#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

int distance;

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
  int btnForwardState;
  int btnMonitoringState;
  int btnLeftState;
  int btnRightState;
} struct_message;

struct_message incomingData;

// --- State penghindaran otomatis ---
bool autoAvoid = false;
bool belokKiri = true;

// --- Buffer pembacaan data serial ---
uint8_t serialBuf[2];
uint8_t serialIndex = 0;

void handleCommand(String cmd, int rot) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Set Servo to: ");
  Serial.println(rot);
  myServo.write(rot);
  if (rot){
    myServo.write(rot);
    if (rot < 45){
      digitalWrite(relayMotorKiri, HIGH);
      digitalWrite(relayMotorKanan, LOW);
      Serial.println("LEFT");
    } else if (rot > 135){
      digitalWrite(relayMotorKiri, LOW);
      digitalWrite(relayMotorKanan, HIGH);
      Serial.println("RIGHT");
    } else {
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
    }
  }
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

  if (!autoAvoid) {
    handleCommand(cmd, rot);
  }

  if (incomingData.btnMonitoringState == LOW) {
    Serial.println("1");  // Tombol monitoring ditekan
  } else {
    Serial.println("0");  // Tidak ditekan
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
  // Baca data jarak dari Serial (int dikirim sebagai 2 byte)
  while (Serial.available() >= 2) {
    distance = Serial.read();

    Serial.print("Jarak diterima (int): ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance < 10 && distance > 0) {
      autoAvoid = true;

      // Belok kiri atau kanan
      if (belokKiri) {
        digitalWrite(relayMotorKiri, HIGH);
        digitalWrite(relayMotorKanan, LOW);
        Serial.println("AUTO: BELOK KIRI");
      } else {
        digitalWrite(relayMotorKiri, LOW);
        digitalWrite(relayMotorKanan, HIGH);
        Serial.println("AUTO: BELOK KANAN");
      }

      belokKiri = !belokKiri;

      delay(500);
      digitalWrite(relayMotorKiri, HIGH);
      digitalWrite(relayMotorKanan, HIGH);
      Serial.println("AUTO: STOP");
      delay(300);

      autoAvoid = false;
    }
  }
}
