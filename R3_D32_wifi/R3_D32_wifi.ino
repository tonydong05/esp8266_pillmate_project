#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================
// 핀 및 설정값
// =====================
#define MAGNET_PIN 12
#define MAGNET_OPEN LOW     // ⭐ LOW = 열림 (수정됨)

const int ledPin = 9;
const int buzzerPin = 8;

const int D32_chk = A3;
const int D32_RST_SW = A2;
const int D32_RST = 6;

// =====================
// 알람 시간 저장
// =====================
String doseTimes[10];
int doseCount = 0;

// =====================
// 알람 관련 변수
// =====================
bool alarmActive = false;
unsigned long nextBeepTime = 0;
unsigned long alarmStartTime = 0;

String activeDoseTime = "";
bool suppressLCD = false;
bool confirmSent = false;

// 센서 디바운스
int lastSensor = HIGH;
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 50;

// =====================
// 시간 포맷 정규화
// =====================
String normalizeTime(String raw) {
  int c = raw.indexOf(':');
  int h = raw.substring(0, c).toInt();
  int m = raw.substring(c + 1).toInt();

  char buf[6];
  sprintf(buf, "%02d:%02d", h, m);
  return String(buf);
}

// =====================
// LCD (알람 중에는 차단)
// =====================
void showLCD(String a, String b = "") {
  if (suppressLCD) return;
  lcd.clear();
  lcd.print(a);
  lcd.setCursor(0, 1);
  lcd.print(b);
}

// =====================
// 알람 시작
// =====================
void startAlarm(String t) {
  alarmActive = true;
  activeDoseTime = t;
  suppressLCD = true;
  confirmSent = false;

  alarmStartTime = millis();   // ⭐ 센서 1초 무시

  digitalWrite(ledPin, HIGH);

  tone(buzzerPin, 1000, 600);
  delay(600);
  noTone(buzzerPin);

  nextBeepTime = millis() + 60000;

  lcd.clear();
  lcd.print("EAT");
  lcd.setCursor(0, 1);
  lcd.print(t);

  Serial.println("[UNO] ALARM START");
}

// =====================
// 알람 종료
// =====================
void stopAlarm() {
  if (!alarmActive) return;

  alarmActive = false;
  suppressLCD = false;

  noTone(buzzerPin);
  digitalWrite(ledPin, LOW);

  lcd.clear();
  lcd.print("complete");

  Serial.println("[UNO] ALARM STOP");

  if (!confirmSent) {
    Serial.println("confirm");
    confirmSent = true;
  }

  delay(500);
}

// =====================
// ESP32 메시지 파싱
// =====================
void parseESP32Msg(String msg) {
  msg.trim();
  Serial.print("[ESP→UNO] ");
  Serial.println(msg);

  if (msg.startsWith("time:")) {
    String t = normalizeTime(msg.substring(5));
    doseTimes[doseCount++] = t;
    showLCD("time add:", t);
    return;
  }

  if (!alarmActive) {
    if (msg.startsWith("wifi:"))  showLCD(msg);
    if (msg.startsWith("ip:"))    showLCD("IP:", msg.substring(3));
  }
}

// =====================
// RTC 알람 체크 (수정됨)
// =====================
void checkRTCAlarm() {
  DateTime now = rtc.now();

  char buf[6];
  sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  String nowStr = buf;

  if (!alarmActive) {
    for (int i = 0; i < doseCount; i++) {

      // ⭐ 정확히 분이 같고 seconds == 0일 때만 알람
      if (doseTimes[i] == nowStr && now.second() == 0) {
        startAlarm(nowStr);
        return;
      }
    }
  }

  // 매 1분마다 부저 재울림
  if (alarmActive && millis() >= nextBeepTime) {
    tone(buzzerPin, 1000, 600);
    delay(600);
    noTone(buzzerPin);
    nextBeepTime = millis() + 60000;
  }
}

// =====================
// 마그네틱 센서 처리 (수정됨)
// =====================
void handleMagnet() {

  // ⭐ 알람 시작 후 1초간 센서 무시
  if (alarmActive && millis() - alarmStartTime < 1000) {
    return;
  }

  int reading = digitalRead(MAGNET_PIN);

  if (reading != lastSensor) {
    lastDebounce = millis();
  }

  if (millis() - lastDebounce > debounceDelay) {

    // ⭐ LOW일 때 열림으로 인식하도록 변경됨
    if (reading == MAGNET_OPEN) {
      if (alarmActive) {
        stopAlarm();
      }
    }
  }

  lastSensor = reading;
}

// =====================
// SETUP
// =====================
void setup() {
  Serial.begin(115200);

  pinMode(MAGNET_PIN, INPUT_PULLUP); // 그대로 사용
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(D32_chk, INPUT_PULLUP);
  pinMode(D32_RST_SW, INPUT_PULLUP);
  pinMode(D32_RST, OUTPUT);

  Wire.begin();
  rtc.begin();

  lcd.init();
  lcd.backlight();
  showLCD("UNO READY");
}

// =====================
// LOOP
// =====================
void loop() {
  // ESP32 → UNO msg
  if (Serial.available()) {
    String m = Serial.readStringUntil('\n');
    parseESP32Msg(m);
  }

  checkRTCAlarm();
  handleMagnet();

  if (digitalRead(D32_chk) == LOW) {
    Serial.println("chk");
    delay(100);
  }

  if (digitalRead(D32_RST_SW) == LOW) {
    digitalWrite(D32_RST, LOW);
    delay(100);
    digitalWrite(D32_RST, HIGH);
  }
}
