#include <Arduino.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

// Pin atamaları (Wemos D1 mini):
static const uint8_t SERVO_PIN = D5;   // GPIO14
// Motor sürücü pinleri (L298N veya L293D)
static const uint8_t MOTOR_ENA = D6;   // GPIO12 - PWM hız kontrolü
static const uint8_t MOTOR_IN1 = D7;   // GPIO13 - Yön 1
static const uint8_t MOTOR_IN2 = D8;   // GPIO15 - Yön 2
// Stop lambası (2 LED seri)
static const uint8_t STOP_LED_PIN = D1; // GPIO5
// Ön farlar (2 LED seri)
static const uint8_t HEADLIGHT_PIN = D2; // GPIO4

Servo steeringServo;

// Servo açı aralığı
static const int SERVO_MIN_US = 544;
static const int SERVO_MAX_US = 2400;
static const int SERVO_MIN_DEG = 0;
static const int SERVO_MAX_DEG = 180;

// Versiyon bilgisi
static const char* FIRMWARE_VERSION = "v1.3.1";
static const char* BUILD_DATE = __DATE__ " " __TIME__;

// Wi-Fi bilgileri
static const char* WIFI_SSID = "3BFab-RD";
static const char* WIFI_PASSWORD = "20223BFab*";

// HTTP sunucusu
static ESP8266WebServer server(80);

// Durum değişkenleri
static int currentServoAngle = 72;   // 0-180 derece (72° = merkez/0°, -18° kalibrasyon)
static int currentMotorSpeed = 0;    // -255 ile +255 arası (+ ileri, - geri)
static bool stopLightOn = false;     // Stop lambası durumu
static bool headlightOn = false;     // Ön far durumu
static char currentGear = 'N';       // Vites: 'D' = Drive, 'R' = Reverse, 'N' = Neutral
static int currentGas = 0;           // Gaz: 0-100%
static bool isBraking = false;       // Fren durumu
static int brakeIntensity = 100;     // Fren yoğunluğu: 0-100%

// Yardımcı: sınırla
static int clampInt(int value, int minVal, int maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}


