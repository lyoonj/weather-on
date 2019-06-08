#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#define DATA_LEN 6
#define STRIP_LEN 12
#define WEATHER_LEN 7

// WiFi, Server
char ssid[] = "HYDORM-522"; // WiFi id
char pass[] = "vlzja0524"; // WiFi pw
char host[] = "www.kma.go.kr";
String url = "/wid/queryDFSRSS.jsp?zone=";
String zone = "4127152500"; // 안산시 상록구 사동 지역코드
WiFiServer server(80);
WiFiClient client;
IPAddress hostIp;

// Weather Data
String datetime = "";
int i_hour = 0, i_day = 1, i_temp = 2, i_sky = 3, i_pty = 4, i_reh = 5;
String tags[DATA_LEN][2] = {{"<hour>","</hour>"}, {"<day>","</day>"}, {"<temp>", "</temp>"}, {"<sky>","</sky>"}, {"<pty>","</pty>"}, {"<reh>", "</reh>"}};
String data[8][DATA_LEN] = {{"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""},
                            {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}};
int head, tail;

// Input
int volume = A0;
int volume_input = 0;
String input_date = ""; 
int input_hour = -1; // 설명 예제에선 LOW라서 byte를 사용하는데.. 여기도 byte를 사용해야하나?
int hour_index = -1;
String output_sky = "";
String output_pty = "";

// Output -> strip이 두 칸 모자라..... 남은 80cm로는 .... 둘레... 애매한데.....sky를 아예 둘레로? 그게 나을지도
Adafruit_NeoPixel weather_strip[WEATHER_LEN] = {Adafruit_NeoPixel(D7, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 하늘 배경
                                                Adafruit_NeoPixel(D6, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 1 맑음
                                                Adafruit_NeoPixel(D5, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 2 구름조금 --> LED 새로 오기 전까지 이거 일단 제외 
                                                Adafruit_NeoPixel(D4, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 3 구름많이
                                                Adafruit_NeoPixel(D3, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 4 흐림
                                                Adafruit_NeoPixel(D2, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 5 비 - 1
                                                Adafruit_NeoPixel(D1, STRIP_LEN, NEO_GRB + NEO_KHZ800)}; // 6 눈 - 4
Adafruit_NeoPixel sky_strip = weather_strip[0];                           
int clock_strip[8] = {6, 9, 12, 15, 18, 21, 24, 3};



















void setup() 
{
    Serial.begin(115200);
    
    // Input setting
    pinMode(volume, INPUT_PULLUP);
    // Output setting
    sky_strip.begin();
    sky_strip.show();
//    for(int i=0; i<WEATHER_LEN; i++){
//      weather_strip[i].begin();
//      weather_strip[i].show();
//    }

    // WiFi & Server setting
    connectToWiFi();
    connectToServer();
}


void loop()  // 문제점 : server에서 data를 게속 받아오면 안된다. 딜레이를 넣어야 한다. 근데 volume input에는 즉각 반응해야한다. delay를 하면서 volume 인풋에만 귀 기울이는 방법? -> interreupt를 사용해보자!
{
  // Server -> Data   [[if data changes.. -> 특정 시간대가 되면?
  parseWeatherData();

  
//  // Input -> Data
  volume_input = analogRead(volume);
  input_hour = map(volume_input, 4, 1024, 1, 24);
  hour_index = getHourIndex();
  output_sky = data[hour_index][i_sky];
  output_pty = data[hour_index][i_pty];

  // Data -> Output
  showInput(); // LCD (datetime)
//  showTodayHour(); // LED (clock)
  showSky(); // RGB gradation
  showWeather(output_sky.toInt(), output_pty.toInt()); // Panel on/off


  delay(1000000000); // 일단 10분 단위로 갱신
}




















// Wifi & Server
void connectToWiFi()
{
    Serial.println("\nConnecting to WiFi...");
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");  
    }
    Serial.println("WiFi Connected!");
    Serial.println("IP address : ");
    Serial.println(WiFi.localIP());
}

void connectToServer()
{
  
    server.begin();
    Serial.println("Connecting to Server...");
    Serial.println("Waiting for DHCP address...");
    while(WiFi.localIP() == INADDR_NONE) {
      Serial.print(".");
      delay(300);
    }
    Serial.println("\n");
    WiFi.hostByName("www.kma.go.kr", hostIp);
  
  if(client.connect(hostIp, 80))  
    {
      Serial.println("Server Connected!");
      client.print(String("GET ") + url + zone + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Connection: close\r\n\r\n");  

      
      int timeout = millis() + 5000;
      while(client.available() == 0)
      {
         if(timeout - millis() < 0)
         {
            Serial.println(">> Client Timeout!");
            client.stop();
            return;
         }  
      }  
    }
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
int getHourIndex() // 18 : (15 ~ 18을 의미. 즉, 16이면 18에 들어가야 함.)
{
      int i = 0;
      int hour_area = input_hour - (input_hour % 3) + 3;
      for(int i=0; i<8; i++)
        if(data[i][i_hour].toInt() == hour_area) return i;
      return -1;
}





// Output
void colorWipe(Adafruit_NeoPixel *strip, uint32_t c, uint8_t wait) {
  for (uint16_t i=0; i<(*strip).numPixels(); i++) {
    (*strip).setPixelColor(i, c);
    (*strip).show();
    delay(wait);
  }
}

void colorOn(Adafruit_NeoPixel *strip) {
  for (uint16_t i=0; i<(*strip).numPixels(); i++) {
    (*strip).setPixelColor(i, (*strip).Color(255, 255, 255));
    (*strip).show();
  }
}

void colorOff(Adafruit_NeoPixel *strip) {
  for (uint16_t i=0; i<(*strip).numPixels(); i++) {
    (*strip).setPixelColor(i, 0);
    (*strip).show();
  }
}



void showInput() // 천서희
{
  
//  hour_index = getHour Index();
  Serial.println("\n\n--- datetime is "+ datetime);
  Serial.println("--- input hour is "+ String(input_hour));
  Serial.println("--- hour_index is "+ String(hour_index));
//  Serial.println("--- sky is "+ data[hour_index][i_sky]);
//  Serial.println("--- pty is "+ data[hour_index][i_sky]);
}

void showTodayHour() // DONE [이윤지 : 회로 작업이 까다로울 듯!!!!!]
{
  for(int i=0; i<8; i++)
  {
    if((data[0][i_hour].toInt()-6)/3 <= i <= 6)
      digitalWrite(clock_strip[i], HIGH);
    else
      digitalWrite(clock_strip[i], LOW);
  }
  Serial.println("--- now_hour is " + data[0][i_hour] + " ~ 24");
}

void showSky() // 이윤지! . 시간에 따라 색깔 달라지기. (밤 즈음엔 필름으로 어둡게...) -> 나중에 바꾸기
{
    colorOn(&sky_strip);

      
//    if(input_hour == 0)
//      colorWipe(&sky_strip, sky_strip.Color(3, 36, 114), 0);
//    else if (input_hour == 6)
//      colorWipe(&sky_strip, sky_strip.Color(4, 104, 150), 0);
//    else if (input_hour == 12)
//      colorWipe(&sky_strip, sky_strip.Color(0, 246, 255), 0);
}

void showWeather(int sky, int pty)  // 이소희
{   
    Serial.println("sky is : " + String(sky));
    Serial.println("pty is : " + String(pty));
    // sky
    // 맑음 : 맑음
    // 구름 조금 : 맑음, 구름 조금
    // 구름 많음 : 맑음, 구름 조금, 구름 많음
    // 흐림 : 구름 조금, 구름 많음, 흐림

    // pty
    // 비, 눈은 따로!
    
//    int pty2 = -1;
//    switch(pty)
//    {
//      case 1:
//        pty = 5;
//        break;
//      case 2: case 3:
//        pty = 5;
//        pty2 = 6;
//        break;
//      case 4:
//        pty = 6;
//        break;
//    };
//    
//    for(int i = 1; i<WEATHER_LEN; i++)
//      if(i==sky || i==pty || i==pty2) colorOn(&weather_strip[i]);
//      else colorOff(&weather_strip[i]);
    
}
