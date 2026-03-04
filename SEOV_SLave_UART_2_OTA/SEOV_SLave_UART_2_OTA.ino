#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <HTTPUpdate.h>

#define EEPROM_SIZE 128
// Ví dụ sử dụng UART2 trên ESP32 với GPIO26 (RX) và GPIO27 (TX) 
#define RXD2 26 
#define TXD2 27

#define ME 25
#define PE 33
#define IT 32
#define QC 35
#define SAFETY 34

#define LED_ME 17
#define LED_PE 16
#define LED_IT 4
#define LED_QC 2
#define LED_SAFETY 15

int rssi = 0;
int deviceID_int = 1;
String timeout = "";
int httpResponseCode = 0;
WebServer server(80);
String ssid = "";
String password = "";
String deviceID = "";
String ServerIP = "";
String Status = "STT";
unsigned long cycle;
unsigned long startMillis[5] = {0}; 
unsigned long pressStart[5] = {0}; 
const unsigned long interval = 500; // nhấp nháy 0.5s
unsigned long previousMillis = 0;
// ESP32 ID
const int esp32_id = 2;
// Khai báo LCD: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(5, 18, 19, 21, 22, 23);
// Nút nhấn
const int buttonPins[5] = {ME, PE, IT, QC, SAFETY};
// Đèn
const int ledPins[5] = {LED_ME, LED_PE, LED_IT, LED_QC, LED_SAFETY};   // 5 chân LED

// Trạng thái cho từng nút (0,1,2)
int states[5] = {0,0,0,0,0};

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<style>";
  html += "body { font-size: 48px; font-family: Arial; }";   // chữ to hơn
  html += "input { font-size: 44px; margin: 8px 0; }";       // input cũng to
  html += "</style></head><body>";
  
  html += "<h1>Device Setting</h1>";
  html += "<form action='/save' method='POST'>";
  html += "Device ID: <input type='text' name='id' value='" + deviceID + "'><br>";
  html += "WiFi SSID: <input type='text' name='ssid' value='" + ssid + "'><br>";
  html += "Password: <input type='text' name='pass' value='" + password + "'><br>"; // không ẩn mật khẩu nữa
  html += "Server IP: <input type='text' name='serverip' value='" + ServerIP + "'><br>"; // không ẩn mật khẩu nữa
  html += "<input type='submit' value='Save'>";
  html += "</form>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  deviceID = server.arg("id");
  ssid = server.arg("ssid");
  password = server.arg("pass");
  ServerIP = server.arg("serverip");
  // Lưu vào EEPROM
  EEPROM.writeString(0, deviceID);
  EEPROM.writeString(32, ssid);
  EEPROM.writeString(64, password);
  EEPROM.writeString(96, ServerIP);
  EEPROM.commit();
  server.send(200, "text/html", "<h1>Saved! Restart ESP32</h1>");
  ESP.restart();
}
void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info) {
  switch (event) {
    case WIFI_EVENT_STA_DISCONNECTED:
      Serial.println("⚠️ WiFi bị ngắt, thử kết nối lại...");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("WiFi's disconnected");
      lcd.setCursor(0,1);
      lcd.print("Re-connecting...");
      delay(1000);
      WiFi.begin(ssid.c_str(), password.c_str());
      break;
  }
}

