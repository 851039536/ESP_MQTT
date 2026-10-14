#include <WiFi.h>
#include <PubSubClient.h>
#include "ChipInfo.h"
#include "CommandParser.h"
#include "RelayController.h"

#define LED2 2
const int relayPin = 13;

// 心跳闪烁已确认属于调试遗留，默认关闭；需要时把第四个参数改为周期毫秒数即可。
// （原实现在 loop() 里用 delay(1000) 无条件翻转继电器，既阻塞又使继电器无法被 MQTT 控制。）
const unsigned long RELAY_BLINK_PERIOD_MS = 0;

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

// LED 写入回调：把副作用集中在一处，便于 RelayController 在测试中注入假实现。
void writeLedPin(int pin, bool on) {
    digitalWrite(pin, on ? HIGH : LOW);
}

RelayController led(LED2, RELAY_BLINK_PERIOD_MS, writeLedPin);

void setup() {
    // 初始化 LED 与继电器引脚
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, LOW);
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);

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
    // 解析逻辑已抽到纯函数中（可在 PC 上单元测试），此处只做副作用派发：
    // 不再把 payload 逐字节拼进 String，因此没有长度上限与堆分配风险。
    const Command command = parseCommand(reinterpret_cast<const char *>(payload), length);

    switch (command) {
        case Command::LedOn:
            Serial.println("Received: LED_ON");
            led.setDesiredState(true);
            break;
        case Command::LedOff:
            Serial.println("Received: LED_OFF");
            led.setDesiredState(false);
            break;
        case Command::Unknown:
        default:
            break; // 未知命令忽略，不打印 payload 内容
    }
}

void connectToMqttBroker() {
    while (!client.connected()) {
        String client_id = "esp32-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the mqtt broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("TOPIC1 mqtt broker connected");
            client.subscribe(topic);
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
}

void loop() {
    // 非阻塞：原实现的两个 delay(1000) 已移除，
    // 因此 MQTT 下行命令不再被拖慢（原先延迟可达约 2 秒）。
    led.update(millis());

    if (!client.connected()) {
        connectToMqttBroker();
    }
    client.loop();
}
