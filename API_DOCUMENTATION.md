# RC Car API Dokümantasyonu

**Firmware Versiyon:** v1.3.1  
**Tarih:** 2025-10-14  
**Protokol:** HTTP/REST API  
**Port:** 80

---

## 📡 Bağlantı Bilgileri

### Wi-Fi Ağı
- **SSID:** 3BFab-RD
- **Şifre:** 20223BFab*

### Cihaz IP Adresi
RC Car Wi-Fi'ye bağlandığında seri monitörden IP adresini alabilirsiniz.
Örnek: `http://192.168.1.100`

### OTA (Over-The-Air) Güncelleme
- **Hostname:** RC-Car
- **OTA Şifre:** 20223BFab*

---

## 🎮 API Endpoints

### 1. Ana Sayfa (Web Arayüzü)
```
GET /
```
**Açıklama:** HTML arayüzünü döner (mobil uygulama için gerekli değil)

**Response:** HTML sayfa

---

### 2. Servo Motor Kontrolü (Direksiyon)
```
GET /api/servo?angle={0-180}
```

**Açıklama:** Direksiyon açısını kontrol eder

**Parametreler:**
- `angle` (zorunlu): 0-180 arası tam sayı
  - `0°` = Maksimum sol
  - `72°` = Merkez/Düz (kalibrasyon: gerçek 0°)
  - `180°` = Maksimum sağ

**Örnek İstek:**
```
GET http://192.168.1.100/api/servo?angle=72
```

**Response:**
- **Başarılı:** `200 OK` - "OK"
- **Hatalı:** `400 Bad Request` - "angle parameter missing"

**Kullanım:**
- Mobil uygulamada slider veya joystick ile kontrol edilebilir
- Gerçek açı = `angle - 72` (kalibrasyon için)

---

### 3. Motor Kontrolü (Gaz/Hız)
```
GET /api/mosfet?duty={-255 ile +255}
```

**Açıklama:** Motor hızını ve yönünü kontrol eder

**Parametreler:**
- `duty` (zorunlu): -255 ile +255 arası tam sayı
  - **Pozitif değer (+1 ile +255):** İleri hareket
  - **Negatif değer (-1 ile -255):** Geri hareket
  - **0:** Motor dur

**PWM Davranışı:**
- 0-255 değeri otomatik olarak 0-1023 PWM'e map edilir
- Minimum PWM eşiği: 200 (yaklaşık %20)
- PWM Frekansı: 2000 Hz (2kHz)

**Örnek İstekler:**
```
# İleri tam gaz
GET http://192.168.1.100/api/mosfet?duty=255

# İleri yarı gaz
GET http://192.168.1.100/api/mosfet?duty=128

# Dur
GET http://192.168.1.100/api/mosfet?duty=0

# Geri yarı gaz
GET http://192.168.1.100/api/mosfet?duty=-128
```

**Response:**
- **Başarılı:** `200 OK` - "OK"
- **Fren aktifse:** `200 OK` - "BRAKING"
- **Hatalı:** `400 Bad Request` - "duty parameter missing"

**Önemli Not:**
- Fren butonu aktifken motor komutları engellenir
- Vites+Gaz mantığını mobil uygulamada yapmanız gerekir:
  ```
  Vites D (Drive) + Gaz %50 = duty=128
  Vites R (Reverse) + Gaz %50 = duty=-128
  Vites N (Neutral) = duty=0
  ```

---

### 4. Fren Kontrolü
```
GET /api/brake?state={0|1}
```

**Açıklama:** Fren sistemini aktif/pasif eder

**Parametreler:**
- `state` (zorunlu): 0 veya 1
  - `1` = Fren aktif
  - `0` = Fren pasif

**Davranış:**
- Fren aktif olunca:
  - Motor dinamik frenleme moduna girer (kısa devre)
  - Stop lambası otomatik yanar
  - Motor komutları engellenilir
- Fren pasif olunca:
  - Motor normal kontrole döner
  - Stop lambası söner

**Örnek İstekler:**
```
# Fren bas
GET http://192.168.1.100/api/brake?state=1

# Fren bırak
GET http://192.168.1.100/api/brake?state=0
```

**Response:**
- **Başarılı:** `200 OK` 
  - "BRAKING" (fren aktif)
  - "RELEASED" (fren pasif)
- **Hatalı:** `400 Bad Request` - "state parameter missing"

**Mobil UI Önerisi:**
- Butona basılı tutulduğunda `state=1`
- Bırakılınca `state=0`

