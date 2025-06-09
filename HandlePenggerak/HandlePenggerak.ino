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

// --- Buffer pembacaan data serial ---
uint8_t serialBuf[2];
uint8_t serialIndex = 0;

void handleCommand(String cmd, int rot) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Set Servo to: ");
  Serial.println(rot);

  // Debugging: Cek jika nilai rot dalam rentang yang valid untuk servo
  if (rot < 0 || rot > 180) {
    Serial.println("Invalid servo rotation value! Rotating to 90 degrees.");
    rot = 90; // Pastikan servo tidak menerima nilai di luar rentang 0-180
  }
  myServo.write(rot);
  
  if (cmd == "FORWARD") {
    // Logika penghindaran otomatis saat FORWARD
    if (distance < 10 && distance > 0) { // Jika jarak terlalu dekat, berhenti
      autoAvoid = true;
      Serial.println("Obstacles detected! Stopping motor...");

      // Hentikan motor secara eksplisit
      digitalWrite(relayMotorKiri, HIGH); // Matikan motor kiri
      digitalWrite(relayMotorKanan, HIGH); // Matikan motor kanan
      Serial.println("Motor stopped due to obstacle");

      // Logika penghindaran jika ada objek
      if (rot < 45) {
        digitalWrite(relayMotorKiri, LOW);
        digitalWrite(relayMotorKanan, HIGH);  // Belok kiri
        Serial.println("Turning LEFT to avoid obstacle");
      } else if (rot > 135) {
        digitalWrite(relayMotorKiri, HIGH);
        digitalWrite(relayMotorKanan, LOW);  // Belok kanan
        Serial.println("Turning RIGHT to avoid obstacle");
      }

      myServo.write(rot); // Sesuaikan rotasi servo
      autoAvoid = false;  // Mengakhiri penghindaran
    } else {
      // Jika tidak ada rintangan, maju ke depan
      digitalWrite(relayMotorKiri, LOW);
      digitalWrite(relayMotorKanan, LOW);
      Serial.println("FORWARD");
    }
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

// Callback ESP-NOW
void onReceiveData(uint8_t *mac, uint8_t *incoming, uint8_t len) {
  memcpy(&incomingData, incoming, sizeof(incomingData));
  String cmd(incomingData.command);
  int rot = incomingData.rotation;

  // Serial.print("Received Command: ");
  // Serial.println(cmd);
  // Serial.print("Received Rotation: ");
  // Serial.println(rot);

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
  myServo.write(90); // Atur servo ke posisi awal (90 derajat)

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
  while (Serial.available() > 0) {
    distance = Serial.read(); // Ambil jarak dari sensor

    Serial.print("Jarak diterima (int): ");
    Serial.print(distance);
    Serial.println(" cm");
  }
}
