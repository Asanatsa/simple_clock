#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include <U8g2lib.h>
#include <RTClib.h>
#include "u8g2_font_unifont_t_cjk.h"

#define SCROLL_MODE 0

#define DISPLAY_TYPE 0
#define FLUSH_DELAY 30
#define U8LOG_WIDTH 12
#define U8LOG_HEIGHT 7
#define DISPLAY_WIDTH 128
#define MESSAGE_Y 60

#if SCROLL_MODE
#define STANDBY_TIME 30
#else
#define STANDBY_TIME 30
#endif

#define NTP1  "ntp1.aliyun.com"
#define NTP2  "ntp2.aliyun.com"
#define NTP3  "ntp3.aliyun.com"

uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];


#if DISPLAY_TYPE
//U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);
U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 12, /* data=*/ 14, /* CS=*/ 27, /* reset=*/ 13);
#else
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ SS, /* dc=*/ 17, /* reset=*/ U8X8_PIN_NONE);
#endif
RTC_DS1307 rtc;

AsyncWebServer server(80);
U8G2LOG u8g2log;


const char* ssid = "TP-202";
const char* password = "qaqll4375";

const char* PARAM_MESSAGE = "message";

String displayText = "永远相信美好的事情即将发生";
struct tm ntp_time;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "前面的区域以后再来探索吧");
}

String timeStr() {
  String hourt, minutet, secondt;
  DateTime now = rtc.now();
  if (now.hour() < 10) {
    hourt = "0" + String(now.hour());
  } else {
    hourt = String(now.hour());
  }

  if (now.minute() < 10) {
    minutet = "0" + String(now.minute());
  } else {
    minutet = String(now.minute());
  }

  if (now.second() < 10) {
    secondt = "0" + String(now.second());
  } else {
    secondt = String(now.second());
  }

  String timet = hourt+":"+minutet+":"+secondt;
  return timet;
}

String dateStr() {
  DateTime now = rtc.now();
  String datet = String(now.year())+"/"+String(now.month())+"/"+String(now.day());
  return datet;
}

void display(String text) {

  u8g2.firstPage();
  do {
    String timet = timeStr();
    String datet = dateStr();
    u8g2.setFont(u8g2_font_luBS18_tn); // 18px
    u8g2.drawStr(1, 20, timet.c_str());

    u8g2.setFont(u8g2_font_DigitalDisco_tn); // 10px
    u8g2.drawStr(1, 40, datet.c_str());

#if SCROLL_MODE
    u8g2.setFont(u8g2_font_unifont_t_cjk);
    int textWidth = u8g2.getUTF8Width(text.c_str());
    int offsetLenght = DISPLAY_WIDTH-textWidth;
    static int offset = 0;
    static bool forward = true;
    static int standby = 0;
    if(textWidth > DISPLAY_WIDTH) {
      if(forward){
        if(offsetLenght-offset >= 0) {
          if(standby >= STANDBY_TIME){
            forward = false;
          }else {
            standby++;
          }
          u8g2.drawUTF8(offset, MESSAGE_Y, text.c_str());
        }else {
          standby = 0;
          u8g2.drawUTF8(offset, MESSAGE_Y, text.c_str());
          offset -= 1;
        }
      }else {
        
        if(offset >= 0) {
          if(standby >= STANDBY_TIME){
            forward = true;
          }else {
            standby++;
          }
          u8g2.drawUTF8(offset, MESSAGE_Y, text.c_str());
        }else {
          standby = 0;
          u8g2.drawUTF8(offset, MESSAGE_Y, text.c_str());
          offset += 1;
        }
      }
    }else {
      u8g2.drawUTF8(2, MESSAGE_Y, text.c_str());
    }

#else
    static int offset = 0;
    static int standby = 0;
    u8g2.setFont(u8g2_font_unifont_t_cjk);
    int textWidth = u8g2.getUTF8Width(text.c_str());
    if(textWidth > DISPLAY_WIDTH){
      if(offset >= textWidth*-1) {
        if(offset == 0 && standby <= STANDBY_TIME) {
          u8g2.drawUTF8(offset, MESSAGE_Y, text.c_str());
          standby++;
        }else {
          standby = 0;
          u8g2.drawUTF8(offset, MESSAGE_Y, text.c_str());
          offset--;
        }
      }else {
        offset = DISPLAY_WIDTH;
      }
    }else {
      u8g2.drawUTF8(2, MESSAGE_Y, text.c_str());
    }

#endif


  } while ( u8g2.nextPage() );
}

void setup() {
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_unifont_t_cjk);	// set the font for the terminal window
    u8g2log.begin(u8g2, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
    u8g2log.setLineHeightOffset(0);	// set extra space between lines in pixel, this can be negative
    u8g2log.setRedrawMode(0);		// 0: Update screen with newline, 1: Update screen for every char  


    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        u8g2log.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    u8g2log.print("IP:");
    u8g2log.println(WiFi.localIP());


    if (!getLocalTime(&ntp_time)) {
      configTime(8 * 3600, 0, NTP1, NTP2,NTP3);
      getLocalTime(&ntp_time);
    }
    


    if (! rtc.begin()) {
    u8g2log.print("\f");
    u8g2log.print("RTC Err");
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(1000);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
  }
  
  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  rtc.adjust(DateTime(ntp_time.tm_year + 1900, ntp_time.tm_mon, ntp_time.tm_mday, ntp_time.tm_hour, ntp_time.tm_min, ntp_time.tm_sec));
  u8g2log.println("Time Seted");
  u8g2log.println("RTC ok");
  delay(1000);


    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "";
        }
        
        request->send(200, "text/plain", "data: " + message);
        displayText = message;
    });

    server.onNotFound(notFound);

    server.begin();
    delay(1000);
}

void loop() {
  display(displayText);
  delay(FLUSH_DELAY);
}
