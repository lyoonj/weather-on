#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <RTClib.h>


// define input
#define clPin D5        // !!! --- pinNUM
#define dtPin D4        // !!! --- pinNUM
#define swPin D3        // !!! --- pinNUM

// define output
#define DATA_LEN 6
#define WEATHER_PIN D6  // !!! --- pinNUM
#define WEATHER_LEN 12
#define WEATHER_NUM 6
#define SKY_PIN D7      // !!! --- pinNUM
#define SKY_LEN 33
#define CLOCK_PIN D8 
#define CLOCK_LEN 8    // !!! --- pinNUM



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
int last_hour = -1;
int now_hour = 0;
int now_minute = 0; // for compile
int now_month = 0;
int now_day = 0;

// Input Data
static int oldA = HIGH;
static int oldB = HIGH;
int now_input = 0;
int last_input = -1;
int hour_index = 0;
int input_day = 0;

// Weather Data
int tail, head;
String datetime = "";
int i_hour = 0, i_day = 1, i_temp = 2, i_sky = 3, i_pty = 4, i_reh = 5;
String tags[DATA_LEN][2] = {{"<hour>","</hour>"}, {"<day>","</day>"}, {"<temp>", "</temp>"}, {"<sky>","</sky>"}, {"<pty>","</pty>"}, {"<reh>", "</reh>"}};
String data[8][DATA_LEN] = {{"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""},
                            {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}};
// Output
int output_sky = 0;
int output_pty = 0;
Adafruit_NeoPixel weather_strip = Adafruit_NeoPixel(WEATHER_LEN * WEATHER_NUM, WEATHER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel sky_strip = Adafruit_NeoPixel(SKY_LEN, SKY_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel clock_strip = Adafruit_NeoPixel(CLOCK_LEN, CLOCK_PIN, NEO_GRB + NEO_KHZ800);



void showSky();
void showWeather();












void setup() {
  Serial.begin(115200);

  // set time
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));

  // set input
  pinMode(clPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP); // del it after all done
  digitalWrite(swPin, HIGH);
  
  // set output

  // connect WiFi, Server
  connectToWiFi();
  connectToServer();
  getWeatherData();
}

void loop() {
  // when hour changes, get weather
//  getTime();
//  if(now_hour != last_hour)
//  {
//    showTodayHour();
//    getWeatherData();
//  }
//  last_hour = now_hour;

  // when input changes, show weather
  
  getInput();
  if(now_input != last_input)
  {
    getHourIndex();
    showInput();
    showSky();
    showWeather();
  }
  last_input = now_input;
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
void getTime()
{
    DateTime now = RTC.now(); 
    now_hour = now.hour();
    now_minute = now.minute(); // for compile
    now_month = now.month();
    now_day = now.day();
}

void getWeatherData()
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
void getInput()
{
  int change = getEncoderTurn();
  now_input = now_input + change;
  if(digitalRead(swPin) == LOW)
    now_input = 0; 
  if(now_input>0)
    now_input = now_input % 20;
  else
    now_input = now_input % 20 + 20; 
  if(now_input == 0)
    now_input = 20;
}

int getEncoderTurn()
{
  int result = 0;
  int newA = digitalRead(clPin);
  int newB = digitalRead(dtPin);
  if(newA != oldA || newB != oldB)
    if(oldA == HIGH && newA == LOW)
      result = (oldB * 2 - 1);
  oldA = newA;
  oldB = newB;
  return result;  
}

void getHourIndex() // ~ input hour
{
      int i = 0;
      int hour_area = now_input - (now_input % 3) + 3;
      for(int i=0; i<8; i++)
        if(data[i][i_hour].toInt() == hour_area) 
          hour_index = i;
}




// Output
void showInput()
{
    Serial.println("your input " + String(now_input));  
    Serial.println("now you see the weather on " + String(now_month) + ("/") + String(now_day + data[hour_index][i_day]) + ", ~" + data[hour_index][i_hour] + " : 00");  
}

void showTodayHour()
{
    Serial.println("Today hour is " + String(now_hour) + " :00 ~ 24:00");
    for(uint16_t i = 0; i<=CLOCK_LEN; i++)
      if(now_hour<=i*3){
          clock_strip.setPixelColor(i, clock_strip.Color(255, 255, 255));
      }
      else{
          clock_strip.setPixelColor(i, 0);
      }
}

void weatherOn(int n)
{
    for (uint16_t i = (n-1)*WEATHER_LEN; i < n*WEATHER_LEN; i++){
        weather_strip.setPixelColor(i, weather_strip.Color(255, 255, 255));  
        weather_strip.show();
    }
}
void weatherOff(int n)
{
    for (uint16_t i = (n-1)*WEATHER_LEN; i < n*WEATHER_LEN; i++){
        weather_strip.setPixelColor(i, 0);  
        weather_strip.show();
    }
}

void showWeather()
{
    Serial.println("\n--- The weather of ~" + String(hour_index*3) + " : 00");
    Serial.println(data[hour_index][i_sky]);
    Serial.println(data[hour_index][i_pty]);

    // sky
    // [WEATHER_LEN*0] <= ~ < [WEATHER_LEN*1] / 맑음 : 맑음
    // [WEATHER_LEN*1] <= ~ < [WEATHER_LEN*2] / 구름 조금 : 맑음, 구름 조금
    // [WEATHER_LEN*2] <= ~ < [WEATHER_LEN*3] / 구름 많음 : 맑음, 구름 조금, 구름 많음
    // [WEATHER_LEN*5] <= ~ < [WEATHER_LEN*4] / 흐림 : 구름 조금, 구름 많음, 흐림
    if(output_sky == 4){
        for(int i=1; i<=4; i++){
            if(i==output_sky)
              weatherOn(i);
            else
              weatherOff(i);
        }
    }
    else{
        for(int i=1; i<=4; i++){
            if(i<=output_sky)
              weatherOn(i);
            else
              weatherOff(i);
        }
    }

    // pty
    // 비, 눈은 따로! (비 : 1, 눈 : 4)
    int rain = 5, snow = 6;
    weatherOff(rain);
    weatherOff(snow); // 깜빡거리면 그냥 조건 일일히 따지는 방향으로~~
    if(output_pty == 1 || output_pty == 4){
      weatherOn(6 - output_pty % 4); // 1 -> 5, 4 -> 6
    }
    else if(output_pty == 2 || output_pty == 3) {
      weatherOn(rain);
      weatherOn(snow);
    }
    // -- 이 코드 맘에 안 들 어 안 들  ㅇ ㅓ   ,  . ...,  , . 근데 이건 전적으로 기상청이 잘못했어... 어
    // --- 4로 나누면?
    // 1%4 = 1, 4%4 = 0
    // 6 - out_pty % 4
    // 3%4 = 1, 
}

void showSky()
{
  for(uint16_t i=0; i<SKY_LEN; i++)
  {
    sky_strip.setPixelColor(i, sky_strip.Color(255, 255, 255));
    sky_strip.show();  
  }
}
