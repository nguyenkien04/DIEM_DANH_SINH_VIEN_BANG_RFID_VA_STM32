
#include <HardwareSerial.h>
#include <stdio.h>
#include <string.h>
#define rxPin 5
#define txPin 18
//khởi tạo chân của uart 

#include "WiFi.h"
#include <HTTPClient.h>
const char* ssid = "._.";  //--> Your wifi name
const char* password = "trungkien";

const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 7 * 3600;  // GMT+7
const int   daylightOffset_sec = 0;    
//

// Khởi tạo HardwareSerial cho UART1
HardwareSerial MySerial(1);

String dataToSend = " ";
String payload = " ";
uint16_t len = 0 ;
String Send_Data_URL = " ";



void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  Serial.println("-------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  MySerial.begin(115200, SERIAL_8N1, rxPin, txPin);
   if (MySerial) {
    Serial.println("UART1 đã khởi động thành công!");
  } else {
    Serial.println("Lỗi khởi động UART1!");
  }  
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(500);  // Chờ cập nhật thời gian từ NTP

  

}

void loop() {

  if (MySerial.available()>0){ 
    Send_Data_URL = " ";
    Send_Data_URL = MySerial.readString();


    if(Send_Data_URL.equals("time")) {
      send_current_time();
    }else{
    Serial.println("data from stm32: " + Send_Data_URL);
    writeLogSheet();
  
    }

  }

}


void uartToStm32(String data){
  dataToSend = "";
  dataToSend = data ;
  len = dataToSend.length();// Độ dài chuỗi (số byte)
  
  // Gửi độ dài trước (2 byte)
  MySerial.write((uint8_t*)&len, sizeof(len));
  delay(3000);
  MySerial.print(dataToSend); 
  Serial.println("data to stm32 ");

}

void send_current_time() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    char timeStr[20]; 
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    Serial.print("Sent time: ");
    Serial.println(timeStr);
    
    MySerial.print(timeStr);
}

void removeTrailingCommas(String &str) {
    while (str.length() > 0 && str.charAt(str.length() - 1) == ',' ) {
        str.remove(str.length() - 1);  // Xóa ký tự cuối cùng
    }
}

void writeLogSheet(){
    Serial.println();
    Serial.println("-------------");
    Serial.println("Send data to Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Send_Data_URL);


    // Initialize HTTPClient as "http".
    // ghi dữ liệu từ datasheet và kiểm tra 
    //Tạo một đối tượng HTTPClient để thực hiện các yêu cầu HTTP (GET, POST, PUT, v.v.).
    HTTPClient http;

    // HTTP GET Request.

    http.begin(Send_Data_URL.c_str());//Mở kết nối tới URL được cung cấp
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
// Đặt chế độ tự động theo dõi chuyển hướng HTTP (301, 302)
   
    // Gửi yêu cầu HTTP GET tới server
    int httpCode = http.GET(); 
    //Lưu mã trạng thái HTTP trả về từ server (ví dụ: 200, 404, 500...).
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from google sheets.

    payload = "";
    if (httpCode > 0) {
      payload = http.getString();
      //Lấy dữ liệu trả về từ server dưới dạng chuỗi.
      Serial.println("Payload : " + payload);    
    }

    http.end();
    delay(100);
}




