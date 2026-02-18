# RC Car Flutter Mobil Uygulama Geliştirme Kılavuzu

## 📋 İçindekiler

Bu klasörde şu dosyalar bulunmaktadır:

1. **API_DOCUMENTATION.md** - Detaylı API dokümantasyonu (Türkçe)
2. **API_REFERENCE.json** - JSON formatında API referansı
3. **RC_Car_API.postman_collection.json** - Postman test koleksiyonu
4. **README_FLUTTER.md** - Bu dosya

---

## 🚀 Hızlı Başlangıç

### Adım 1: RC Car'ı Wi-Fi'ye Bağla

1. RC Car'ı açın
2. Seri monitörden IP adresini not edin
3. Aynı Wi-Fi ağına bağlanın (SSID: `3BFab-RD`)

### Adım 2: API'yi Test Et

**Tarayıcıdan:**
```
http://192.168.1.100/api/version
```

Yanıt: `v1.3.1 | Oct 14 2025 12:34:56`

### Adım 3: Flutter Projesini Hazırla

```yaml
# pubspec.yaml
dependencies:
  flutter:
    sdk: flutter
  http: ^1.1.0
```

---

## 🎯 Mobil Uygulama Gereksinimleri

### Minimum Özellikler

1. **Vites Sistemi**
   - R (Reverse) - Geri
   - N (Neutral) - Boş
   - D (Drive) - İleri

2. **Gaz Pedalı**
   - Slider: 0-100%
   - Real-time güncelleme

3. **Direksiyon**
   - Slider veya joystick: 0-180°
   - Merkez: 72°
   - Gösterge: -90° ile +90°

4. **Fren Butonu**
   - Basılı tutma (hold to brake)
   - Stop lambası otomatiği

5. **Işık Kontrolleri**
   - Ön far: Toggle buton
   - Stop lambası: Toggle buton

6. **Acil Durdur**
   - Tüm sistemleri durdur

---

## 💻 Flutter Kod Örnekleri

### API Service Sınıfı

```dart
// lib/services/rc_car_api.dart

import 'package:http/http.dart' as http;
import 'dart:async';

class RCCarAPI {
  String baseUrl;
  
  RCCarAPI({this.baseUrl = 'http://192.168.1.100'});
  
  // Timeout süresi
  final Duration timeout = Duration(seconds: 2);
  
  // Direksiyon kontrolü
  Future<bool> setSteeringAngle(int angle) async {
    if (angle < 0 || angle > 180) return false;
    
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/servo?angle=$angle')
      ).timeout(timeout);
      
      return response.statusCode == 200;
    } catch (e) {
      print('Servo error: $e');
      return false;
    }
  }
  
  // Motor kontrolü
  Future<bool> setMotorSpeed(int duty) async {
    if (duty < -255 || duty > 255) return false;
    
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/mosfet?duty=$duty')
      ).timeout(timeout);
      
      return response.statusCode == 200 && response.body == 'OK';
    } catch (e) {
      print('Motor error: $e');
      return false;
    }
  }
  
  // Fren kontrolü
  Future<bool> setBrake(bool active) async {
    try {
      final state = active ? 1 : 0;
      final response = await http.get(
        Uri.parse('$baseUrl/api/brake?state=$state')
      ).timeout(timeout);
      
      return response.statusCode == 200;
    } catch (e) {
      print('Brake error: $e');
      return false;
    }
  }
  
  // Ön far toggle
  Future<bool> toggleHeadlight() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/headlight')
      ).timeout(timeout);
      
      return response.body == 'ON';
    } catch (e) {
      print('Headlight error: $e');
      return false;
    }
  }
  
  // Stop lambası toggle
  Future<bool> toggleStopLight() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/stoplight')
      ).timeout(timeout);
      
      return response.body == 'ON';
    } catch (e) {
      print('StopLight error: $e');
      return false;
    }
  }
  
  // Versiyon al
  Future<String> getVersion() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/version')
      ).timeout(timeout);
      
      return response.body;
    } catch (e) {
      return 'Bağlantı hatası';
    }
  }
  
  // Acil durdur
  Future<void> emergencyStop() async {
    await setMotorSpeed(0);
    await setSteeringAngle(72);
  }
}
```

### Controller Sınıfı (Provider/GetX/Riverpod)

