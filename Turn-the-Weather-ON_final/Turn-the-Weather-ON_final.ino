#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include <RTClib.h>

// define time


// define input


// define output
#define DATA_LEN 6
#define WEATHER_PIN   // !!! --- pinNUM
#define WEATHER_LEN 12
#define WEATHER_NUM 6
#define SKY_PIN 0     // !!! --- pinNum
#define SKY_LEN 0
#define CLOCK_PIN 0 
#define CLOCK_LEN 8   // !!! --- pinNum



// WiFi, Server
char ssid[] = "HYDORM-522"; // WiFi id
char pass[] = "vlzja0524"; // WiFi pw
char host[] = "www.kma.go.kr";
String url = "/wid/queryDFSRSS.jsp?zone=";
String zone = "4127152500"; // 안산시 상록구 사동 지역코드
WiFiServer server(80);
WiFiClient client;
IPAddress hostIp;

// Time Data
RTC_DS1307 RTC;
int now_hour = 0;
int last_hour = 0;
int now_minute = 0; // for compile
int now_month = 0;
int now_day = 0;

// Input Data
int now_input = 0;
int last_input = 0;
int hour_index = 0;
int input_day = 0;

// Weather Data
String datetime = "";
int i_hour = 0, i_day = 1, i_temp = 2, i_sky = 3, i_pty = 4, i_reh = 5;
String tags[DATA_LEN][2] = {{"<hour>","</hour>"}, {"<day>","</day>"}, {"<temp>", "</temp>"}, {"<sky>","</sky>"}, {"<pty>","</pty>"}, {"<reh>", "</reh>"}};
String data[8][DATA_LEN] = {{"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""},
                            {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}};
// Output
int output_sky = 0;
int output_pty = 0;
Adafruit_NeoPixel weather_strip = Adafruit_NeoPixel(WEATHER_PIN, WEATHER_LEN * WEATHER_NUM, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel sky_strip = Adafruit_NeoPixel(SKY_PIN, SKY_LEN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel clock_strip = Adafruit_NeoPixel(CLOCK_PIN, CLOCK_LEN, NEO_GRB + NEO_KHZ800);
















void setup() {
  Serial.begin(115200);

  // set input

  // set output

  // set data
  connectToWiFi();
  connectToServer();
  parseWeatherData();
}

void loop() {
  // put your main code here, to run repeatedly:

}














// Data
void parseWeatherData()
{
  if(client.available()){
//    Serial.println("\n\n--now parsing weather data\n\n");
    int index_now =0;
    int index_count = 0;
    if (client.connected())
    {
      Serial.print(".");
      while(index_now<8) // 이쪽은 횟수가 아닌 특정 조건으로 반복해야 함!!
      {
        index_count = 0;
        if(client.available())
        {
          // get line
          String line = client.readStringUntil('\n');
//          Serial.println("" + line + "\n");
  
          // get datetime
          if (line.indexOf("</tm>")>0)
            datetime = line.substring(line.indexOf("<tm>")+4, line.indexOf("</tm>"));
   
          // get weather info by hour
          for(int index_count=0; index_count<DATA_LEN; index_count++) // 이쪽은 횟수 제한으로 반복.
          {
             tail = line.indexOf(tags[index_count][1]);
             if(tail>0)
             {            
                head = line.indexOf(tags[index_count][0]) + tags[index_count][0].length();
                data[index_now][index_count] = line.substring(head, tail);
                if (index_count==DATA_LEN-1) // 여기다.. 이 조건 안에서 플러스를 해줘야지..
                  index_now++;
//                Serial.println("\n---- " + line.substring(head, tail) + " is in data [" + String(index_now) + "][" + String(index_count) + "]\n"); // 확인용
             }
          }
         }
       }
    }
  }
}





// Input
int getHourIndex(int input_hour) // 18 : (15 ~ 18을 의미. 즉, 16이면 18에 들어가야 함.)
{
      int i = 0;
      int hour_area = input_hour - (input_hour % 3) + 3;
      for(int i=0; i<8; i++)
        if(data[i][i_hour].toInt() == hour_area) return i;
      return -1;
}





// Output
void showTodayHour()
{
  Serial.println("Today hour is " + String(now_hour) + " :00 ~ 24:00");
}

void colorOn()
{
  
}

void showWeather()
{
  
}

void showSky()
{
  
}
