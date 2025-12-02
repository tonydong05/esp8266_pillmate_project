//----------------------------------------------------
// LOLIN D32 ESP32-WROOM-32E & UNO R3
// 2025.11.28
//----------------------------------------------------

const int D32_chk = A3; // 스위치 연결 핀
const int D32_RST_SW = A2; // D32 RST
int D32_RST = 6 ;


void setup() 
  {
    pinMode(D32_chk, INPUT_PULLUP);
    pinMode(D32_RST_SW, INPUT_PULLUP);  // D32 reset

    Serial.begin(115200);               // D32와 연결
    Serial.println("UNO ready");
    pinMode(D32_RST, OUTPUT);
    Serial.println("LOLIN D32 RESET");
    digitalWrite(D32_RST,LOW);
    delay(150);
    digitalWrite(D32_RST,HIGH);
  }

void loop() 
  {
  if (digitalRead(D32_chk) == LOW)
    {  // 스위치 ON : D32로 'chk' 전송
      Serial.println("chk");
      delay(300);
    }
  if (Serial.available()) 
    { // D32에서 오는 메시지 수신
      String msg = Serial.readStringUntil('\n');
      msg.trim();           // 공백 제거  
      Serial.print("From D32 : ");
      Serial.println(msg);
    }
   if (digitalRead(D32_RST_SW) == LOW)
    {  // 스위치 ON : D32로 'chk' 전송
      digitalWrite(D32_RST,LOW);
      delay(150);
      digitalWrite(D32_RST,HIGH);
    }
  }
