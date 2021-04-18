/*------------------------------------------------------------------------------
  Copyright  @ DVXLAB , All rights reserved 
  Distributed under MIT License
------------------------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFiUdp.h>

#define AP_MODE

#define LED  D4
#define MOTOR1_A D1
#define MOTOR1_B D2
#define MOTOR2_A D5
#define MOTOR2_B D6
#define HEADLIGHT D7
#define TAILLIGHT D8 

#ifdef AP_MODE
IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 101);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); 
IPAddress secondaryDNS(8, 8, 4, 4); 
char* ssid = "eCar32AP";
char* password = "12345678";
#else 
IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); 
IPAddress secondaryDNS(8, 8, 4, 4);
char* ssid = "HawksEye";
char* password = "aspen911";
#endif
const int Port = 80;
WiFiUDP Udp;


void setup()
{  
  Serial.begin(9600);
  //Wifi Init
#ifdef AP_MODE
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)){
    Serial.println("Soft AP config Failed");
  }
  if (!WiFi.softAP(ssid, password)){
    Serial.println("Soft AP Failed");
  }
   Serial.println(WiFi.softAPIP());
#else 
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)){
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(500);

  } 
 #endif 
  // IO Init 
  pinMode(MOTOR1_A, OUTPUT);
  pinMode(MOTOR2_A, OUTPUT);
  pinMode(MOTOR1_B, OUTPUT);
  pinMode(MOTOR2_B, OUTPUT);
  analogWrite(MOTOR1_A,0);
  analogWrite(MOTOR2_A,0);
  digitalWrite(MOTOR1_B, LOW);
  digitalWrite(MOTOR2_B, LOW);
  pinMode(LED, OUTPUT);
  pinMode(HEADLIGHT,OUTPUT);
  pinMode(TAILLIGHT,OUTPUT);

  Serial.println("");
  Serial.print("IP Address: ");
  //Serial.println(WiFi.localIP());
  Udp.begin(Port);
}

void loop()
{
  enum{REVERSE,FORWARD};
  static bool LED_Status=true;
  int VBatADC = 0; 
  static unsigned long l = 0;
  static unsigned long task1_Led = 0;
  unsigned long t = millis();
  //1.LED blinking Task
  if((t - task1_Led ) > 500){
    task1_Led = t;
    if ( LED_Status== false)
       digitalWrite(LED, HIGH);   
    else                    
       digitalWrite(LED, LOW);    
    LED_Status=!LED_Status;    
  }
  
  //2.Packet Processing
  int packetSize = Udp.parsePacket();
  if (packetSize) {

    String line = Udp.readStringUntil('\r');
    Serial.print(line);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, line);
    String msg = doc["message"];

    //3. CAR Run
    if(msg=="joystick"){
      //Step1 -> idle position
      int Motor1Val=0;
      int Motor2Val=0;
      int Direction = FORWARD;
      int jsonX= doc["X1"]; 
      int jsonY= doc["Y1"];
      int X=map(jsonX, 0, 4096, 0, 11);//remap
      int Y=map(jsonY, 0, 4096, 0, 11);
      if ((X>3) && (X<6) )
        Motor1Val= 0;
      if ((Y>3) && (Y<6) )  
        Motor2Val= 0;

      //Step2 ->  Reverse
      if (Y>5){ 
        Motor1Val= (Y-5)*200;
        Motor2Val= (Y-5)*200;
        Direction = REVERSE;
      }
      //Step3 -> Forward
      if ((Y<4)) {
        Motor1Val= (5-Y)*200;
        Motor2Val= (5-Y)*200;
        Direction = FORWARD;
      }
      //Step4 -> Move Left
      if ((X<4)) {
        Motor2Val=0;
        Motor1Val=700;     
      }
      //Step4 -> Move right 
      if ((X>5)) {
        Motor1Val=0;
        Motor2Val=700;       
      }            
      if (Direction==FORWARD){
        analogWrite(MOTOR1_A,Motor1Val);
        analogWrite(MOTOR2_A,Motor2Val);
        digitalWrite(MOTOR1_B,LOW);
        digitalWrite(MOTOR2_B,LOW); 
      }
      else{
        analogWrite(MOTOR1_B,Motor1Val);
        analogWrite(MOTOR2_B,Motor2Val);
        digitalWrite(MOTOR1_A,LOW);
        digitalWrite(MOTOR2_A,LOW);
      }
    }
    // 4. lights 
    if(msg=="lights"){
      String jsonHeadLight= doc["Head"];
      String jsonTailLight= doc["Tail"];
      if (jsonHeadLight=="ON")
        digitalWrite(HEADLIGHT,LOW);
      else
        digitalWrite(HEADLIGHT,HIGH); 
      if (jsonTailLight=="ON")
        digitalWrite(HEADLIGHT,LOW);
      else
        digitalWrite(HEADLIGHT,HIGH);

    }

  }

}