---

### 5. Ön Far Kontrolü
```
GET /api/headlight
```

**Açıklama:** Ön farları aç/kapa (toggle)

**Parametreler:** Yok

**Davranış:**
- Her çağrıda durum değişir (açık ↔ kapalı)
- D2 (GPIO4) pini kontrol edilir

**Örnek İstek:**
```
GET http://192.168.1.100/api/headlight
```

**Response:**
- **Başarılı:** `200 OK`
  - "ON" (farlar açık)
  - "OFF" (farlar kapalı)

**Mobil UI Önerisi:**
- Toggle switch veya buton
- Response'a göre UI durumunu güncelle

---

### 6. Stop Lambası Kontrolü
```
GET /api/stoplight
```

**Açıklama:** Stop lambasını aç/kapa (toggle) - Manuel kontrol

**Parametreler:** Yok

**Davranış:**
- Her çağrıda durum değişir (açık ↔ kapalı)
- D1 (GPIO5) pini kontrol edilir
- **Not:** Fren basıldığında otomatik yanar, bu API manuel kontroldür

**Örnek İstek:**
```
GET http://192.168.1.100/api/stoplight
```

**Response:**
- **Başarılı:** `200 OK`
  - "ON" (lamba açık)
  - "OFF" (lamba kapalı)

**Mobil UI Önerisi:**
- Toggle switch veya buton
- Response'a göre UI durumunu güncelle

---

### 7. Versiyon Bilgisi
```
GET /api/version
```

**Açıklama:** Firmware versiyonunu ve build tarihini döner

**Parametreler:** Yok

**Örnek İstek:**
```
GET http://192.168.1.100/api/version
```

**Response:**
```
v1.3.1 | Oct 14 2025 12:34:56
```

**Mobil UI Önerisi:**
- Ayarlar sayfasında gösterebilirsiniz

---

## 🎯 Mobil Uygulama Geliştirme Önerileri

### 1. Vites Sistemi (Mobil Tarafta)
Mobil uygulamada 3 vites butonu olmalı:
- **R (Reverse):** Geri vites
- **N (Neutral):** Boşta
- **D (Drive):** İleri vites

**Mantık:**
```dart
int calculateMotorSpeed(String gear, int gasPercent) {
  if (gear == 'N') return 0;
  
  int speed = (gasPercent * 2.55).round(); // 0-100 → 0-255
  
  if (gear == 'R') {
    return -speed; // Negatif = Geri
  } else if (gear == 'D') {
    return speed; // Pozitif = İleri
  }
  
  return 0;
}
```

### 2. Joystick/Slider Kontrolleri

**Gaz Pedalı:**
- Slider: 0-100%
- Her değişiklikte `/api/mosfet` çağrılır
- Vites durumuna göre pozitif/negatif değer gönderilir

**Direksiyon:**
- Slider veya joystick: 0-180°
- Gerçek açı: value - 72
- Her değişiklikte `/api/servo` çağrılır
- Merkez: 72°

### 3. HTTP İstek Örneği (Flutter/Dart)

```dart
import 'package:http/http.dart' as http;

class RCCarAPI {
  final String baseUrl = 'http://192.168.1.100'; // RC Car IP
  
  // Direksiyon kontrolü
  Future<void> setServoAngle(int angle) async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/servo?angle=$angle')
      );
      
      if (response.statusCode == 200) {
        print('Servo: ${response.body}');
      }
    } catch (e) {
      print('Servo error: $e');
    }
  }
  
  // Motor kontrolü
  Future<void> setMotorSpeed(int duty) async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/mosfet?duty=$duty')
      );
      
      if (response.statusCode == 200) {
        print('Motor: ${response.body}');
      }
    } catch (e) {
      print('Motor error: $e');
    }
  }
  
  // Fren kontrolü
  Future<void> setBrake(bool pressed) async {
    try {
      final state = pressed ? 1 : 0;
      final response = await http.get(
        Uri.parse('$baseUrl/api/brake?state=$state')
      );
      
      if (response.statusCode == 200) {
        print('Brake: ${response.body}');
      }
    } catch (e) {
      print('Brake error: $e');
    }
  }
  
  // Ön far toggle
  Future<String> toggleHeadlight() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/headlight')
      );
      
      if (response.statusCode == 200) {
        return response.body; // "ON" veya "OFF"
      }
    } catch (e) {
      print('Headlight error: $e');
    }
    return 'OFF';
  }
  
  // Stop lambası toggle
  Future<String> toggleStopLight() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/stoplight')
      );
      
      if (response.statusCode == 200) {
        return response.body; // "ON" veya "OFF"
      }
    } catch (e) {
      print('StopLight error: $e');
    }
    return 'OFF';
  }
  
  // Versiyon al
  Future<String> getVersion() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/version')
      );
      
      if (response.statusCode == 200) {
        return response.body;
      }
    } catch (e) {
      print('Version error: $e');
    }
    return 'Unknown';
  }
}
```

