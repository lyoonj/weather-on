#include <ESP8266WiFi.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

char ssid[] = "HYDROM-522"; // 와이파이 id
char pass[] = "vlzja0524"; // 와이파이 pw
char host[] = "www.kma.go.kr";
String url = "/wid/queryDFSRSS.jsp?zone=";
String zone = "4127152500";

WiFiServer server(80);
WiFiClient client;
IPAddress hostIp;

String hour[16] = {"", };
String weather[16] = {"", };
String temp[16] = {"", };
String reh[16] = {"", };
String info_index[4][2] = {{"<hour>","</hour>"}, {"<wfKor>","</wfKor>"}, {"<temp>", "</temp>"}, {"<reh>", "</reh>"}};
String* info_contents[4] = {hour, weather, temp, reh};

int volume = A0; // Input Sensor 연결 포트
int now_hour;

void connectToWiFi();
void connectToServer();
void parseWeatherData();
void showWeatherData();

void setup() 
{
    Serial.begin(115200);

    connectToWiFi();
    connectToServer();
}

void loop() 
{
  parseWeatherData();
  printWeatherData();

  delay(1000);
}


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
    if(client.connect(hostIp, 80))
    {
      Serial.println("Server Connected!");
      client.print(String("GET ") + url + zone + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Connection: close\r\n\r\n");  
    }  
}

void parseWeatherData()
{
  if(client.available())
  {
    int head, tail;
    String line = client.readStringUntil('\n');  

    for(int i=0; i<8; i++)
      for(int j=0; j<4; j++)
      {
        head = line.indexOf(info_index[j][0]) + info_index[j][0].length();
        tail = line.indexOf(info_index[j][1]);
        if(tail>0)
          info_contents[j][i] = line.substring(head, tail);
      }
  }
} 


void printWeatherData()
{
    for(int i=0; i<8; i++)
  {
      Serial.println("hour is " + hour[i]);
      Serial.println("temp is " + temp[i]);
      Serial.println("weather is " + weather[i]);
      Serial.println("reh is " + reh[i]);
      Serial.println("");
  }
}

void showWeatherData()
{
    
}
