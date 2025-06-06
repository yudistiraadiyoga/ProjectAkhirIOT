#include "DHT.h"         // Library untuk sensor DHTxx
#include <ESP8266WiFi.h> // Untuk ESP8266. Jika ESP32, gunakan: #include <WiFi.h>
#include <PubSubClient.h> // Library MQTT Client
#include <ArduinoJson.h>  // Library untuk membuat payload JSON

// --- Konfigurasi Wi-Fi ---
const char *ssid = "";    // Diisi nama Wi-Fi
const char *password = "";    // Diisi Password Wifi

// --- Konfigurasi MQTT Broker ---
const char *mqtt_broker = "broker.emqx.io"; // GANTI DENGAN IP BROKER MQTT ANDA (misal "192.168.1.100")
const char *topic_publish = "71220830"; // Topik untuk mempublikasikan data sensor (JSON)
const int mqtt_port = 1883;
const char *mqtt_username = ""; // KOSONGKAN "" JIKA TIDAK ADA USERNAME/PASSWORD
const char *mqtt_password = ""; // KOSONGKAN "" JIKA TIDAK ADA USERNAME/PASSWORD

const char *mqtt_client_id_prefix = "esp8266-sensor-"; // ID unik, akan ditambah MAC address

WiFiClient espClient;
PubSubClient client(espClient);

// --- Konfigurasi Sensor DHTxx ---
#define DHTPIN 2    // D4 (GPIO2) - Pin Digital yang terhubung ke sensor DHT
#define DHTTYPE DHT11 // Tipe sensor DHT yang digunakan (DHT22 atau DHT11)
DHT dht(DHTPIN, DHTTYPE);

// --- Konfigurasi Sensor Ultrasonik HC-SR04 ---
const int trigPin = 5; // D1 (GPIO5) - Pin Trigger sensor
const int echoPin = 4; // D2 (GPIO4) - Pin Echo sensor

// --- Variabel Global ---
long duration; // Durasi pulsa suara untuk sensor HC-SR04
int distance;  // Jarak dalam cm dari sensor HC-SR04

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // Kirim data setiap 1 detik (1000 milidetik)

// --- Fungsi Callback untuk Pesan MQTT yang Diterima (Opsional) ---
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Pesan MQTT diterima di topik: ");
  Serial.println(topic);
  Serial.print("Pesan: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

// --- Fungsi untuk Koneksi Wi-Fi ---
void setup_wifi() {
  delay(10);
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung");
  // Serial.print("Alamat IP: "); // DIHAPUS: Tidak menampilkan IP di Serial Monitor
  // Serial.println(WiFi.localIP()); // DIHAPUS: Tidak menampilkan IP di Serial Monitor
}

// --- Fungsi untuk Reconnect ke MQTT Broker ---
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Mencoba koneksi MQTT...");
    String client_id = mqtt_client_id_prefix;
    client_id += String(WiFi.macAddress()); // TETAP DIGUNAKAN untuk client ID unik

    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("terhubung");
      client.subscribe(topic_subscribe);
    } else {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); 
  Serial.println("Sistem Sensor ESP8266 Siap!");

  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  setup_wifi();
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;

    // --- Baca Data Sensor DHT22 ---
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      h = -1.0;
      t = -1.0;
      Serial.println(F("Gagal membaca dari sensor DHT!"));
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
      Serial.println("Error: Tidak ada gema diterima dari sensor Ping.");
    } else {
      distance = duration * 0.034 / 2;
      if (distance > 400 || distance <= 0) {
        distance = -1;
      }
    }

    // --- Buat Payload JSON untuk MQTT ---
    StaticJsonDocument<200> doc;
    doc["temperature"] = t;
    doc["humidity"] = h;
    doc["distance"] = distance;
    // doc["device_id"] = WiFi.macAddress(); // DIHAPUS: Tidak menyertakan MAC Address di payload JSON

    char jsonBuffer[200];
    serializeJson(doc, jsonBuffer);

    // --- Publikasikan Data ke MQTT Broker ---
    if (client.publish(topic_publish, jsonBuffer)) {
      Serial.print("MQTT JSON terkirim ke ");
      Serial.print(topic_publish);
      Serial.print(": ");
      Serial.println(jsonBuffer); // Tetap menampilkan JSON yang dikirim untuk verifikasi
    } else {
      Serial.println("Gagal mengirim data MQTT.");
    }

    // --- Cetak Data yang Lebih Mudah Dibaca di Serial Monitor (Hanya untuk debugging) ---
    Serial.println("--- Debug Output ---");
    Serial.print("Suhu: ");
    Serial.print(t, 2);
    Serial.println(" C");
    Serial.print("Kelembaban: ");
    Serial.print(h, 2);
    Serial.println(" %");
    Serial.print("Jarak: ");
    Serial.print(distance);
    Serial.println(" cm");
    Serial.println("--------------------");
  }
}
