//----------------------------------------------------
// LOLIN D32 ESP32-WROOM-32E
// 2025.11.28
//----------------------------------------------------

#include <WiFi.h>

const char* ssid   = "A257AFh";    //"SSID";
const char* password = "!253192!";  //"PASSWORD";

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
  }

void loop()
  {
  if (Serial2.available()) 
    { // UNO에서 메시지 수신 확인
      String msg = Serial2.readStringUntil('\n');
      msg.trim();  // 공백 제거
      if (msg == "chk")
        { // chk 메시지를 받으면 출력
          print_ToBoth("Received 'chk' from UNO! Responding...");
        }
    }
  }

void print_ToBoth(String msg) 
  {   // 시리얼 모니터와 UNO 모두로 출력
    Serial.println(msg);
    Serial2.println(msg);
}
