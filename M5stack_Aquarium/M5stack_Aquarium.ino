#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h> // MQTT
#include <OneWire.h>      // DS18B20  ※カスタマイズ版を使用する
#include <DallasTemperature.h>  //　温度の単位換算
#include "HCSR04.h"       // 距離センサ　

// 距離センサ　定義
#define TRIG    5
#define ECHO    17
HCSR04 hcsr04;

// リレー　定義
#define RELAY_PUMP   3
#define RELAY_ON    HIGH
#define RELAY_OFF   LOW

// Wi-Fi定義
char *ssid = "************";            // Wi-FiのSSID
char *password = "********";              // Wi-Fiのパスワード

// MQTT 定義
const char *endpoint = "broker.shiftr.io";    // MQTTブローカ（shiftrの場合）
const int port = 1883;                    // MQTT 接続ポート
char *deviceID = "M5Stack";               // デバイスID
char *userName = "********";              // MQTTブローカ認証ユーザ名
char *userPass = "****************";      // MQTTブローカ認証パスワード
char *pubTopic = "/pub/";          // メッセージを知らせるトピック
char *subTopic = "/sub/pump";          // メッセージを待つトピック
WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);

// DS18B20の定義
const int oneWireBus = 16;     
OneWire oneWire(oneWireBus);
DallasTemperature DS18B20(&oneWire);

int st_pump = 0;          // モータ状態
int th_distance = 5;      // 水位の基準値
int Aquarium_distance = 20; // 水槽の高さ  
int WaterLevel = 0;         // 水位
float celsius;              // 水温

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) ;
    
    // 距離センサ　初期化
    hcsr04.begin(TRIG, ECHO);

    // リレー端子設定（ポンプ）
    pinMode(RELAY_PUMP, OUTPUT);
    digitalWrite(RELAY_PUMP, RELAY_OFF);
    
    // Wi-Fi設定
    WiFi_init();

    // connect MQTT
    mqttClient.setServer(endpoint, port);
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
    
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(3);
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}


void get_temp(void){
  DS18B20.begin();
  int count = DS18B20.getDS18Count();

  if (count > 0) {
    DS18B20.requestTemperatures();

    for (int i = 0; i < count; i++) {
      celsius = DS18B20.getTempCByIndex(i);
//      fahrenheit = celsius * 1.8 + 32.0;
//      fahrenheit = round(fahrenheit);
    }
  }
  Serial.printf("dev[%d] get = %2.2f\n",count ,celsius);
}

long messageSentAt = 0;
int count = 0;
char pubMessage[128];

void loop() {
  // 水温を取得
  get_temp();

  // 距離を取得
  int HCSR04_d = hcsr04.distance();
  Serial.printf("disrance = %d\n",HCSR04_d);

  if(HCSR04_d > Aquarium_distance){
    WaterLevel = 0;
  }else{
    WaterLevel = Aquarium_distance - HCSR04_d;  // 水位
  }
  
  // MQTT 送信  Ambient用データにフォーマット
  sprintf(pubMessage, "{\"d1\": %2.01f,\"d2\": %d}", celsius,WaterLevel);

  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
//  M5.Lcd.print("Devices found: ");
//  M5.Lcd.println(count);
  
  M5.Lcd.print("temp : ");
  M5.Lcd.print(celsius, 0);
  M5.Lcd.print( "C");

  M5.Lcd.setCursor(0, 25);

  M5.Lcd.print("Water : ");
  M5.Lcd.print(WaterLevel, 0);
  M5.Lcd.print( "cm");
  
  Serial.print("Publishing message to topic ");
  Serial.println(pubTopic);
  Serial.println(pubMessage);
  mqttClient.publish(pubTopic, pubMessage);
  Serial.println("Published.");

  mqttLoop();
  
//  delay(10000);
  delay(1000);
}

void WiFi_init(void){
  // WiFi setting
  Serial.println("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  // WiFi Connected
  Serial.println("\nWiFi Connected.");
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("WiFi Connected.");
}

void connectMQTT() {
  mqttClient.setServer(endpoint, port);
  mqttClient.setCallback(mqttCallback);
  while (!mqttClient.connected()) {
      if (mqttClient.connect(deviceID,userName,userPass)) {
          Serial.println("MQTT Connected.");
          int qos = 0;
          mqttClient.subscribe(subTopic, qos);
          Serial.println("Subscribed.");
      } else {
          Serial.print("Failed. Error state=");
          Serial.print(mqttClient.state());
          // Wait 5 seconds before retrying
          delay(5000);
      }
  }
}

void mqttCallback (char* topic, byte* payload, unsigned int length) {
    String str = "";
    static int pump[2] = {0,0};
    Serial.print("Received. topic=");
    Serial.println(topic);
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        str += (char)payload[i];
    }
    Serial.print("\n");
   
    // 受け取ったJSON形式のメッセージをパース
    StaticJsonBuffer<200> jsonBuffer;    
    JsonObject& root = jsonBuffer.parseObject(str);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }
    // JSONデータを割りあて
    const char* message = root["message"];
    pump[0] = root["pump"];
    Serial.println("recive message");
    Serial.println(st_pump);

    if(pump[0] != pump[1]){
      st_pump = pump[0];
    }
    pump[1] = pump[0];

    if(st_pump == 1){
      digitalWrite(RELAY_PUMP, RELAY_ON);
      Serial.println("ON");
    }else{
      digitalWrite(RELAY_PUMP, RELAY_OFF);
      Serial.println("OFF");
    }
}
