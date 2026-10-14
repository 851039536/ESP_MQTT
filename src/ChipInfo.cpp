#include <Arduino.h>
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "ChipInfo.h"
// 打印ESP芯片信息
void printChipInfo() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    Serial.printf("-------------------ESP32 info-------------------------%s\n","");
    Serial.printf("Chip model: %s\n", CONFIG_IDF_TARGET);
    Serial.printf("Chip revision: %d\n", chip_info.revision);
    Serial.printf("CPU cores: %d\n", chip_info.cores);
    Serial.printf("Wi-Fi: %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");
    Serial.printf("Bluetooth: %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "Yes" : "No");
    Serial.printf("Bluetooth LE: %s\n", (chip_info.features & CHIP_FEATURE_BLE) ? "Yes" : "No");

    uint32_t flash_size = 0;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        Serial.printf("Flash size: %u MB\n", flash_size / (1024 * 1024));
    }
    Serial.printf("----------------------------------------------------%s\n","");
}