void setup() {
  for (int i=0; i<5; i++) pinMode(buttonPins[i], INPUT);

  lcd.begin(16, 2);
  // Đọc dữ liệu EEPROM trước
  EEPROM.begin(EEPROM_SIZE);
  deviceID = EEPROM.readString(0);
  ssid     = EEPROM.readString(32);
  password = EEPROM.readString(64);
  ServerIP = EEPROM.readString(96);

  // Nếu rỗng thì gán "Nodata"
  if (deviceID == "") deviceID = "1";
  if (ssid == "")     ssid     = "Anh Linh";
  if (password == "") password = "08032025";
  if (ServerIP == "") ServerIP = "192.168.0.1/5000";
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Press PE");
  lcd.setCursor(0,1); //Cột - Dòng
  lcd.print("To re-config");
  delay(2000);

  if (digitalRead(ME) == LOW) 
  {
    lcd.setCursor(0,0);
    lcd.print("ESP32:12345678");
    lcd.setCursor(0,1);
    lcd.print("IP:192.168.4.1");
    WiFi.softAP("ESP32", "12345678");
    // Serial.println("⚠️ Vào chế độ cấu hình (CONFIG MODE)");
    // Serial.println("AP IP: " + WiFi.softAPIP().toString());
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    while(1)
    {
      server.handleClient();
    }
  }


  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Press ME");
  lcd.setCursor(0,1);
  lcd.print("To change ID");
  delay(2000);

  if(digitalRead(PE)==LOW)
  {
    lcd.clear();
    deviceID_int = deviceID.toInt();
    while(digitalRead(IT)==HIGH)
    {
      lcd.setCursor(0,0);
      lcd.print(" QC++  SAFETY--");
      lcd.setCursor(0,1);
      lcd.print("ID:");
      lcd.print(deviceID_int);
      lcd.print("   ");
      if(digitalRead(QC) == LOW)
      {
        delay(200);
        deviceID_int++;
      }
      if(digitalRead(SAFETY) == LOW)
      {
        delay(200);
        deviceID_int--;
        if(deviceID_int < 0)
        {
          deviceID_int = 0;
        }
      }
    }
    deviceID = String(deviceID_int);
    EEPROM.writeString(0, deviceID);
    EEPROM.commit();
    ESP.restart();
    lcd.clear();
    while(digitalRead(IT)==LOW)
    {
      lcd.setCursor(0,0);
      lcd.print("Succesfully...");
      lcd.setCursor(0,1);
      lcd.print("ID:");
      lcd.print(deviceID_int);
      lcd.print("   ");
    }
    delay(1000);
  }
  // Kiểm tra chân 34
  WiFi.onEvent(WiFiEvent);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  for (int i = 0; i < 5; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) 
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WIFI:");
    lcd.print(ssid);
    lcd.setCursor(0, 1);
    lcd.print("ID:"+ deviceID + " ");
    delay(2000);
    for (int i = 0; i < 5; i++) 
    {
      digitalWrite(ledPins[i], HIGH);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Server IP:");
    lcd.setCursor(0, 1);
    lcd.print(ServerIP);
    delay(2000);
    for (int i = 0; i < 5; i++) 
    {
      digitalWrite(ledPins[i], LOW);
    }
    update_by_Serial();
  }
  for (int i = 0; i < 5; i++) 
  {
    digitalWrite(ledPins[i], LOW);
  }
  lcd.clear();
  rssi = WiFi.RSSI(); // đơn vị dBm
}

