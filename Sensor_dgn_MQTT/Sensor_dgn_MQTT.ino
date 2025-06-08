#include "DHT.h"         // Library untuk sensor DHTxx
#include <ESP8266WiFi.h> // Untuk ESP8266.
#include <PubSubClient.h> // Library MQTT Client

// --- Konfigurasi Wi-Fi ---
const char *ssid = "WARUNG ABANK!";     // Nama Wi-FI
const char *password = "ayamtelor";     // Password wi-Fi

// --- Konfigurasi MQTT Broker ---
const char *mqtt_broker = "broker.emqx.io"; // GANTI DENGAN IP BROKER MQTT ANDA
const char *topic_publish = "71220830"; // Topik untuk mempublikasikan data sensor 
const char *mqtt_username = ""; // KOSONGKAN "" JIKA TIDAK ADA USERNAME/PASSWORD
const char *mqtt_password = ""; // KOSONGKAN "" JIKA TIDAK ADA USERNAME/PASSWORD
const int mqtt_port = 1883;

const char *mqtt_client_id_prefix = "esp8266-sensor-"; // ID unik, akan ditambah MAC address

WiFiClient espClient;
PubSubClient client(espClient);

// --- Konfigurasi Sensor DHTxx ---
#define DHTPIN 2    // D4 (GPIO2) - Pin Digital yang terhubung ke sensor DHT
#define DHTTYPE DHT22 // Tipe sensor DHT yang digunakan (DHT22 atau DHT11)
DHT dht(DHTPIN, DHTTYPE);

// --- Konfigurasi Sensor Ultrasonik HC-SR04 ---
const int trigPin = 5; // D1 (GPIO5) - Pin Trigger sensor
const int echoPin = 4; // D2 (GPIO4) - Pin Echo sensor

// --- Konfigurasi Buzzer ---
const int buzzerPin = 14; // D5 (GPIO14) - Pin Digital yang terhubung ke buzzer

// --- Konfigurasi LDR (Light Dependent Resistor) ---
const int ldrPin = A0; // A0 (GPIO17) - Pin Analog untuk LDR

// --- Konfigurasi LED ---
const int ledPin1 = 12; // D6 (GPIO12) - Pin Digital untuk LED 1
const int ledPin2 = 13; // D7 (GPIO13) - Pin Digital untuk LED 2

// --- Variabel Global ---
long duration; // Durasi pulsa suara untuk sensor HC-SR04
int distance;  // Jarak dalam cm dari sensor HC-SR04
int ldrValue;  // Nilai pembacaan dari LDR (0-1023)

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // Kirim data setiap 1 detik (1000 milidetik)

char jsonBuffer[150]; // Buffer JSON

// --- Variabel untuk Kontrol Database dari Serial (dengan Debouncing) ---
bool sendToDatabase = true; // Default: kirim ke database (1)
int incomingBtnState = -1; // Status tombol terbaru yang diterima dari serial (-1: invalid, 0: off, 1: on)
int lastKnownBtnState = -1; // Status tombol terakhir yang stabil dan diproses
unsigned long lastStateChangeTime = 0; // Waktu terakhir status tombol stabil berubah
const unsigned long serialDebounceDelay = 150; // Debounce delay untuk serial

// --- Fungsi untuk Koneksi Wi-Fi ---
void setup_wifi() {
  delay(10);
  // Serial.print("Menghubungkan ke ");
  // Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }

  // Serial.println("\nWiFi Terhubung");
}

