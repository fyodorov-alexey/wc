// Контроллер туалета
const char* firmware_version = "2020-07-04/v1.6.2"; // Код подчищен от явного мусора

void reportUptimeTask( void * parameter );
void reportWaterStateTask( void * parameter );
void reportGasStateTask( void * parameter );
void reportLightStateTask( void * parameter );
void mqttClientCycleTask( void * parameter );
void otaUpdaterCycleTask( void * parameter );
void wifiControlCycleTask( void * parameter );
void rssiCycleTask( void * parameter );

#include <Arduino.h>
#include "funcs.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Update.h>

const char *ssid                  = "SSID";
const char *password              = "PASS";
int status                        = WL_IDLE_STATUS;

WiFiClient wificlient;
WebServer server(80);

const char *mqtt_server           = "MQTT SERVER IP";
const int   mqtt_port             = 1883;
const char *mqtt_user             = "MQTT_USER";
const char *mqtt_pass             = "MQTT_PASS";
const char *mqtt_device           = "wc-esp";

const char *topic_firmware        = "/home/wc/firmware_version";
const char *topic_ip              = "/home/wc/ip_address";
const char *topic_uptime          = "/home/wc/uptime";
const char *topic_light           = "/home/wc/light";
const char *topic_light_info      = "/home/wc/light_info";
const char *topic_ext_light_info  = "/home/wc/ext_light_info";
const char *topic_fan             = "/home/wc/fan";
const char *topic_fan_info        = "/home/wc/fan_info";
const char *topic_msg             = "/home/wc/msg";
const char *topic_wsensors        = "/home/wc/watersensors";
const char *topic_gas             = "/home/wc/gas";
const char *topic_time1           = "/home/wc/time1";
const char *topic_time2           = "/home/wc/time2";
const char *topic_time1_info      = "/home/wc/time1_info";
const char *topic_time2_info      = "/home/wc/time2_info";
const char *topic_rssi            = "/home/wc/rssi";

bool retained                     = true;

PubSubClient mqttclient(mqtt_server, mqtt_port, wificlient);

int8_t _hour, _min, _sec, _day, _month;
int16_t _year, fan_timer;

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{  
  String message;
  for (int i = 0; i < length; i++) { message +=(char)payload[i]; }
  if( strcmp(topic, topic_light) == 0 )
  {
    if (message == "0")
    {
      digitalWrite(PIN_RELAY_LIGHT, LOW);
      mqttclient.publish(topic_msg, "WC:OK,LIGHT_OFF", retained);
      mqttclient.publish(topic_light_info, "0", retained);
    } 
    if (message == "1")
    {
      digitalWrite(PIN_RELAY_LIGHT, HIGH);
      mqttclient.publish(topic_msg, "WC:OK,LIGHT_ON", retained);
      mqttclient.publish(topic_light_info, "1", retained);
    }
  }
  if( strcmp(topic, topic_fan) == 0 )
  {
    if (message == "0")
    {
      digitalWrite(PIN_RELAY_FAN, LOW);
      mqttclient.publish(topic_msg, "WC:OK,FAN_OFF", retained);
      mqttclient.publish(topic_fan_info, "0", retained);
    }
    if (message == "1")
    {
      digitalWrite(PIN_RELAY_FAN, HIGH);
      mqttclient.publish(topic_msg, "WC:OK,FAN_ON", retained);
      mqttclient.publish(topic_fan_info, "1", retained);
    }
  }
  if( strcmp(topic, topic_msg) == 0 )
  {
    if (message == "reset")
    {
      PRINTS("REBOOT NOW\n");
      mqttclient.publish(topic_msg, "WC:REBOOT", retained);
      delay(200); 
      ESP.restart();
    }
  }
}

void mqtt_connect(const char* msg)
{
/* коды состояния mqttclient.state()
(252) -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
(253) -3 : MQTT_CONNECTION_LOST - the network connection was broken
(254) -2 : MQTT_CONNECT_FAILED - the network connection failed
(255) -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
(0) 0 : MQTT_CONNECTED - the client is connected
(1) 1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
(2) 2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
(3) 3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
(4) 4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
(5) 5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
*/
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqttclient.connected())
    {
      String fullmsg = String(msg) + "_prev_state:" + mqttclient.state();
      PRINTS("Connecting to MQTT server\n");
      mqttclient.setServer(mqtt_server,mqtt_port);   
      if (mqttclient.connect(mqtt_device, mqtt_user, mqtt_pass)) 
      {
         mqttclient.setCallback(mqtt_callback);
         PRINT("Connected to MQTT server, ", msg);
         mqttclient.publish(topic_msg,fullmsg.c_str()); // prev. state
         String fullmsg = String(msg) + "_new_state:" + mqttclient.state();
         mqttclient.publish(topic_msg,fullmsg.c_str()); // new state
         mqttclient.subscribe(topic_light);
         mqttclient.subscribe(topic_fan);
         mqttclient.subscribe(topic_msg);
      } 
      else
      {
        Serial.println("Could not connect to MQTT server");
      }
    }
  }
}