void loop() 
{
  for (int i = 0; i<5; i++) 
  {
    // --- Nhấn lần đầu ---
    if ((digitalRead(buttonPins[i]) == LOW) && (states[i] == 0) )
    {
      digitalWrite(ledPins[i], HIGH);
      states[i] = 1;
      delay(100);
      while(digitalRead(buttonPins[i]) == LOW);
      delay(500);
    }
    // ---- Cố gắng ghi 1 lên server ----
    if (states[i] == 1)
    {
      if (sendUpdate(i, 1) == true) 
      {
        states[i] = 2;
      }
    }
    // --- Nhấn lần thứ 2 ---
    if ((digitalRead(buttonPins[i]) == LOW) && (states[i] == 2)) 
    {
      digitalWrite(ledPins[i], LOW);
      delay(100);
      if (digitalRead(buttonPins[i]) == LOW ) 
      {
        states[i] = 3;
        // Lấy cường độ tín hiệu WiFi
        rssi = WiFi.RSSI(); // đơn vị dBm
        delay(100);
        while(digitalRead(buttonPins[i]) == LOW);
        delay(500);
      }
    }

    // ---- Cố gắng ghi 2 lên server ----
    if (states[i] == 3)
    {
      if (sendUpdate(i, 2) == true) 
      {
        states[i] = 4;
      }
    }

    // --- Nhấn lần thứ 3 và giữ > 1s ---
    if ((digitalRead(buttonPins[i]) == LOW) && (states[i] == 4)) 
    {
      delay(1000);
      if (digitalRead(buttonPins[i]) == LOW) 
      {
        digitalWrite(ledPins[i], HIGH);
        // Lấy cường độ tín hiệu WiFi
        rssi = WiFi.RSSI(); // đơn vị dBm
        states[i] = 5;
        delay(100);
        while(digitalRead(buttonPins[i]) == LOW);
        delay(500);
      }
    }

    // ---- Cố gắng ghi 0 lên server ----
    if (states[i] == 5)
    {
      if (sendUpdate(i, 0)  == true ) 
      {
        states[i] = 0;
      }   
    }
  }
  // --- Điều khiển LED ---
  for (int i=0; i<5; i++) 
  {
    if (states[i] == 0) 
    {
      digitalWrite(ledPins[i], LOW);
    } 
    else if ((states[i] == 1) || (states[i] == 2)) 
    {
      digitalWrite(ledPins[i], HIGH);
    }
    else if ((states[i] == 3) ||  (states[i] == 4))
    {
      // Lấy số nguyên sau khi chia cho 5000
      unsigned long cycle = (millis() /500);
      if (cycle % 2 == 0) 
      {
        digitalWrite(ledPins[i], HIGH);  // chẵn → LED sáng
      } 
      else 
      {
        digitalWrite(ledPins[i], LOW);   // lẻ → LED tắt
      } 
    }
    else
    {
      digitalWrite(ledPins[i], LOW);
    }
  }
  update_by_Serial(); 
  String Wifi_stt = "WIFI:" + String(rssi) + " RP:" + String(httpResponseCode);
  String Button_stt = "ID:" + deviceID + "  STT:" ;
  for (int i = 0; i < 5; i++) 
  { 
    Button_stt += String(states[i]); 
  }
  lcd.setCursor(0, 0);
  lcd.print(Button_stt);
  lcd.setCursor(0, 1);
  lcd.print(Wifi_stt);
}

void update_by_Serial()
{
  if (Serial2.available()) 
  {
    String input = Serial2.readStringUntil('\n');
    input.trim();

    // Nếu nhận "Data ?"
    if (input.equalsIgnoreCase("Data?")) 
    {
      String packet = deviceID + ";;" + ssid + ";;" + password + ";;" + ServerIP;
      Serial2.println(packet);
    }
    else 
    {
      // Nhận gói tin từ ứng dụng
      // Ví dụ: "ESP32_001;;MyWiFi;;12345678;;192.168.1.100/5000"
      String fields[4];
      int index = 0;
      int lastPos = 0;
      while (true) 
      {
        int pos = input.indexOf(";;", lastPos);
        if (pos == -1) {
          fields[index++] = input.substring(lastPos);
          break;
        } else {
          fields[index++] = input.substring(lastPos, pos);
          lastPos = pos + 2;
          if (index >= 5) break;
        }
      }

      // Kiểm tra đủ 5 trường
      if (index == 4) 
      {
        deviceID = fields[0];
        ssid     = fields[1];
        password = fields[2];
        ServerIP = fields[3];
        EEPROM.writeString(0, deviceID);
        EEPROM.writeString(32, ssid);
        EEPROM.writeString(64, password);
        EEPROM.writeString(96, ServerIP);
        EEPROM.commit();
        Serial2.println("Setup thành công");
        // ESP.restart();
      } 
      else 
      {
        Serial2.println("Thiếu thông tin");
      }
    }
  }
}

