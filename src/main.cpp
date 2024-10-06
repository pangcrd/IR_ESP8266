#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>    
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#define URL "http://www.google.com"

String ssid="";
String password="";

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
const uint16_t kRecvPin = 14; //Recommended: 14 (D5)

IRsend irsend(kIrLed); 
IRrecv irrecv(kRecvPin);

decode_results results;

WiFiUDP udp;
unsigned int localUdpPort = 9; //Local port
IPAddress remote_IP(255, 255, 255, 255);

const uint16_t tx = 1;  
const uint16_t rx = 3;

void httpClientRequest(){
  
  WiFiClient client; 
 
  //Tạo đối tượng HTTPClient
  HTTPClient httpClient;
 

  httpClient.begin(client,URL); 
  Serial.print("URL: "); Serial.println(URL);
 
  //Bắt đầu kết nối thông qua hàm GET và gửi yêu cầu HTTP
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);
  
  //Nếu máy chủ phản hồi HTTP_CODE_OK (200), hãy lấy thông tin nội dung phản hồi từ máy chủ và xuất thông qua cổng nối tiếp
  //Nếu máy chủ không phản hồi HTTP_CODE_OK (200), mã trạng thái phản hồi của máy chủ sẽ được xuất qua cổng nối tiếp.
  if (httpCode == HTTP_CODE_OK) {
    // Sử dụng hàm getString để lấy data phản hồi từ máy chủ
    String responsePayload = httpClient.getString();
    Serial.println("Server Response Payload: ");
    Serial.println(responsePayload);
  } else {
    Serial.println("Server Respose Code：");
    Serial.println(httpCode);
  }
 
  //Ngắt kết nối giữa ESP8266 và máy chủ
  httpClient.end();
}

void IR_UDP(){
    if (irrecv.decode(&results)) {
    // print() & println() can't handle printing long longs. (uint64_t)
    serialPrintUint64(results.value, HEX);
    uint8_t *p=(uint8_t *)&results.value;
    udp.beginPacket(remote_IP, localUdpPort); //Chuẩn bị gửi dữ liệu
    udp.write("IR");
    udp.write(*p);
    udp.write(*(p+1));
    udp.write(*(p+2));
    udp.write(*(p+3));
    udp.write(*(p+4));
    udp.write(*(p+5));
    udp.write(*(p+6));
    udp.write(*(p+7));    
    udp.endPacket();
  
    irrecv.resume();  // Receive the next value
  }
}
  

void UDP_IR() {
  // Kiểm tra mã IR truyền nhận
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Đảm bảo rằng độ dài gói dữ liệu không vượt quá 255
    if(packetSize > 255) {
      packetSize = 255;
    }

    // Đọc dữ liệu và in
    uint8_t packetBuffer[11]={0};
    int len = udp.read(packetBuffer, packetSize);

   if (len > 0) {
     packetBuffer[len] = '\0';
     //Serial.println(packetBuffer);
   }  

    uint64_t nec=0;
    uint8_t *p1=(uint8_t *)&packetBuffer[0];
    uint8_t *p2=(uint8_t *)&nec;
    if(*p1==0x49 && *(p1+1)==0x52)
    {
        Serial.println("NEC");
        if(*(p1+5) ==0x00)    //Mã hóa mã NEC
        {
          *p2    =*(p1+2);
          *(p2+1)=*(p1+3);
          *(p2+2)=*(p1+4);
          *(p2+3)=*(p1+5);
  
        }
        else if(*(p1+5)==0xff)
          nec=0xFFFFFFFFUL; //irsend.sendNEC(0x00FFE01FUL); lệnh send mã NEC
    }
    irrecv.pause();  // Pause collection of received IR data.
    //0x00FF12EDUL :phần bổ sung của người dùng (thường là 0xFF) + UL
    irsend.sendNEC(nec);
    irrecv.resume();  // Receive the next value
 }
}

void setup() {
  WiFiManager wifiManager;

//Jump TX RX để xóa mật khẩu WIFI, để cấu hình lại
  pinMode(tx, OUTPUT);
  digitalWrite(tx, LOW);   
  pinMode(rx, INPUT_PULLUP);
  if (digitalRead(rx) == LOW) 
  {
    delay(100);
    if (digitalRead(rx) == LOW) 
       wifiManager.resetSettings();
  }

  Serial.begin(115200);
  
  // Tạo đối tượng WiFiManager
  // Tự động kết nối với WiFi.Và đặt tên WiFi
  wifiManager.autoConnect("IR_WIFI");

  Serial.println("\nWiFi connect succcess!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());     
  Serial.print("gateway address: ");
  Serial.println(WiFi.gatewayIP());   
  httpClientRequest();      

  udp.begin(9);                      // Local port
  Serial.println("UDP started");    
  
  irsend.begin();                   // Gửi mã IR
  irrecv.enableIRIn();              // Thu mã IR

}

void loop() {
  UDP_IR();
  IR_UDP();
}
