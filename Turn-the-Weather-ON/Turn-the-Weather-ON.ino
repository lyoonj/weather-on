#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>

#define STRIP_LEN 8
#define WEATHER_LEN 7



// WiFi, Server
char ssid[] = "HYDROM-522"; // 와이파이 id
char pass[] = "vlzja0524"; // 와이파이 pw
char host[] = "www.kma.go.kr";
String url = "/wid/queryDFSRSS.jsp?zone=";
String zone = "4127152500";
WiFiServer server(80);
WiFiClient client;
IPAddress hostIp;

// Weather Data
int i_hour = 0, i_sky = 1, i_pty = 2, i_temp = 3, i_reh = 4;
String tags[6][2] = {{"<hour>","</hour>"}, {"<sky>","</sky>"}, {"<pty>","</pty>"}, {"<temp>", "</temp>"}, {"<reh>", "</reh>"}};
String data[8][6] = {{"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""},
                     {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}, {"", "", "", "", "", ""}};

// Input
int volume = A0;
int volume_input;
int now_hour = -1;
String input_date = "";
int input_hour = -1;
int hour_index;
String output_sky;
String output_pty;

// Output
Adafruit_NeoPixel weather_strip[WEATHER_LEN] = {Adafruit_NeoPixel(D7, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 하늘 배경
                                                Adafruit_NeoPixel(D6, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 맑음
                                                Adafruit_NeoPixel(D5, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 구름조금
                                                Adafruit_NeoPixel(D4, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 구름많이
                                                Adafruit_NeoPixel(D3, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 흐림
                                                Adafruit_NeoPixel(D2, STRIP_LEN, NEO_GRB + NEO_KHZ800),  // 비 - 1
                                                Adafruit_NeoPixel(D1, STRIP_LEN, NEO_GRB + NEO_KHZ800)}; // 눈 - 4
Adafruit_NeoPixel sky_strip = weather_strip[0];                                               












void setup() 
{
    Serial.begin(115200);
    
    // Input setting
    pinMode(volume, INPUT);
    // Output setting
    sky_strip.begin();
    sky_strip.show();
    for(int i=0; i<WEATHER_LEN; i++){
      weather_strip[i].begin();
      weather_strip[i].show();
    }

    // WiFi & Server setting
    connectToWiFi();
    connectToServer();
}


void loop()  // 문제점 : server에서 data를 게속 받아오면 안된다. 딜레이를 넣어야 한다. 근데 volume input에는 즉각 반응해야한다. delay를 하면서 volume 인풋에만 귀 기울이는 방법?
{
  // Server -> Data
  parseWeatherData();
  checkWeatherData();
  
  // Input -> Data
  volume_input = analogRead(volume);
  input_hour = getHourInput();
  hour_index = getHourIndex(); // 이 코드도 맡길 수 있음 맡기고.. -> 이거는 내가 해야할 듯... 내일 이거 구현하고 / 도면 그리고 / 보고서 작성 및 추후 계획
  output_sky = data[hour_index][i_sky];
  output_pty = data[hour_index][i_pty];
  // 온도 습도는 나중에 여유 되면...

  // Data -> Output
  showDate();  // 아마 LCD로 출력 -> 이 코드 누구한테 맡겨야지
  showNowHour(); // now_hour -> 한시간마다 갱신..
  showInputHour(); // input_hour - 7 segment에 hour 출력 -> 이 코드 누구한테 맡겨야지
  showSky(); // input_hour -
  showWeather(output_sky.toInt(), output_pty.toInt());

  
  delay(10000); // 10분 단위로 갱신
}














// Wifi & Server
void connectToWiFi()
{
    Serial.println("Connecting to WiFi...");
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
    WiFi.hostByName(host, hostIp);
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
  if(client.available())
  {
    String line = client.readStringUntil('\n');  
    int head, tail;
    
    for(int i=0; i<8; i++)
      for(int j=0; j<4; j++)
      {
          head = line.indexOf(tags[j][0]) + tags[j][0].length();
          tail = line.indexOf(tags[j][1]);
          if(tail>0){
              data[j][i] = line.substring(head, tail);
              Serial.println("\n---- " + line.substring(head, tail) + " is in data [" + String(j) + "][" + String(i) + "]\n"); // 컴파일용. 이후 지우기
          }    
      }
  }
} 

void checkWeatherData() // 컴파일용. 이후 지우기.
{
    for(int i=0; i<16; i++)
    {
      Serial.println("");
      Serial.println("hour is " + data[i][i_hour]);
      Serial.println("temp is " + data[i][i_temp]);
      Serial.println("sky is " + data[i][i_sky]);
      Serial.println("pty is " + data[i][i_pty]);
      Serial.println("reh is " + data[i][i_reh]);
      Serial.println("");
    }
}




// Input -- 이 부분만 다듬자..,.
int getHourInput() // 이건 직접 돌려보면서 다듬어야함. 등분은 되어있으니까 배치에 따라서 몇시부터 시작할지 봐야해...
{
      int input_hour = 6;
      if (volume_input>=925){    
        input_hour = 5;
      }
      else if (volume_input>=883) {    
        input_hour = 4;
      }
      else if (volume_input>=841){
        input_hour = 3;
      }
      else if (volume_input>=799){
        input_hour = 2;
      }
      else if (volume_input>=757){
        input_hour = 1;
      }
      else if (volume_input>=715){
        input_hour = 24;
      }
      else if (volume_input>=673){
        input_hour = 23;
      }
      else if (volume_input>=631){
        input_hour = 22;
      }
     else if (volume_input>=589){
       input_hour = 21;
      }
      else if (volume_input>=547){
        input_hour = 20;
      }
      else if (volume_input>=505){
        input_hour = 19;
      }
      else if (volume_input>=463){
        input_hour = 18;
      }
      else if (volume_input>=421){
        input_hour = 17;
      }
      else if (volume_input>=379){
        input_hour = 16;
      }
      else if (volume_input>=337){
        input_hour = 15;
      }
      else if (volume_input>=295){
        input_hour = 14;
      }
      else if (volume_input>=258){
        input_hour = 13;
      }
      else if (volume_input>=253){
        input_hour = 12;
      }
      else if (volume_input>=211){
        input_hour = 11;
      }
      else if (volume_input>=169){
        input_hour = 10;
      }
      else if (volume_input>=127){
        input_hour = 9;
      }
      else if (volume_input>=85){
        input_hour = 8;
      }
      else if (volume_input>=43){
        input_hour = 7;
      }
      else if (volume_input>=0){
        input_hour = 6;
      }
      return input_hour;
}

int getHourIndex() // 제일 까다롭다....... 이것만 완성하면 이제 Sky 부분만 대충 손볼 수 있고 그럼 난 잘 거야... -> 다음날 완성! 그래 그때 정신머리로 이거 못했어.. 아주 짧은.. 코드지만.....
{
      int i = 0;
      int hour_area = input_hour - (input_hour % 3); // 입력 시간을 9, 12, 15 등의 3배수 시간대로 환원
      while(data[i][i_hour].toInt() != hour_area)    // 이후.. 시뮬레이션 방식이 아니라 연산식으로 코드 수정할 것....,
        i++;
      return i;
}




// Output
void colorWipe(Adafruit_NeoPixel strip, uint32_t c, uint8_t wait) {
  for (uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void colorOn(Adafruit_NeoPixel strip) {
  for (uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
    strip.show();
  }
}

void showDate()
{
    // 7 segment나 LCD 모니터로 날짜 표현... 아마 LCD 모니터...  -> 부탁해요
}

void showNowHour()
{
    // 현재 시간부터 24시까지 LED로 켜기 -> 근데 이걸 오늘 날짜라고 생각을 할까 사람들이..? 안돼 그런 거 지금 생각하면 망해
}

void showInputHour()
{
    // 7 segment로 input_hour 표현! -> 부탁해요(2자리 수!)
}

void showSky() // 시간에 따라 색깔 달라지기. (밤 즈음엔 필름으로 어둡게...)
{
    if(input_hour == 0)
      colorWipe(sky_strip, sky_strip.Color(3, 36, 114), 0);
    else if (input_hour == 6)
      colorWipe(sky_strip, sky_strip.Color(4, 104, 150), 0);
    else if (input_hour == 12)
      colorWipe(sky_strip, sky_strip.Color(0, 246, 255), 0);
}

void showWeather(int sky, int pty) // weather도 숫자로 매핑하면 코드 확 줄어드는데 -> 아니 근데 구현하기엔 좀 까다로운 거 같아.... -> 해냈음.
{
    colorOn(sky);
    
    int rainy=5, snowy=6;
    if (pty == 0)
      return ;
    else if (pty == 1) //비
      colorOn(rainy);
    else if (pty == 4) //눈
      colorOn(snowy);
    else{
      colorOn(rainy);
      colorOn(snowy); // 나중에 밝기 차이 두기? 회색이랑 흰색으로.... 그때는 switch문으로 ㄱㄱ.
    }
}
