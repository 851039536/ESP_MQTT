#include <WiFi.h>
#include <PubSubClient.h>
#include "ChipInfo.h"

#define LED2 2
const int relayPin = 13;

void connectToMqttBroker();
void callback(char *topic, const byte *payload, unsigned int length);



// WiFi配置
const char *ssid = "kkk";
const char *password = "11223344";

// MQTT配置
const char *mqtt_broker = "192.168.139.206";
const char *topic = "TOPIC1";
const char *mqtt_username = "user1";
const char *mqtt_password = "1234";
const int mqtt_port = 1883;

// WiFi 和 MQTT 客户端
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {


    // 初始化 LED 引脚
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, LOW);
    pinMode(relayPin, OUTPUT);

    // 初始化串行通信
    Serial.begin(115200);

    // 连接到 WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");
    // 调用封装好的方法打印芯片信息
    printChipInfo();

    // 配置 MQTT 客户端
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

    // 连接到MQTT服务器
    connectToMqttBroker();

}

void callback(char *topic, const byte *payload, const unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    if (msg.equals("onled2")) {
        Serial.println("on:"+msg);
        digitalWrite(LED2, HIGH);

    } else if (msg.equals("off")) {
        Serial.println("off:"+msg);
        digitalWrite(LED2, LOW);
    } else {
        Serial.println("Received:"+msg);
    }
}

void connectToMqttBroker() {
    while (!client.connected()) {
        String client_id = "esp32s2-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public TOPIC1 mqtt broker connected");
            client.subscribe(topic);
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
}

void loop() {

    digitalWrite(relayPin, HIGH); // 打开继电器
    Serial.println("13 ON");
    delay(1000); // 延时1秒
    digitalWrite(relayPin, LOW); // 关闭继电器
    Serial.println("13 OFF");
    delay(1000); // 延时1秒


    if (!client.connected()) {
        connectToMqttBroker();
    }
    client.loop();
}