### 4. Kullanım Örneği

```dart
void main() async {
  final car = RCCarAPI();
  
  // İleri %50 güç ile git
  await car.setMotorSpeed(128);
  
  // Direksiyonu sağa çevir (90° + 72 = 162)
  await car.setServoAngle(162);
  
  // Fren bas
  await car.setBrake(true);
  
  // Bekle
  await Future.delayed(Duration(seconds: 1));
  
  // Fren bırak
  await car.setBrake(false);
  
  // Farları aç
  String headlightStatus = await car.toggleHeadlight();
  print('Headlight: $headlightStatus');
}
```

---

## 🔧 Pin Bağlantıları (Referans)

| Pin | GPIO | Fonksiyon | Bağlantı |
|-----|------|-----------|----------|
| D5 | GPIO14 | Servo PWM | Direksiyon servo |
| D6 | GPIO12 | Motor ENA (PWM) | L298N Enable A |
| D7 | GPIO13 | Motor IN1 | L298N Input 1 |
| D8 | GPIO15 | Motor IN2 | L298N Input 2 |
| D1 | GPIO5 | Stop LED | LED+ (dirençle) |
| D2 | GPIO4 | Headlight LED | LED+ (dirençle) |

---

## ⚡ Hızlı Test Senaryosu

1. **Bağlantı Testi:**
   ```
   GET http://{IP}/api/version
   ```

2. **Direksiyon Merkez:**
   ```
   GET http://{IP}/api/servo?angle=72
   ```

3. **Motor Dur:**
   ```
   GET http://{IP}/api/mosfet?duty=0
   ```

4. **İleri Yavaş:**
   ```
   GET http://{IP}/api/mosfet?duty=100
   ```

5. **Geri Yavaş:**
   ```
   GET http://{IP}/api/mosfet?duty=-100
   ```

6. **Farları Aç:**
   ```
   GET http://{IP}/api/headlight
   ```

---

## 📱 Önerilen Mobil UI Yapısı

```
┌─────────────────────────────┐
│      RC Car Control         │
│  v1.3.1 | 192.168.1.100     │
├─────────────────────────────┤
│                             │
│  ┌───┐  ┌───┐  ┌───┐       │
│  │ R │  │ N │  │ D │       │ ← Vites Seçici
│  └───┘  └─▲─┘  └───┘       │
│                             │
│  ⚡ Gaz Pedalı              │
│  ▓▓▓▓▓▓░░░░  50%           │ ← Slider (0-100%)
│                             │
│  🚗 Direksiyon              │
│  ░░░░░▓▓▓░░░  +18°         │ ← Slider (0-180°)
│                             │
│  ┌─────────────────┐       │
│  │  🛑 ACİL DURDUR │       │
│  └─────────────────┘       │
│                             │
│  ┌─────────────────┐       │
│  │  🅱️ FREN       │       │ ← Basılı tut
│  └─────────────────┘       │
│                             │
│   ┌──────┐  ┌──────┐      │
│   │  💡  │  │  🔴  │      │ ← Toggle
│   │ FAR  │  │ STOP │      │
│   └──────┘  └──────┘      │
│                             │
└─────────────────────────────┘
```

---

## ⚠️ Önemli Notlar

1. **Timeout:** HTTP isteklerinde timeout süresi ekleyin (örn: 2 saniye)
2. **Hata Yönetimi:** Ağ bağlantısı kesilirse kullanıcıya bildir
3. **Seri İstekler:** Slider değişirken çok sık istek atmamak için debounce kullanın
4. **Güvenlik:** Aynı Wi-Fi ağında olmanız gerekir
5. **IP Adresi:** Uygulama ayarlarından IP girişi ekleyin

---

## 📞 Destek

Sorularınız için:
- **Firmware:** v1.3.1
- **Platform:** ESP8266 (Wemos D1 Mini)
- **Geliştirici:** 3BFAB Teknoloji

---

**Son Güncelleme:** 14 Ekim 2025