// --- Fungsi untuk Reconnect ke MQTT Broker ---
void reconnect_mqtt() {
  while (!client.connected()) {
    // Serial.print("Mencoba koneksi MQTT...");
    String client_id = mqtt_client_id_prefix;
    client_id += String(WiFi.macAddress()); 

    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      // Serial.println("terhubung");
      int a = 0;
    } else {
      // Serial.print("gagal, rc=");
      // Serial.print(client.state());
      // Serial.println(" coba lagi dalam 5 detik");
      int a = 0;
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600); // Inisialisasi Serial
  // Serial.println("Sistem Sensor ESP8266 Siap!");

  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);     // Pin buzzer sebagai OUTPUT
  digitalWrite(buzzerPin, LOW);   // Buzzer mati saat startup

  pinMode(ledPin1, OUTPUT);       // Pin LED1 sebagai OUTPUT
  digitalWrite(ledPin1, LOW);     // LED1 mati saat startup

  pinMode(ledPin2, OUTPUT);       // Pin LED2 sebagai OUTPUT
  digitalWrite(ledPin2, LOW);     // LED2 mati saat startup

  setup_wifi();
  client.setServer(mqtt_broker, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // --- Baca dan Debounce data dari Serial (dari Control_Remote) ---
  while (Serial.available()) {
    char incomingChar = Serial.read();
    
    if (incomingChar == '0') {
      incomingBtnState = 0;
    } else if (incomingChar == '1') {
      incomingBtnState = 1;
    }
  }

  // Logika debouncing:
  if (incomingBtnState != -1) { 
    if (incomingBtnState != lastKnownBtnState) {
        lastStateChangeTime = millis();
        lastKnownBtnState = incomingBtnState; 
    }

    if ((millis() - lastStateChangeTime) >= serialDebounceDelay) {
        if ((lastKnownBtnState == 0 && sendToDatabase == true) || 
            (lastKnownBtnState == 1 && sendToDatabase == false)) {
            
            sendToDatabase = (lastKnownBtnState == 1);

            // Serial.print("Status DB: ");
            // Serial.print(sendToDatabase ? "Aktif" : "Nonaktif");
            // Serial.print(" (Diterima stabil: ");
            // Serial.print(lastKnownBtnState);
            // Serial.println(")");
        }
    }
  }
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;

    // --- Baca Data Sensor DHT ---
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      h = -1.0;
      t = -1.0;
      // Serial.println(F("Gagal membaca dari sensor DHT!"));
    }

    // --- Baca Data Sensor HC-SR04 ---
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH, 30000);

    if (duration == 0) {
      distance = -1;
      // Serial.println("Error: Tidak ada gema diterima dari sensor Ping.");
    } else {
      distance = duration * 0.034 / 2;
      if (distance > 400 || distance <= 0) {
        distance = -1;
      }
    }

    if (distance != -1 && distance < 10) { 
      digitalWrite(buzzerPin, HIGH); 
      // Serial.println("Jarak kurang dari 10 cm! BUZZER ON!");
    } else {
      digitalWrite(buzzerPin, LOW);  
    }

    // --- Baca Nilai LDR ---
    ldrValue = analogRead(ldrPin);
    Serial.print("Nilai LDR: ");
    Serial.println(ldrValue);

    // --- Kontrol LED berdasarkan nilai LDR ---
    if (ldrValue < 8) { 
      digitalWrite(ledPin1, HIGH); 
      digitalWrite(ledPin2, HIGH); 
      // Serial.println("Kondisi sangat gelap, kedua LED ON.");
    } else { 
      digitalWrite(ledPin1, LOW);  
      digitalWrite(ledPin2, LOW);  
      // Serial.println("Kondisi terang, kedua LED OFF.");
    }

    bool led1State = digitalRead(ledPin1);
    bool led2State = digitalRead(ledPin2);

    // --- Publikasikan Data ke MQTT Broker ---
    snprintf(jsonBuffer, sizeof(jsonBuffer), 
             "{\"temperature\":%.2f,\"humidity\":%.2f,\"distance\":%d,\"ldr\":%d,\"led1Status\":%d,\"led2Status\":%d,\"sendToDB\":%s}", 
             t, h, distance, ldrValue, led1State, led2State, sendToDatabase ? "true" : "false");
    
    if (client.publish(topic_publish, jsonBuffer)) {
      // Serial.print("MQTT JSON (lengkap) terkirim ke ");
      // Serial.print(topic_publish);
      // Serial.print(": ");
      // Serial.println(jsonBuffer);
      int a = 0;
    } else {
      // Serial.println("Gagal mengirim data MQTT.");
      int a = 0;
    }

    // --- Cetak Data Debugging ---
    // Serial.println("--- Debug Output Sensor_dgn_MQTT ---");
    // Serial.print("Suhu: ");
    // Serial.print(t, 2);
    // Serial.println(" C");
    // Serial.print("Kelembaban: ");
    // Serial.print(h, 2);
    // Serial.println(" %");
    // Serial.print("Jarak: ");
    // Serial.print(distance);
    // Serial.println(" cm");
    // Serial.print("LDR: ");
    // Serial.println(ldrValue);
    // Serial.print("LED 1 Status: ");
    // Serial.println(led1State ? "ON" : "OFF");
    // Serial.print("LED 2 Status: ");
    // Serial.println(led2State ? "ON" : "OFF");
    // Serial.print("Status Kirim DB: ");
    // Serial.println(sendToDatabase ? "Aktif" : "Nonaktif");
    // Serial.println("--------------------");

    // --- Kirim jarak sensor HC-SR04 lewat serial TX sebagai 2 byte mentah ---
    Serial.write(distance);
  }
}