// HTML sayfa (RC Car kumanda arayüzü - Sade Tasarım)
static const char HTML_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RC Car Kumanda</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
      color: #fff; 
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container { 
      background: rgba(0,0,0,0.4);
      backdrop-filter: blur(10px);
      border-radius: 20px;
      padding: 30px;
      max-width: 450px;
      width: 100%;
      box-shadow: 0 20px 60px rgba(0,0,0,0.5);
    }
    .header {
      text-align: center;
      margin-bottom: 25px;
    }
    .header h1 {
      font-size: 2em;
      margin-bottom: 5px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
    }
    .gear-selector {
      display: flex;
      gap: 10px;
      justify-content: center;
      margin-bottom: 25px;
    }
    .gear-btn {
      flex: 1;
      padding: 20px;
      font-size: 1.8em;
      font-weight: bold;
      border: 3px solid rgba(255,255,255,0.3);
      background: rgba(255,255,255,0.1);
      color: rgba(255,255,255,0.5);
      border-radius: 15px;
      cursor: pointer;
      transition: all 0.3s ease;
    }
    .gear-btn:hover {
      background: rgba(255,255,255,0.15);
    }
    .gear-btn.active {
      border-color: #00ff88;
      background: linear-gradient(45deg, #00d4aa, #00ff88);
      color: #000;
      box-shadow: 0 5px 20px rgba(0,255,136,0.4);
      transform: scale(1.05);
    }
    .gear-btn.active.reverse {
      border-color: #ff4757;
      background: linear-gradient(45deg, #ff4757, #ff6348);
      color: #fff;
      box-shadow: 0 5px 20px rgba(255,71,87,0.4);
    }
    .control-group {
      margin-bottom: 25px;
    }
    .control-label {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
      font-weight: bold;
      font-size: 1.1em;
    }
    .value-display {
      background: rgba(0,255,136,0.2);
      padding: 5px 15px;
      border-radius: 20px;
      min-width: 70px;
      text-align: center;
      color: #00ff88;
      font-weight: bold;
    }
    .slider {
      width: 100%;
      height: 50px;
      background: rgba(255,255,255,0.1);
      border-radius: 25px;
      outline: none;
      -webkit-appearance: none;
      appearance: none;
    }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 40px;
      height: 40px;
      background: linear-gradient(45deg, #00d4aa, #00ff88);
      border-radius: 50%;
      cursor: pointer;
      box-shadow: 0 3px 15px rgba(0,255,136,0.5);
    }
    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      background: linear-gradient(45deg, #00d4aa, #00ff88);
      border-radius: 50%;
      cursor: pointer;
      border: none;
      box-shadow: 0 3px 15px rgba(0,255,136,0.5);
    }
    .btn {
      width: 100%;
      background: linear-gradient(45deg, #ff4757, #ff6348);
      border: none;
      color: white;
      padding: 18px;
      margin: 8px 0;
      border-radius: 15px;
      font-size: 1.2em;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      box-shadow: 0 5px 20px rgba(255,71,87,0.3);
    }
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 7px 25px rgba(255,71,87,0.5);
    }
    .btn:active {
      transform: translateY(0);
    }
    .btn.light {
      background: linear-gradient(45deg, #666, #888);
      box-shadow: 0 5px 20px rgba(0,0,0,0.3);
    }
    .btn.brake {
      background: linear-gradient(45deg, #c0392b, #e74c3c);
      box-shadow: 0 5px 20px rgba(231,76,60,0.4);
    }
    .btn.brake.active {
      background: linear-gradient(45deg, #e74c3c, #ff6b6b);
      box-shadow: 0 8px 30px rgba(231,76,60,0.6);
      transform: scale(1.02);
    }
    .btn.headlight {
      background: linear-gradient(45deg, #95a5a6, #bdc3c7);
      box-shadow: 0 5px 20px rgba(189,195,199,0.3);
    }
    .btn.headlight.active {
      background: linear-gradient(45deg, #f39c12, #f1c40f);
      box-shadow: 0 8px 30px rgba(241,196,15,0.6);
      transform: scale(1.02);
    }
    .light-controls {
      display: flex;
      gap: 20px;
      justify-content: center;
      margin-top: 25px;
    }
    .round-btn {
      width: 100px;
      height: 100px;
      border-radius: 50%;
      border: none;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: all 0.3s ease;
      font-size: 0.75em;
      font-weight: bold;
      color: rgba(255,255,255,0.7);
      background: rgba(255,255,255,0.1);
      box-shadow: 0 5px 20px rgba(0,0,0,0.3);
    }
    .round-btn:hover {
      transform: translateY(-3px);
      box-shadow: 0 8px 25px rgba(0,0,0,0.4);
    }
    .round-btn:active {
      transform: translateY(-1px);
    }
    .round-btn .icon {
      font-size: 2.5em;
      margin-bottom: 5px;
    }
    .round-btn.headlight-btn {
      background: linear-gradient(135deg, #95a5a6, #7f8c8d);
    }
    .round-btn.headlight-btn.active {
      background: linear-gradient(135deg, #f39c12, #f1c40f);
      color: #000;
      box-shadow: 0 8px 30px rgba(241,196,15,0.6);
    }
    .round-btn.headlight-btn.active .icon {
      filter: drop-shadow(0 0 10px rgba(255,255,255,0.8));
    }
    .round-btn.stoplight-btn {
      background: linear-gradient(135deg, #95a5a6, #7f8c8d);
    }
    .round-btn.stoplight-btn.active {
      background: linear-gradient(135deg, #e74c3c, #c0392b);
      color: #fff;
      box-shadow: 0 8px 30px rgba(231,76,60,0.6);
    }
    .round-btn.stoplight-btn.active .icon {
      filter: drop-shadow(0 0 10px rgba(255,0,0,0.8));
    }
  </style>
  <script>
    let currentGear = 'N';
    let currentGas = 0;
    
    // Vites değiştir
    async function changeGear(gear) {
      currentGear = gear;
      document.querySelectorAll('.gear-btn').forEach(btn => btn.classList.remove('active'));
      document.getElementById('gear' + gear).classList.add('active');
      updateMotor();
    }
    
    // Gas değiştir
    function updateGas(value) {
      currentGas = parseInt(value);
      document.getElementById('gasLabel').textContent = currentGas + '%';
      updateMotor();
    }
    
    // Motoru güncelle (vites + gaz)
    async function updateMotor() {
      let speed = 0;
      if (currentGear === 'D') {
        speed = Math.round(currentGas * 2.55); // 0-100 -> 0-255
      } else if (currentGear === 'R') {
        speed = -Math.round(currentGas * 2.55); // 0-100 -> 0 to -255
      }
      
      try {
        await fetch('/api/mosfet?duty=' + speed);
      } catch (e) {
        console.error('Motor error:', e);
      }
    }
    
    // Direksiyon
    async function updateSteering(angle) {
      try {
        await fetch('/api/servo?angle=' + angle);
        document.getElementById('steerLabel').textContent = (angle-72) + '°';
      } catch (e) {
        console.error('Servo error:', e);
      }
    }
    
    // Durdur
    function emergencyStop() {
      changeGear('N');
      document.getElementById('gasSlider').value = 0;
      document.getElementById('steerSlider').value = 72;
      updateGas(0);
      updateSteering(72);
    }
    
    // Ön far
    async function toggleHeadlight() {
      try {
        const response = await fetch('/api/headlight');
        const status = await response.text();
        const btn = document.getElementById('headlightBtn');
        if (status === 'ON') {
          btn.classList.add('active');
        } else {
          btn.classList.remove('active');
        }
      } catch (e) {
        console.error('Headlight error:', e);
      }
    }
    
    // Stop lambası
    async function toggleStopLight() {
      try {
        const response = await fetch('/api/stoplight');
        const status = await response.text();
        const btn = document.getElementById('stopBtn');
        if (status === 'ON') {
          btn.classList.add('active');
        } else {
          btn.classList.remove('active');
        }
      } catch (e) {
        console.error('Stop light error:', e);
      }
    }
    
    // Fren (basılı tutma)
    let brakeActive = false;
    async function applyBrake(pressed) {
      brakeActive = pressed;
      const btn = document.getElementById('brakeBtn');
      
      if (pressed) {
        btn.classList.add('active');
        try {
          await fetch('/api/brake?state=1');
        } catch (e) {
          console.error('Brake error:', e);
        }
      } else {
        btn.classList.remove('active');
        try {
          await fetch('/api/brake?state=0');
          // Fren bırakınca motor durumunu güncelle
          updateMotor();
        } catch (e) {
          console.error('Brake error:', e);
        }
      }
    }
    
    // Versiyon bilgisini al
    async function loadVersion() {
      try {
        const response = await fetch('/api/version');
        const version = await response.text();
        document.getElementById('version').textContent = version;
      } catch (e) {
        document.getElementById('version').textContent = 'RC Car v1.0';
      }
    }
    
    // Sayfa yüklendiğinde
    document.addEventListener('DOMContentLoaded', function() {
      loadVersion();
    });
  </script>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🏎️ RC Car Kumanda</h1>
      <div style="font-size: 0.8em; opacity: 0.7; margin-top: 5px;" id="version">Yükleniyor...</div>
    </div>
    
    <!-- Vites Seçici -->
    <div class="gear-selector">
      <button class="gear-btn reverse" id="gearR" onclick="changeGear('R')">R</button>
      <button class="gear-btn active" id="gearN" onclick="changeGear('N')">N</button>
      <button class="gear-btn" id="gearD" onclick="changeGear('D')">D</button>
    </div>
    
    <!-- Gaz Pedalı -->
    <div class="control-group">
      <div class="control-label">
        <span>⚡ Gaz Pedalı</span>
        <span class="value-display" id="gasLabel">0%</span>
      </div>
      <input class="slider" id="gasSlider" type="range" min="0" max="100" value="0" 
             oninput="updateGas(this.value)">
    </div>
    
    <!-- Direksiyon -->
    <div class="control-group">
      <div class="control-label">
        <span>🚗 Direksiyon</span>
        <span class="value-display" id="steerLabel">0°</span>
      </div>
      <input class="slider" id="steerSlider" type="range" min="0" max="180" value="72" 
             oninput="updateSteering(this.value)">
    </div>
    
    <!-- Butonlar -->
    <button class="btn" onclick="emergencyStop()">🛑 ACİL DURDUR</button>
    <button class="btn brake" id="brakeBtn" 
            onmousedown="applyBrake(true)" 
            onmouseup="applyBrake(false)" 
            onmouseleave="applyBrake(false)"
            ontouchstart="applyBrake(true)" 
            ontouchend="applyBrake(false)">🅱️ FREN (Basılı Tut)</button>
    
    <!-- Işık Kontrolleri (Yuvarlak Butonlar) -->
    <div class="light-controls">
      <button class="round-btn headlight-btn" id="headlightBtn" onclick="toggleHeadlight()">
        <div class="icon">💡</div>
        <div>ÖN FAR</div>
      </button>
      <button class="round-btn stoplight-btn" id="stopBtn" onclick="toggleStopLight()">
        <div class="icon">🔴</div>
        <div>STOP</div>
      </button>
    </div>
  </div>
</body>
</html>
)HTML";

// HTTP handlers
static void handleRoot() {
  server.send(200, "text/html; charset=utf-8", HTML_PAGE);
}

static void handleServo() {
  if (!server.hasArg("angle")) { 
    server.send(400, "text/plain", "angle parameter missing"); 
    return; 
  }
  
  int angle = clampInt(server.arg("angle").toInt(), SERVO_MIN_DEG, SERVO_MAX_DEG);
  steeringServo.write(angle);
  currentServoAngle = angle;
  
  Serial.print("Servo: ");
  Serial.print(angle - 72);
  Serial.println("°");
  
  server.send(200, "text/plain", "OK");
}

static void handleMosfet() {
  if (!server.hasArg("duty")) { 
    server.send(400, "text/plain", "duty parameter missing"); 
    return; 
  }
  
  // -255 ile +255 arası değer al (+ ileri, - geri)
  int speed = clampInt(server.arg("duty").toInt(), -255, 255);
  currentMotorSpeed = speed;
  
  // Fren aktifse motor kontrolünü engelle
  if (isBraking) {
    server.send(200, "text/plain", "BRAKING");
    return;
  }
  
  if (speed == 0) {
    // Motor tamamen durdur
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    analogWrite(MOTOR_ENA, 0);
    Serial.println("Motor: DUR");
  } else if (speed > 0) {
    // İLERI GİT
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
    
    int pwmValue = map(speed, 0, 255, 0, 1023);
    if (pwmValue > 0 && pwmValue < 200) {
      pwmValue = 200;  // Minimum %20 PWM
    }
    
    analogWrite(MOTOR_ENA, pwmValue);
    
    Serial.print("Motor: İLERI ");
    Serial.print((speed * 100) / 255);
    Serial.print("% (PWM: ");
    Serial.print(pwmValue);
    Serial.println(")");
  } else {
    // GERİ GİT (speed < 0)
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
    
    int absSpeed = abs(speed);
    int pwmValue = map(absSpeed, 0, 255, 0, 1023);
    if (pwmValue > 0 && pwmValue < 200) {
      pwmValue = 200;  // Minimum %20 PWM
    }
    
    analogWrite(MOTOR_ENA, pwmValue);
    
    Serial.print("Motor: GERİ ");
    Serial.print((absSpeed * 100) / 255);
    Serial.print("% (PWM: ");
    Serial.print(pwmValue);
    Serial.println(")");
  }
  
  server.send(200, "text/plain", "OK");
}

static void handleBrake() {
  if (!server.hasArg("state")) { 
    server.send(400, "text/plain", "state parameter missing"); 
    return; 
  }
  
  int state = server.arg("state").toInt();
  isBraking = (state == 1);
  
  if (isBraking) {
    // FREN AKTIF: Dinamik frenleme (motor kısa devre modu)
    // Her iki yönü HIGH yaparak motor üzerinden enerji dissipasyonu
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    
    // Fren yoğunluğunu PWM ile ayarla (0-100% -> 0-1023)
    int brakePWM = map(brakeIntensity, 0, 100, 0, 1023);
    analogWrite(MOTOR_ENA, brakePWM);
    
    // Stop lambasını yak
    digitalWrite(STOP_LED_PIN, HIGH);
    stopLightOn = true;
    
    Serial.print("FREN AKTIF - Yoğunluk: ");
    Serial.print(brakeIntensity);
    Serial.println("%");
  } else {
    // FREN PASIF: Normal motor kontrolüne dön
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    analogWrite(MOTOR_ENA, 0);
    
    // Stop lambasını söndür
    digitalWrite(STOP_LED_PIN, LOW);
    stopLightOn = false;
    
    Serial.println("FREN SERBEST");
  }
  
  server.send(200, "text/plain", isBraking ? "BRAKING" : "RELEASED");
}

static void handleHeadlight() {
  // Toggle ön farlar
  headlightOn = !headlightOn;
  digitalWrite(HEADLIGHT_PIN, headlightOn ? HIGH : LOW);
  
  Serial.print("Ön farlar: ");
  Serial.println(headlightOn ? "AÇIK" : "KAPALI");
  
  server.send(200, "text/plain", headlightOn ? "ON" : "OFF");
}

static void handleStopLight() {
  // Toggle stop lambası
  stopLightOn = !stopLightOn;
  digitalWrite(STOP_LED_PIN, stopLightOn ? HIGH : LOW);
  
  Serial.print("Stop lambası: ");
  Serial.println(stopLightOn ? "AÇIK" : "KAPALI");
  
  server.send(200, "text/plain", stopLightOn ? "ON" : "OFF");
}

static void handleVersion() {
  String versionInfo = String(FIRMWARE_VERSION) + " | " + String(BUILD_DATE);
  server.send(200, "text/plain", versionInfo);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Servo başlat
  steeringServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  steeringServo.write(currentServoAngle);

  // Motor sürücü pinleri
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_ENA, 0);
  analogWriteRange(1023);
  analogWriteFreq(2000);  // 2kHz - DC motor için ideal
  
  Serial.println("Motor sürücü (L298N) hazır - İleri/Geri destekli");

  // Stop lambası (başlangıçta kapalı)
  pinMode(STOP_LED_PIN, OUTPUT);
  digitalWrite(STOP_LED_PIN, LOW);
  Serial.println("Stop lambası hazır (D1)");
  
  // Ön farlar (başlangıçta kapalı)
  pinMode(HEADLIGHT_PIN, OUTPUT);
  digitalWrite(HEADLIGHT_PIN, LOW);
  Serial.println("Ön farlar hazır (D2)");

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi: ");
  Serial.print(WIFI_SSID);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" HATA!");
  }

  // OTA (Over-The-Air) güncelleme
  ArduinoOTA.setHostname("RC-Car");
  ArduinoOTA.setPassword("20223BFab*");
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("OTA Basladi: " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Tamamlandi");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Hata[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA aktif - Hostname: RC-Car");

  // HTTP yollar
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/servo", HTTP_GET, handleServo);
  server.on("/api/mosfet", HTTP_GET, handleMosfet);
  server.on("/api/brake", HTTP_GET, handleBrake);
  server.on("/api/headlight", HTTP_GET, handleHeadlight);
  server.on("/api/stoplight", HTTP_GET, handleStopLight);
  server.on("/api/version", HTTP_GET, handleVersion);
  server.begin();
  Serial.println("HTTP sunucu basladi");
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
}