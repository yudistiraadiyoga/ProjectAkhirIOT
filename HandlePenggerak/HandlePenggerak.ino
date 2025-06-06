#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

// Pin relay motor
const int relayMotorKiri = 5;   // D1
const int relayMotorKanan = 4;  // D2

// Servo pin
const int servoPin = 15;  // D8
Servo myServo;

// Struktur data dari control_remote
typedef struct struct_message {
  char command[10];
  int rotation;
  int btnForwardState;
  int btnMonitoringState;
  int btnLeftState;
  int btnRightState;
} struct_message;

struct_message incomingData;

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

  // Proses motor & servo
  handleCommand(cmd, rot);

  // Kirim status monitoring ke Sensor_dgn_MQTT melalui UART
  if (incomingData.btnMonitoringState == LOW) {
    Serial.println("1");  // Tombol monitoring ditekan
  } else {
    Serial.println("0");  // Tidak ditekan
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(relayMotorKiri, OUTPUT);
  pinMode(relayMotorKanan, OUTPUT);
  digitalWrite(relayMotorKiri, HIGH);
  digitalWrite(relayMotorKanan, HIGH);

  myServo.attach(servoPin);
  myServo.write(90);  // Servo awal

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
  // Kosong
}