void wifi_and_mqtt_reconnect()
{
    delay(10);
    Serial.println();
    Serial.print("Re-connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    delay (1000);
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        mqtt_connect("reconnect_wifi_and_mqtt");
    }
}

void setup() {
  Serial.begin(115200);
  PRINT("SETUP START. Firmware ", firmware_version);
  PRINTS("\n");
  PRINTS("SETUP. Wi-Fi init begin\n");
  while (status != WL_CONNECTED) 
  { 
    PRINTS("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin ( ssid, password );
    delay(3000);
  }
  PRINTS("\nSETUP. wi-fi init passed\n");

  PRINTS("SETUP. MQTT init begin\n");
  mqtt_connect("mqtt_init");
  PRINTS("\nSETUP. MQTT init passed\n");

  PRINTS("SETUP. PIN assignments begin\n");  
  pinMode(PIN_WATERLEVEL, INPUT);         // пин вход - на нём датчики воды, оба    
  pinMode(PIN_WATERLEVEL_POWER, OUTPUT);  // пин выход - на нём питание датчиков
  pinMode(PIN_EXTERNAL_CURRENT, INPUT);   // пин вход - на нём внешнее питание из цепи внешней неконтролируемой лампочки, гальванически развязанное через оптопару
  pinMode(PIN_RELAY_FAN, OUTPUT);         // пин управления реле вентилятора
  pinMode(PIN_RELAY_LIGHT, OUTPUT);       // пин управления реле освещения
  pinMode(PIN_BEEPER, OUTPUT);            // пин пищалки
  pinMode(PIN_GAS_SENSOR, INPUT);         // датичк газоанализатора - аналоговый вход
  PRINTS("SETUP. Pin assignment end, pause 3 sec \n");
  delay(3000);

  PRINTS("SETUP. HTTP Server funcs init start.\n"); 

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", indexpage);
  });
  server.on("/updater", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", updater);
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { 
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { 
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  PRINTS("SETUP. HTTP Server funcs init end.\n"); 
  PRINTS("SETUP. HTTP Server start.\n"); 
  server.begin();
  
  PRINTS("SETUP. MQTT initial publishing start.\n"); 
  mqttclient.publish(topic_firmware, firmware_version, retained);
  mqttclient.publish(topic_ip, WiFi.localIP().toString().c_str(), retained);
  mqttclient.publish(topic_uptime,uptime().c_str(), retained);
  mqttclient.publish(topic_wsensors,"0", retained); 
  mqttclient.publish(topic_time1,"0030", retained); 
  mqttclient.publish(topic_time2,"0830", retained);
  mqttclient.publish(topic_msg,"Started", retained);
  PRINTS("SETUP. MQTT initial publishing end.\n");

  PRINTS("END SETUP. Create tasks.\n");
  xTaskCreate(reportUptimeTask, "printUptimeTask", 5000, NULL, 1, NULL);          // Постит аптайм раз в минуту в UART и MQTT
  xTaskCreate(reportLightStateTask, "printStateTask", 5000, NULL, 1, NULL);       // Постит статус малого света в MQTT
  xTaskCreate(reportWaterStateTask, "printWaterStateTask", 5000, NULL, 1, NULL);  // Постит статус датчиков протечки раз в секунду в UART и MQTT
  xTaskCreate(reportGasStateTask, "reportGasStateTask", 5000, NULL, 1, NULL);     // Постит состояние датчика газоанализатора раз в 2 секунды в UART и MQTT
  xTaskCreate(mqttClientCycleTask, "mqttClientCycleTask", 10000, NULL, 1, NULL);  // цикл MQTT клиента с проверкой соединения (и реконнектом, если разорвалось)
  xTaskCreate(otaUpdaterCycleTask, "otaUpdaterCycleTask", 5000, NULL, 1, NULL);   // цикл OTA обновлятора (веб-сервер) - TODO: сделать секьюрити по IP
  xTaskCreate(wifiControlCycleTask, "wifiControlCycleTask", 3000, NULL, 1, NULL); // цикл проверки wi-fi соединения, если порвалось, реконнект вайфая и MQTT
  xTaskCreate(rssiCycleTask, "wifiControlCycleTask", 3000, NULL, 1, NULL);        // цикл накопления данных о качестве wi-fi соединения, усреднения и постинга в MQTT
  PRINTS("END SETUP. End creating tasks.\n");
}

void reportUptimeTask( void * parameter ) // Постит аптайм раз в минуту в UART и MQTT
{
  const TickType_t xDelay = 60000 / portTICK_PERIOD_MS;
  for(;;){ 
    Serial.println(uptime());
    mqttclient.publish(topic_uptime,uptime().c_str(), retained);
    vTaskDelay( xDelay );
  }
}

void rssiCycleTask( void * parameter ) // цикл накопления данных о качестве wi-fi соединения
{
  const TickType_t xDelay = 10000 / portTICK_PERIOD_MS;
  for(;;){ 
    // Уровень сигнала - собираем 6 статусов раз в 10 секунд, усредняем их и постим раз в минуту.
    if (rssiCount == 5)
    {
      long rssiAvg = round((rssiSum/rssiCount));
      mqttclient.publish(topic_rssi, String(rssiAvg).c_str(), retained);
      rssiCount = 0;
      rssiSum = 0;
    }
    else
    {
      long rssi = WiFi.RSSI();
      rssiSum = rssiSum + rssi;
      rssiCount++;
    }
    vTaskDelay( xDelay );
  }
}

void reportWaterStateTask( void * parameter ) // Постит статус датчиков протечки и внешнего света раз в секунду в UART и MQTT (только если они изменились)
{
  const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
  for(;;){ 
    digitalWrite(PIN_WATERLEVEL_POWER, HIGH); // Подаём питание на датчики воды
    delay(100); // задержка 100 мс чтобы датчик точно был готов выдать нормальные данные, хз сколько ему надо времени чтобы раскачаться после подачи напряжения
    int ws = analogRead(PIN_WATERLEVEL);
    digitalWrite(PIN_WATERLEVEL_POWER, LOW);
    char* ws_ = (p2dig(ws, DEC));     
    PRINT(" WATER SENSORS:", ws_);
    PRINTS(";\n ");
    if (ws != 0) mqttclient.publish(topic_wsensors, ws_, retained);
    vTaskDelay( xDelay );
  }
}

void reportGasStateTask( void * parameter ) // Постит инфу с датчика газоанализатора раз в секунду в UART и MQTT (только в случае если изменилась)
{
  const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;
  for(;;){ 
    int gas = analogRead(PIN_GAS_SENSOR);
    prev_gas[gas_i] = gas;
    if (gas_i <= 4) gas_i++;
    else gas_i = 0;
    int gas_avg = round((prev_gas[0] + prev_gas[1] + prev_gas[2] + prev_gas[3] + prev_gas[4] + prev_gas[5])/6);
    PRINT("GAS SENSOR(AVG5):", String(gas_avg).c_str());
    PRINTS(";\n ");
    if (gas_avg != prev_gas_avg)
    {
      mqttclient.publish(topic_gas, String(gas_avg).c_str());
      prev_gas_avg = gas_avg;
    }
    vTaskDelay( xDelay );
  }
}

void reportLightStateTask( void * parameter ) // Постит статус внешнего света в MQTT (только если изменилось)
{
  const TickType_t xDelay = 200 / portTICK_PERIOD_MS;
  for(;;){ 
    external_light = !digitalRead(PIN_EXTERNAL_CURRENT);
    if (external_light != prev_external_light) { mqttclient.publish(topic_ext_light_info, String(external_light).c_str(), retained); }
    prev_external_light = external_light;
    vTaskDelay( xDelay );
  }
}

void mqttClientCycleTask( void * parameter ) // цикл MQTT клиента с проверкой соединения (и реконнектом, если разорвалось)
{
  const TickType_t xDelay = 200 / portTICK_PERIOD_MS;
  for(;;){ 
    if (mqttclient.connected()) mqttclient.loop();  else mqtt_connect("mqtt_reconnect");
    vTaskDelay( xDelay );
  }
}

void otaUpdaterCycleTask( void * parameter ) // цикл OTA обновлятора (веб-сервер) - TODO: сделать секьюрити по IP
{
  const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
  for(;;){ 
    server.handleClient(); // веб-сервер на 80 порту
    vTaskDelay( xDelay );
  }
}

void wifiControlCycleTask( void * parameter ) // цикл проверки всё ли норм с wi-fi соединением и если не норм то реконнект
{
  const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;
  for(;;){ 
    if (WiFi.status() != WL_CONNECTED) wifi_and_mqtt_reconnect();
    vTaskDelay( xDelay );
  }
}

void loop() {
  while(true) {}
}