```dart
// lib/controllers/rc_car_controller.dart

import 'package:flutter/material.dart';
import '../services/rc_car_api.dart';

class RCCarController extends ChangeNotifier {
  final RCCarAPI api;
  
  RCCarController(this.api);
  
  // Durum değişkenleri
  String _gear = 'N'; // R, N, D
  int _gasPercent = 0; // 0-100
  int _steeringAngle = 72; // 0-180
  bool _isBraking = false;
  bool _headlightOn = false;
  bool _stopLightOn = false;
  
  // Getters
  String get gear => _gear;
  int get gasPercent => _gasPercent;
  int get steeringAngle => _steeringAngle;
  bool get isBraking => _isBraking;
  bool get headlightOn => _headlightOn;
  bool get stopLightOn => _stopLightOn;
  
  // Direksiyon gösterge açısı (-90 ile +90)
  int get displayAngle => _steeringAngle - 72;
  
  // Vites değiştir
  Future<void> changeGear(String newGear) async {
    if (!['R', 'N', 'D'].contains(newGear)) return;
    
    _gear = newGear;
    notifyListeners();
    
    // Motor durumunu güncelle
    await _updateMotor();
  }
  
  // Gaz değiştir
  Future<void> setGas(int percent) async {
    _gasPercent = percent.clamp(0, 100);
    notifyListeners();
    
    // Motor durumunu güncelle
    await _updateMotor();
  }
  
  // Motor güncelle (vites + gaz hesabı)
  Future<void> _updateMotor() async {
    int duty = 0;
    
    if (_gear == 'D') {
      // İleri: pozitif değer
      duty = (_gasPercent * 2.55).round();
    } else if (_gear == 'R') {
      // Geri: negatif değer
      duty = -(_gasPercent * 2.55).round();
    } else {
      // Neutral: 0
      duty = 0;
    }
    
    await api.setMotorSpeed(duty);
  }
  
  // Direksiyon değiştir
  Future<void> setSteering(int angle) async {
    _steeringAngle = angle.clamp(0, 180);
    notifyListeners();
    
    await api.setSteeringAngle(_steeringAngle);
  }
  
  // Fren
  Future<void> setBrake(bool active) async {
    _isBraking = active;
    notifyListeners();
    
    await api.setBrake(active);
  }
  
  // Ön far
  Future<void> toggleHeadlight() async {
    _headlightOn = await api.toggleHeadlight();
    notifyListeners();
  }
  
  // Stop lambası
  Future<void> toggleStopLight() async {
    _stopLightOn = await api.toggleStopLight();
    notifyListeners();
  }
  
  // Acil durdur
  Future<void> emergencyStop() async {
    await changeGear('N');
    await setGas(0);
    await setSteering(72);
    await api.emergencyStop();
  }
}
```

### UI Örneği

```dart
// lib/screens/control_screen.dart

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../controllers/rc_car_controller.dart';

class ControlScreen extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('RC Car Kumanda'),
      ),
      body: Consumer<RCCarController>(
        builder: (context, controller, child) {
          return Padding(
            padding: EdgeInsets.all(16),
            child: Column(
              children: [
                // Vites Seçici
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    _buildGearButton('R', controller),
                    SizedBox(width: 10),
                    _buildGearButton('N', controller),
                    SizedBox(width: 10),
                    _buildGearButton('D', controller),
                  ],
                ),
                
                SizedBox(height: 30),
                
                // Gaz Pedalı
                Text('⚡ Gaz: ${controller.gasPercent}%'),
                Slider(
                  value: controller.gasPercent.toDouble(),
                  min: 0,
                  max: 100,
                  onChanged: (value) {
                    controller.setGas(value.round());
                  },
                ),
                
                SizedBox(height: 20),
                
                // Direksiyon
                Text('🚗 Direksiyon: ${controller.displayAngle}°'),
                Slider(
                  value: controller.steeringAngle.toDouble(),
                  min: 0,
                  max: 180,
                  onChanged: (value) {
                    controller.setSteering(value.round());
                  },
                ),
                
                SizedBox(height: 30),
                
                // Acil Durdur
                ElevatedButton(
                  onPressed: () => controller.emergencyStop(),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.red,
                    minimumSize: Size(double.infinity, 50),
                  ),
                  child: Text('🛑 ACİL DURDUR'),
                ),
                
                SizedBox(height: 10),
                
                // Fren (Basılı Tut)
                GestureDetector(
                  onTapDown: (_) => controller.setBrake(true),
                  onTapUp: (_) => controller.setBrake(false),
                  onTapCancel: () => controller.setBrake(false),
                  child: Container(
                    width: double.infinity,
                    height: 50,
                    decoration: BoxDecoration(
                      color: controller.isBraking ? Colors.red[700] : Colors.red[400],
                      borderRadius: BorderRadius.circular(10),
                    ),
                    alignment: Alignment.center,
                    child: Text(
                      '🅱️ FREN (Basılı Tut)',
                      style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
                    ),
                  ),
                ),
                
                SizedBox(height: 20),
                
                // Işıklar
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    _buildLightButton(
                      '💡\nÖN FAR',
                      controller.headlightOn,
                      () => controller.toggleHeadlight(),
                    ),
                    _buildLightButton(
                      '🔴\nSTOP',
                      controller.stopLightOn,
                      () => controller.toggleStopLight(),
                    ),
                  ],
                ),
              ],
            ),
          );
        },
      ),
    );
  }
  
  Widget _buildGearButton(String gear, RCCarController controller) {
    bool isActive = controller.gear == gear;
    
    return ElevatedButton(
      onPressed: () => controller.changeGear(gear),
      style: ElevatedButton.styleFrom(
        backgroundColor: isActive ? Colors.green : Colors.grey,
        minimumSize: Size(70, 70),
      ),
      child: Text(
        gear,
        style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
      ),
    );
  }
  
  Widget _buildLightButton(String label, bool isOn, VoidCallback onPressed) {
    return ElevatedButton(
      onPressed: onPressed,
      style: ElevatedButton.styleFrom(
        backgroundColor: isOn ? Colors.yellow : Colors.grey,
        minimumSize: Size(100, 100),
        shape: CircleBorder(),
      ),
      child: Text(
        label,
        textAlign: TextAlign.center,
        style: TextStyle(fontSize: 12),
      ),
    );
  }
}
```

