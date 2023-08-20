#include <ESP8266WiFi.h>       //wifi库
#include <ESP8266HTTPClient.h> //http请求库
#include <ArduinoJson.h>       //json库
#include <iostream>
using namespace std;


//fanmod 开源，qq1751512538
//github：https://github.com/fanmod1/arduino-esp8266-ddns

//声明清单
String ssid = "wifi名字";            //WiFi_ssid
String password = "wifi密码";  //WiFi_password

String apihead = "api.cloudflare.com";       // api host
String quyuid = "区域id";   // 区域id  域名概括页右下角可查看
String yuming = "fanmod.com";                         // 区域域名
String dnsname = "www";                                // 要ddns解析的name
String cfkey = "1_xxxxxxxxxXUIC16T3xxxxxxxxxkigxxxxxxxxx";  // api令牌

int iftime = 600000;   // 判断间隔时间
String itip = "http://ifconfig.me/ip";   // 取 公网IP 的api 


//创建对象
WiFiClient wificlient;
HTTPClient httpClient;
WiFiClientSecure https;

int looplock = 0;
void setup(void){

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);  // led灯 引脚初始化

  delay(1000);

  Serial.print("\n连接wifi：");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA); //连接模式设置为STA
  WiFi.begin(ssid, password); //连接wifi

  // WiFi.status() 是WiFi连接状态 连接成功后返回 WL_CONNECTED
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, 0); //亮
    Serial.print(".");
    delay(300);
    digitalWrite(LED_BUILTIN, 1); //灭
    delay(300);
  }
  //连接成功
  
  Serial.println("\n连接成功");

  
  digitalWrite(LED_BUILTIN, 0); //亮
  delay(3000);
  digitalWrite(LED_BUILTIN, 1); //灭
  
  Serial.print("[");
  Serial.print(WiFi.localIP().toString()); //打印dhcp分配的IP地址
  Serial.println("]");
  Serial.println("FanMod开源，https://github.com/fanmod1/arduino-esp8266-ddns");

  looplock = 1;
}

 
void loop(void){
if(looplock == 1){

  
  https.setInsecure();
  
  String url = "https://" + apihead + "/client/v4/zones/" + quyuid + "/dns_records";
  
  //通过begin函数配置请求地址。
  httpClient.begin(https,url);
  httpClient.addHeader("Content-Type","application/json");
  httpClient.addHeader("Authorization","Bearer " + cfkey);
  
  int httpResponseCode = httpClient.GET();

  String response;
  if (httpResponseCode == 200) {
    // dns记录列表请求成功
    response = httpClient.getString();
    Serial.println("dns记录列表获取成功:");
    //Serial.println("返回体: ");
    //Serial.println(response);
  } else {
    // dns记录列表请求失败
    response = httpClient.getString();
    Serial.print("错误代码: ");
    Serial.println(httpResponseCode);
    Serial.print("错误返回体: ");
    Serial.println(response);
    
  }
  httpClient.end();

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("未能解析JSON: ");
      Serial.println(error.c_str());
      return;
    }
    // 获取result数组
    JsonArray resultArray = doc["result"];
    
    String id = "0";
    String content;
    for (JsonObject obj : resultArray) {

      String name = obj["name"].as<String>();
      if(name == dnsname+"."+yuming){
        id = obj["id"].as<String>();
        content = obj["content"].as<String>();
        Serial.println("找到目标dns记录_id为:"+id);
      }
      
    }
    if(id == "0"){
      Serial.println("域名或域名的name值找不到");
    }else{
    httpClient.begin(wificlient,itip);
    httpResponseCode = httpClient.GET();
    if(httpResponseCode == 200){
      String response2 = httpClient.getString();
      
      if(content != response2){
        Serial.println("ip不同："+response2+" / "+content);
        Serial.println("执行更新");

        String url2 = url + "/" + id;
        // 构建 put json 数据
        StaticJsonDocument<200> payload;
        payload["content"] = response2;
        payload["name"] = dnsname;
        payload["type"] = "A";
        payload["proxied"] = true;

        // 序列化 put json
        String jsonPayload;
        serializeJson(payload, jsonPayload);

        // 建立连接
        https.setInsecure();
        if (!https.connect(apihead, 443)) {
          Serial.println("建立cloudflare连接失败");
          return;
        }

        // 构建put请求
        String request = "PUT ";
        request += url2;
        request += " HTTP/1.1\r\n";
        request += "Host: "+apihead+"\r\n";
        request += "Authorization: Bearer ";
        request += cfkey;
        request += "\r\n";
        request += "Content-Type: application/json\r\n";
        request += "Content-Length: ";
        request += jsonPayload.length();
        request += "\r\n\r\n";
        request += jsonPayload;
        // 发送put请求
        https.print(request);
        // 等待响应
        while (https.connected()) {
          String line = https.readStringUntil('\n');
          if (line == "\r") {
            break;
          }
        }
        // 读取 响应
        int response3 = 0;
        String datare;
        while (https.available()) {
          datare = https.readStringUntil('\n');
          if(datare.indexOf("{\"result\":{\"id\":") >= 0){
            response3 = 1;
          }
        }
        if(response3 == 1){
          Serial.println("更新成功，新IP为："+response2);
        }else{
          Serial.println("更新失败："+datare);
        }
        https.stop();
        
      }else{
        Serial.println("ip相同："+response2);
      }
    }else{
      Serial.println("获取公网IP的api出问题了");
      
    }
    }
  }

  
  delay(iftime);
}
}


