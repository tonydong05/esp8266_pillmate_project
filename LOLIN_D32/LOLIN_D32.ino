//----------------------------------------------------
// LOLIN D32 ESP32-WROOM-32E
// 2025.11.28
//----------------------------------------------------

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid   = "세인의 S24";    //"SSID";
const char* password = "sour678!";  //"PASSWORD";

String BASE_URL = "https://sein0327.shop";

//json 파싱 정보 변수 선언
struct DoseItem {
  int dose_id;
  String name;
  String time;
  bool is_taken;
};

DoseItem doses[10];   // 최대 10개 저장
int doseCount = 0;

//마그네틱 센서 핀
// const int MAGNET_PIN = 12; -> 주석 해제 필요
const int ledPin = 9;      // LED
const int buzzerPin = 8;   // 부저

// int prevMagState = HIGH;


void setup() 
  {
    Serial.begin(115200);        // USB 모니터용
    Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17

    // Wi-Fi 연결
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    print_ToBoth("Connecting to Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) 
      {
        delay(500);
        print_ToBoth(".");
      }
    print_ToBoth("\nConnected!");
    print_ToBoth("D32 IP: " + WiFi.localIP().toString());

    // 서버에서 오늘의 약 정보 가져오기
    getTodayDose();
  }


void loop()
{
    // UNO → ESP32 메시지
    if (Serial2.available()) 
    {
        String msg = Serial2.readStringUntil('\n');
        msg.trim();

        Serial.println("[UNO → ESP32] " + msg);

        // 1) UNO가 chk 보내면 → 약 정보 전달
        if (msg == "chk") {
            sendDoseInfoToUNO();
        }

        // 2) UNO가 confirm 보내면 → 서버로 POST
        if (msg.startsWith("confirm")) {
            Serial.println("[UNO → ESP32] confirm 수신 → 서버 전송 시작");
            confirmDose();
        }
    }

    // 🔍 PC 시리얼 테스트용
    if (Serial.available()) {
        String s = Serial.readStringUntil('\n');
        s.trim();
        if (s == "confirm") {
            Serial.println("🔔 시리얼 명령으로 테스트!");
            confirmDose();
        }
    }
}


//----------------------------------------------
// 오늘 약 정보 서버에서 GET
//----------------------------------------------
void getTodayDose() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = BASE_URL + "/medicine/arduino/today-dose/";

  Serial.println("📡 GET " + url);
  http.begin(url);

  int httpCode = http.GET();
  Serial.printf("→ HTTP %d\n", httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Response:");
    Serial.println(payload);

    StaticJsonDocument<2048> doc;  
    deserializeJson(doc, payload);

    JsonArray arr = doc["doses"];
    doseCount = arr.size();

    Serial.println("📌 약 개수: " + String(doseCount));

    for (int i = 0; i < doseCount; i++) {
      JsonObject item = arr[i];

      doses[i].dose_id   = item["dose_id"];
      doses[i].name      = item["name"].as<String>();
      doses[i].time      = item["alarm_time"].as<String>();
      doses[i].is_taken  = item["is_taken"];

      Serial.println("---- dose #" + String(i));
      Serial.println(" id: " + String(doses[i].dose_id));
      Serial.println(" name: " + doses[i].name);
      Serial.println(" time: " + doses[i].time);
      Serial.println(" taken: " + String(doses[i].is_taken));
    }

  } else {
    Serial.println("❌ GET 실패");
  }

  http.end();
}


//----------------------------------------------
// UNO 요청 시 약 정보 전달
//----------------------------------------------
void sendDoseInfoToUNO() {
  for (int i = 0; i < doseCount; i++) {
    if (doses[i].is_taken == true) continue;  // 🔥 먹은 약은 스킵
    // printToUNO("dose_id:" + String(doses[i].dose_id));
    printToUNO("name:" + doses[i].name);
    printToUNO("time:" + doses[i].time);
    // printToUNO("taken:" + String(doses[i].is_taken));
    printToUNO("---"); // 구분용
  }

  Serial.println("→ 모든 약 정보를 UNO로 전송완료");
}

//----------------------------------------------
// POST 복용완료 전송
//----------------------------------------------
void confirmDose() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return;
  }

  int idx = getFirstNotTakenDose();

  if (idx == -1) {
    Serial.println("❌ 오늘 남은 약 없음");
    return;
  }

  int sendDoseId = doses[idx].dose_id;

  HTTPClient http;
  String url = BASE_URL + "/medicine/arduino/confirm/";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"dose_id\":" + String(sendDoseId) + "}";

  Serial.println("📡 POST confirm → " + body);

  int httpCode = http.POST(body);
  Serial.printf("→ POST HTTP %d\n", httpCode);

  if (httpCode == 200 || httpCode == 201) {
    Serial.println("✅ 복약 완료 서버 반영 성공");
    doses[idx].is_taken = true;   // 상태 업데이트
  } else {
    Serial.println("❌ 복약 완료 반영 실패");
  }

  http.end();
}


int getFirstNotTakenDose() {
  for (int i = 0; i < doseCount; i++) {
    if (doses[i].is_taken == false) {
      return i;  // index 반환
    }
  }
  return -1; // 없음
}



//----------------------------------------------
// Helper: UNO로 메시지 보내기
//----------------------------------------------
void printToUNO(String msg) {
  Serial2.println(msg);
  Serial.println("[ESP32 → UNO] " + msg);
}

void print_ToBoth(String msg) 
  {   // 시리얼 모니터와 UNO 모두로 출력
    Serial.println(msg);
    Serial2.println(msg);
}