### Main.dart

```dart
// lib/main.dart

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'services/rc_car_api.dart';
import 'controllers/rc_car_controller.dart';
import 'screens/control_screen.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    // API servisini oluştur
    final api = RCCarAPI(baseUrl: 'http://192.168.1.100');
    
    return ChangeNotifierProvider(
      create: (_) => RCCarController(api),
      child: MaterialApp(
        title: 'RC Car',
        theme: ThemeData(
          primarySwatch: Colors.blue,
        ),
        home: ControlScreen(),
      ),
    );
  }
}
```

---

## 🎨 UI/UX Önerileri

### 1. Joystick Kontrolü (Opsiyonel)

```yaml
# pubspec.yaml
dependencies:
  flutter_joystick: ^0.0.5
```

### 2. Debounce (Slider için)

```dart
import 'dart:async';

class Debouncer {
  final int milliseconds;
  Timer? _timer;
  
  Debouncer({required this.milliseconds});
  
  void run(VoidCallback action) {
    _timer?.cancel();
    _timer = Timer(Duration(milliseconds: milliseconds), action);
  }
}

// Kullanım:
final _debouncer = Debouncer(milliseconds: 100);

Slider(
  onChanged: (value) {
    _debouncer.run(() {
      controller.setSteering(value.round());
    });
  },
);
```

### 3. Bağlantı Durumu Göstergesi

```dart
FutureBuilder<String>(
  future: api.getVersion(),
  builder: (context, snapshot) {
    if (snapshot.hasData) {
      return Text('✅ Bağlı: ${snapshot.data}');
    } else {
      return Text('❌ Bağlantı Yok');
    }
  },
)
```

---

## ⚠️ Dikkat Edilmesi Gerekenler

1. **Timeout:** Her HTTP isteğinde timeout ekleyin (2 saniye)
2. **Hata Yönetimi:** Try-catch ile hataları yakalayın
3. **Debounce:** Slider hareketinde çok sık istek atmayın
4. **Wi-Fi Kontrolü:** Uygulama başlatıldığında Wi-Fi kontrolü yapın
5. **IP Yapılandırması:** IP adresini ayarlardan değiştirilebilir yapın

---

## 📦 Gerekli Paketler

```yaml
dependencies:
  flutter:
    sdk: flutter
  
  # HTTP istekleri için
  http: ^1.1.0
  
  # State management için (birini seçin)
  provider: ^6.0.5
  # veya
  get: ^4.6.5
  # veya
  riverpod: ^2.4.0
  
  # Opsiyonel: Joystick
  flutter_joystick: ^0.0.5
  
  # Opsiyonel: Ayarlar sayfası
  shared_preferences: ^2.2.2
```

---

## 🧪 Test Adımları

1. RC Car'ı aç ve IP adresini al
2. Postman koleksiyonunu import et
3. Tüm endpoint'leri test et
4. Flutter uygulamasını geliştir
5. Gerçek cihazda test et

---

## 📞 Destek

Sorularınız için:
- **Firmware Versiyon:** v1.3.1
- **Platform:** ESP8266 (Wemos D1 Mini)
- **Geliştirici:** 3BFAB Teknoloji

---

**Son Güncelleme:** 14 Ekim 2025

**İyi Kodlamalar! 🚀**