bool sendUpdate(int buttonIndex, int value) {
  if (WiFi.status() == WL_CONNECTED) 
  {
    HTTPClient http;
    String url = "http://" + ServerIP + "/update";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Tạo JSON body
    String jsonBody = "{\"cell\":\"" + deviceID + "_" + String(buttonIndex) + "\",\"value\":" + String(value) + "}";

    // Gửi POST request
    httpResponseCode= http.POST(jsonBody);

    if (httpResponseCode > 0) {
      Serial.printf("Mã phản hồi HTTP: %d\n", httpResponseCode);
      if (httpResponseCode == 200) {
        Serial.println("✅ Thành công (200 OK)");
      } else if (httpResponseCode == 400) {
        Serial.println("⚠️ Lỗi request (400 Bad Request)");
      } else if (httpResponseCode == 404) {
        Serial.println("❌ Không tìm thấy route (404 Not Found)");
      } else if (httpResponseCode == 500) {
        Serial.println("💥 Lỗi server (500 Internal Server Error)");
      } else {
        Serial.println("Mã phản hồi khác: " + String(httpResponseCode));
      }

      String payload = http.getString();
      Serial.println("Phản hồi từ server: " + payload);

      // Parse JSON phản hồi
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        const char* status = doc["status"];
        const char* cell = doc["cell"];
        int val = doc["value"];

        Serial.printf("Server trả về: status=%s, cell=%s, value=%d\n", status, cell, val);

        if (strcmp(status, "OK") == 0) {
          Serial.println("Xác nhận: server đã xử lý thành công");
          http.end();
          return true;
        } else {
          Serial.println("Server báo lỗi!");
        }
      } else {
        Serial.println("Lỗi parse JSON: " + String(error.c_str()));
      }
    } else {
        Serial.printf("Lỗi HTTPClient: %d\n", httpResponseCode);
        switch (httpResponseCode) {
          case -1:  Serial.println("HTTPC_ERROR_CONNECTION_REFUSED - Server từ chối kết nối hoặc không chạy"); break;
          case -2:  Serial.println("HTTPC_ERROR_SEND_HEADER_FAILED - Gửi header thất bại"); break;
          case -3:  Serial.println("HTTPC_ERROR_SEND_PAYLOAD_FAILED - Gửi dữ liệu thất bại"); break;
          case -4:  Serial.println("HTTPC_ERROR_NOT_CONNECTED - Chưa kết nối"); break;
          case -5:  Serial.println("HTTPC_ERROR_CONNECTION_LOST - Mất kết nối trong khi gửi/nhận"); break;
          case -6:  Serial.println("HTTPC_ERROR_NO_STREAM - Không có stream dữ liệu"); break;
          case -7:  Serial.println("HTTPC_ERROR_NO_HTTP_SERVER - Không tìm thấy server HTTP"); break;
          case -8:  Serial.println("HTTPC_ERROR_TOO_LESS_RAM - Không đủ RAM để xử lý"); break;
          case -9:  Serial.println("HTTPC_ERROR_ENCODING - Lỗi mã hóa dữ liệu"); break;
          case -10: Serial.println("HTTPC_ERROR_STREAM_WRITE - Ghi stream thất bại"); break;
          case -11: Serial.println("HTTPC_ERROR_READ_TIMEOUT - Server không phản hồi (timeout)"); break;
          default:  Serial.println("Lỗi không xác định"); break;
        }
    }

    http.end();
  } 
  else 
  {
    Serial.println("Thất bại do không thể kết nối WiFi");
  }

  return false;
}

// ---Bước 0: chưa ấn gì, đèn tắt
// ---Bước 1: Sau khi ấn nút lần 1, đèn sáng
// ---Bước 2: Sau khi ghi thành công 1 lên Server, đèn sáng
// ---Bước 3: Sau khi ấn nút lần 2, đèn nhấp nháy
// ---Bước 4: Sau khi ghi thành công 2 lên Server, đèn nhấp nháy
// ---Bước 5: Sau khi ấn nút lần 3, đèn tắt
// ---Bước 6: Sau khi ghi thành công 0 lên Server, đèn tắt -> thực tế là về bước 0
// ---Bước 7:






