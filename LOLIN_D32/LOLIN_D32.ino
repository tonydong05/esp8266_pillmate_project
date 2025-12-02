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
  if (Serial2.available()) 
    { // UNO에서 메시지 수신 확인
      String msg = Serial2.readStringUntil('\n');
      msg.trim();  // 공백 제거

      Serial.println("[UNO → ESP32] " + msg);

      // 1) chk 요청 → 오늘 약 정보 보내기
      if (msg == "chk") {
        sendDoseInfoToUNO();
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
    // printToUNO("dose_id:" + String(doses[i].dose_id));
    printToUNO("name:" + doses[i].name);
    printToUNO("time:" + doses[i].time);
    // printToUNO("taken:" + String(doses[i].is_taken));
    printToUNO("---"); // 구분용
  }

  Serial.println("→ 모든 약 정보를 UNO로 전송완료");
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
