#include <espnow.h>
#include <ESP8266WiFi.h>

// Slave MAC Address
uint8_t mac_slave[] = {0xF4, 0xCF, 0xA2, 0x75, 0x64, 0x0E};  // Slave MAC address

// Struktur data yang akan dikirim
typedef struct struct_message {
  char command[10];  // Perintah kontrol (FORWARD, LEFT, RIGHT, STOP, MONITORING)
  int rotation;      // Nilai rotasi dari potensiometer
  int btnForwardState; // Status tombol Forward
  int btnMonitoringState; // Status tombol Monitoring
  int btnLeftState; // Status tombol Left
  int btnRightState; // Status tombol Right
} struct_message;

struct_message myData;  // Data yang akan dikirim

int btnForward = 5;
int btnMonitoring = 4;
int btnRight = 0;
int btnLeft = 2;
int pot = A0;

void OnDataSent(uint8_t *mac_addr, uint8_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  if (status == 0) {
    Serial.println("Delivery Success");
  } else {
    Serial.print("Delivery Fail with status code: ");
    Serial.println(status);
  }
}

void setup() {
  Serial.begin(9600);

  // Set pin mode untuk tombol
  pinMode(btnForward, INPUT_PULLUP);
  pinMode(btnMonitoring, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
  pinMode(pot, INPUT);

  // Inisialisasi Wi-Fi Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Pastikan perangkat tidak terhubung ke jaringan Wi-Fi

  // Inisialisasi ESP-NOW
  if (esp_now_init() != 0) {  // Menggunakan nilai 0 untuk pengecekan keberhasilan
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set role sebagai controller (master)
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);  // Mengatur perangkat ini sebagai master/controller

  // Register callback untuk pengiriman data
  esp_now_register_send_cb(OnDataSent);

  // Tambahkan peer (slave) untuk komunikasi
  if (esp_now_add_peer(mac_slave, ESP_NOW_ROLE_SLAVE, 1, NULL, 0) != 0) {  // Menggunakan nilai 0 untuk pengecekan keberhasilan
    Serial.println("Failed to add peer");
    return;
  }

  // Cek status buffer
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // Baca nilai potensiometer
  int potValue = analogRead(pot);
  int rotation = map(potValue, 0, 1023, 0, 180);  // Map potensiometer ke rentang 0-180

  // Tentukan perintah berdasarkan tombol yang ditekan
  if (digitalRead(btnForward) == LOW) {
    strcpy(myData.command, "FORWARD");
  } else if (digitalRead(btnMonitoring) == LOW) {
    strcpy(myData.command, "MONITORING");
    Serial.println("Monitoring activated. Sending full sensor data...");
  } else if (digitalRead(btnLeft) == LOW) {
    strcpy(myData.command, "LEFT");
  } else if (digitalRead(btnRight) == LOW) {
    strcpy(myData.command, "RIGHT");
  } else {
    strcpy(myData.command, "STOP");  // Jika tidak ada tombol yang ditekan
  }

  // Set nilai rotasi dan status tombol dalam struktur data
  myData.rotation = rotation;
  myData.btnForwardState = digitalRead(btnForward);
  myData.btnMonitoringState = digitalRead(btnMonitoring);
  myData.btnLeftState = digitalRead(btnLeft);
  myData.btnRightState = digitalRead(btnRight);

  // Menampilkan perintah dan status tombol yang dikirimkan
  Serial.println("Command: " + String(myData.command));
  Serial.print("Forward Btn: ");
  Serial.println(myData.btnForwardState);
  Serial.print("Monitoring Btn: ");
  Serial.println(myData.btnMonitoringState);
  Serial.print("Left Btn: ");
  Serial.println(myData.btnLeftState);
  Serial.print("Right Btn: ");
  Serial.println(myData.btnRightState);

  // Kirim data ke slave menggunakan ESP-NOW
  int result = esp_now_send(mac_slave, (uint8_t *)&myData, sizeof(myData));
  if (result == 0) {  // Menggunakan nilai 0 untuk pengecekan keberhasilan
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
    Serial.print("Error code: ");
    Serial.println(result);

    // Cek jika error adalah masalah memori
    if (result == -3) {  // -3 adalah kode kesalahan untuk ESP_ERR_ESPNOW_NO_MEM
      Serial.println("No memory available for ESP-NOW operation.");
    }
  }

  // Menampilkan sisa memori
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());

  // Tambahkan delay untuk mengatur frekuensi pengiriman
  delay(100);  // Mengirim data setiap 0.1 detik
}