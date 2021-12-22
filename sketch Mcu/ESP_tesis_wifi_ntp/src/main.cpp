#include <Arduino.h>
#include <Wire.h> 
#include <wifi.h>
#include <time.h>
#include<PubSubClient.h>
#include <LiquidCrystal_I2C.h>

//declarar constantes de setup aqui
LiquidCrystal_I2C lcd(0x3f, 16, 2);
const char* ssid       = "MQTT_Avetol";
const char* password   = "7Cy65ZysvYk5";
const char* mqtt_server = "192.168.1.10";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;
const int   daylightOffset_sec = 3600;


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//declarar constantes aqui
int vdc_sensor = 0;
int idc_sensor = 0;
const int Luz = 32;
const int Wind = 33;
char all[4];


//funciones
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println(&timeinfo, "%d %Y %H:%M:%S");
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "ton"){
      Serial.println("ton");
      digitalWrite(Luz, LOW);
    }
    else if(messageTemp == "toff"){
      Serial.println("toff");
      digitalWrite(Luz, HIGH);
    }
    else if (messageTemp == "won"){
      Serial.println("won");
      digitalWrite(Wind, LOW);
    }
    else if (messageTemp == "woff"){
      Serial.println("woff");
      digitalWrite(Wind, HIGH);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
    Wire.begin();
    lcd.begin(16, 2);
    lcd.backlight();
    Serial.begin(115200);
    pinMode(Luz, OUTPUT);
    pinMode(Wind, OUTPUT);
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
  printLocalTime();
  vdc_sensor = analogRead(36);
  //Serial.println(vdc_sensor);
  float volt = ((vdc_sensor*17.62) / 4095);
  Serial.print(volt);
  Serial.println("Vdc");
  constexpr size_t buffer_sizev = 1;
  char volts[buffer_sizev];
  dtostrf(volt, buffer_sizev, 2, volts);
  String volta = volts;
  client.publish("windligthmodule/volt", volts);
  //Serial.println(volts);
  idc_sensor = analogRead(39);
  //Serial.println(idc_sensor);
  float curr = ((idc_sensor*8.5) / 4095);
  char currs [buffer_sizev];
  dtostrf(curr, buffer_sizev, 2, currs);
  client.publish("windligthmodule/ampere", currs);
  String curra = currs;
  String out = (String(volta)+","+String(curra));
  //Serial.println(out);  
  char outs [12];
  out.toCharArray(outs,12);
  client.publish("windligthmodule/all", outs);
  //Serial.println(outs);
  Serial.print(curr);
  Serial.println("Adc");
  Serial.print(volt*curr);
  Serial.println("Wdc");
  lcd.setCursor(0,1);
  lcd.println(WiFi.localIP());
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
  }
}