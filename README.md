# RC Car - ESP8266 Kontrollü Araç Projesi

Wi-Fi üzerinden kontrol edilebilen RC araç projesi. ESP8266 (Wemos D1 Mini) mikrodenetleyici kullanılarak geliştirilmiştir.

## 🚗 Özellikler

- **Wi-Fi Kontrolü**: HTTP/REST API üzerinden web arayüzü veya mobil uygulama ile kontrol
- **Servo Direksiyon**: 0-180° arası hassas direksiyon kontrolü
- **DC Motor Kontrolü**: İleri/geri hareket ve hız kontrolü
- **LED Işıklandırma**: Ön farlar ve stop lambaları
- **OTA Güncelleme**: Kablosuz firmware güncelleme desteği
- **Gerçek Zamanlı Durum**: Araç durumu ve sensör verileri için API endpoint'leri

## 📋 Donanım Gereksinimleri

- **Mikrodenetleyici**: Wemos D1 Mini (ESP8266)
- **Servo Motor**: Standart SG90 veya benzeri servo motor
- **Motor Sürücü**: L298N veya L293D motor sürücü modülü
- **LED'ler**: 2x LED (ön farlar), 2x LED (stop lambaları)
- **Güç Kaynağı**: 5V USB güç kaynağı + motor için ayrı güç kaynağı

## 🔌 Pin Bağlantıları

| Bileşen | Pin | GPIO | Açıklama |
|---------|-----|------|----------|
| Servo Motor | D5 | GPIO14 | Direksiyon kontrolü |
| Motor ENA | D6 | GPIO12 | PWM hız kontrolü |
| Motor IN1 | D7 | GPIO13 | Yön kontrolü 1 |
| Motor IN2 | D8 | GPIO15 | Yön kontrolü 2 |
| Stop LED | D1 | GPIO5 | Stop lambası |
| Headlight LED | D2 | GPIO4 | Ön farlar |

## 🛠️ Kurulum

### 1. PlatformIO Kurulumu

Proje PlatformIO kullanılarak geliştirilmiştir. PlatformIO IDE veya VS Code eklentisi kurulu olmalıdır.

### 2. Bağımlılıkları Yükleme

```bash
pio lib install
```

### 3. Wi-Fi Ayarları

**Önemli**: Hassas bilgiler (WiFi şifreleri, OTA şifreleri) `include/config.h` dosyasında saklanır ve Git'e yüklenmez.

1. `include/config.h.example` dosyasını `include/config.h` olarak kopyalayın:
```bash
cp include/config.h.example include/config.h
```

2. `include/config.h` dosyasını açın ve kendi bilgilerinizi girin:
```cpp
static const char* WIFI_SSID = "WiFi-Ağ-Adınız";
static const char* WIFI_PASSWORD = "WiFi-Şifreniz";
static const char* OTA_PASSWORD = "OTA-Şifreniz";
```

### 4. Derleme ve Yükleme

**USB ile yükleme (ilk yükleme):**
```bash
pio run -e d1_mini -t upload
```

**OTA ile yükleme (sonraki güncellemeler):**
```bash
pio run -e d1_mini_ota -t upload
```

OTA için `platformio.ini` dosyasındaki IP adresini ve şifreyi güncelleyin:
```ini
upload_port = 192.168.1.100  # ESP8266'nın IP adresi
upload_flags =
  --auth=OTA-Şifreniz         # config.h dosyasındaki OTA_PASSWORD ile aynı olmalı
```

## 📡 API Kullanımı

### Temel Endpoint'ler

**Servo Kontrolü (Direksiyon)**
```
GET /api/servo?angle=72
```

**Motor Kontrolü**
```
GET /api/motor?speed=100&direction=forward
```

**LED Kontrolü**
```
GET /api/leds?headlight=1&stoplight=0
```

**Durum Bilgisi**
```
GET /api/status
```

Detaylı API dokümantasyonu için `API_REFERENCE.json` ve `API_DOCUMENTATION.md` dosyalarına bakın.

## 📁 Proje Yapısı

```
RC Car/
├── src/
│   └── main.cpp          # Ana program kodu
├── platformio.ini        # PlatformIO yapılandırması
├── API_REFERENCE.json    # API referans dokümantasyonu
├── API_DOCUMENTATION.md  # Detaylı API dokümantasyonu
└── README.md             # Bu dosya
```

## 🔧 Yapılandırma

### Servo Kalibrasyonu

Servo açı aralığı `main.cpp` içinde ayarlanabilir:
```cpp
static const int SERVO_MIN_DEG = 0;
static const int SERVO_MAX_DEG = 180;
static int currentServoAngle = 72;  // Merkez pozisyon
```

### Motor Hız Kontrolü

Motor hızı -255 ile +255 arasında ayarlanabilir:
- Pozitif değerler: İleri hareket
- Negatif değerler: Geri hareket
- 0: Durdurma

## 📝 Versiyon Bilgisi

- **Firmware Versiyonu**: v1.3.1
- **Platform**: ESP8266 (Arduino Framework)
- **Kütüphaneler**: ESP8266Servo

## 🤝 Katkıda Bulunma

1. Bu repository'yi fork edin
2. Yeni bir branch oluşturun (`git checkout -b feature/yeni-ozellik`)
3. Değişikliklerinizi commit edin (`git commit -am 'Yeni özellik eklendi'`)
4. Branch'inizi push edin (`git push origin feature/yeni-ozellik`)
5. Pull Request oluşturun

## 📄 Lisans

Bu proje açık kaynaklıdır ve serbestçe kullanılabilir.

## 👨‍💻 Geliştirici

3BFAB TEKNOLOJİ A.Ş.

---

**Not**: Bu proje eğitim ve hobi amaçlıdır. Ticari kullanım için gerekli sertifikasyonları kontrol edin.
