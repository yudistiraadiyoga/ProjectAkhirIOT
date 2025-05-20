#include "DHT.h" // Library untuk sensor DHTxx

// --- Konfigurasi Sensor DHTxx ---
#define DHTPIN 2    // Pin Digital yang terhubung ke sensor DHT (Contoh: pin 2 di Arduino Uno/Nano)
#define DHTTYPE DHT22 // Tipe sensor DHT yang digunakan (DHT22 atau DHT11)
DHT dht(DHTPIN, DHTTYPE);

// --- Konfigurasi Sensor Ultrasonik HC-SR04 ---
const int trigPin = 13; // Pin Digital untuk Trigger (Contoh: pin 13 di Arduino Uno/Nano)
const int echoPin = 12; // Pin Digital untuk Echo (Contoh: pin 12 di Arduino Uno/Nano)

// --- Variabel Global ---
long duration; // Durasi pulsa suara untuk sensor HC-SR04
int distance;  // Jarak dalam cm dari sensor HC-SR04

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // Kirim data setiap 5 detik (dalam milidetik)

void setup() {
  Serial.begin(9600); // Inisialisasi komunikasi Serial dengan baud rate 9600
                      // PENTING: Baud rate ini HARUS SAMA dengan yang diatur di Node-RED Serial In node
  Serial.println("Arduino Sensors Ready!");

  dht.begin(); // Memulai sensor DHT
  
  // Mengatur pin sensor HC-SR04
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
}

void loop() {
  unsigned long currentMillis = millis();

  // Kirim data hanya jika sudah melewati interval waktu yang ditentukan
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis; // Update waktu terakhir pengiriman

    // --- Baca Data Sensor DHT22 ---
    float h = dht.readHumidity();      // Membaca kelembaban
    float t = dht.readTemperature();   // Membaca suhu dalam Celsius

    // Tangani error pembacaan DHT
    if (isnan(h) || isnan(t)) {
      h = -1; // Menandakan error atau pembacaan tidak valid
      t = -1; // Menandakan error atau pembacaan tidak valid
      Serial.println(F("Failed to read from DHT sensor!"));
    }

    // --- Baca Data Sensor HC-SR04 ---
    // Membersihkan trigPin (mengatur LOW selama 2 mikrodetik)
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Mengatur trigPin HIGH selama 10 mikrodetik untuk mengirim pulsa
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Membaca durasi pulsa pada echoPin. Timeout 30000 mikrodetik (30ms) untuk jarak maksimal ~5m.
    duration = pulseIn(echoPin, HIGH, 30000); 

    // Jika tidak ada gema atau timeout
    if (duration == 0) {
      distance = -1; // Menandakan error atau out of range
      Serial.println("Error: No echo received for Ping sensor.");
    } else {
      // Menghitung jarak: speed of sound (0.034 cm/us) * duration / 2
      distance = duration * 0.034 / 2;
      // Batasi jarak ke 400 cm atau filter nilai negatif/nol
      if (distance > 400 || distance <= 0) { 
          distance = -1; // Tandai sebagai tidak valid jika di luar jangkauan yang wajar
      }
    }

    // --- Kirim Data Gabungan ke Serial Port dalam Format JSON ---
    // Format JSON yang akan dikirim: {"temperature":XX.X,"humidity":YY.Y,"distance":ZZ}
    // Ini adalah string JSON yang dibuat manual, karena library ArduinoJson tidak digunakan untuk menghemat memori.
    Serial.print("{\"temperature\":");
    Serial.print(t); // Suhu
    Serial.print(",\"humidity\":");
    Serial.print(h); // Kelembaban
    Serial.print(",\"distance\":");
    Serial.print(distance); // Jarak
    Serial.println("}"); // Menambahkan newline (delimiter) di akhir pesan
  }
  // Tidak ada delay() di akhir loop, karena interval diatur oleh lastSendTime dan sendInterval
}