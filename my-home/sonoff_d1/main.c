/*
 * Sonoff D1
 */

#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <udplogger.h>
#include "wifi.h"

float bri;
bool on;

void setup_uart() {
    uart_set_baud(0, 9600);
}

static void tsoftuart_task(void *pvParameters)
{
    uint8_t buffer[17] = { 0xAA,0x55,0x01,0x04,0x00,0x0A,0x00,0x64,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00 };

    buffer[6] = on ? 1 : 0;
    buffer[7] = 100;

    UDPLUO("Sent: ");
    for (uint32_t i = 0; i < sizeof(buffer); i++) {
        if ((i > 1) && (i < sizeof(buffer) -1)) { buffer[16] += buffer[i]; }
        uart_putc(0, buffer[i]);
        UDPLUO("%02x.", buffer[i]);
    }
    UDPLUO("\n");

    vTaskDelay(200 / portTICK_PERIOD_MS);

    vTaskDelete(NULL);
}

static void wifi_init()
{
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

homekit_value_t light_on_get() { return HOMEKIT_BOOL(on); }

void light_on_set(homekit_value_t value)
{
    if (value.format != homekit_format_bool)
    {
        UDPLUO("Invalid on-value format: %d\n", value.format);
        return;
    }
    on = value.bool_value;
    xTaskCreate(&tsoftuart_task, "tsoftuart1", 256, NULL, 1, NULL);
}

homekit_value_t light_bri_get() { return HOMEKIT_INT(bri); }

void light_bri_set(homekit_value_t value)
{
    if (value.format != homekit_format_int)
    {
        UDPLUO("Invalid bri-value format: %d\n", value.format);
        return;
    }
    bri = value.int_value;
    xTaskCreate(&tsoftuart_task, "tsoftuart1", 256, NULL, 1, NULL);
}

void light_identify_task(void *_args)
{
    //Identify Sonoff by Pulsing LED.
    for (int j = 0; j < 3; j++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int i = 0; i <= 40; i++)
            {
                int w;
                float b;
                w = (UINT16_MAX - UINT16_MAX * i / 20);
                if (i > 20)
                {
                    w = (UINT16_MAX - UINT16_MAX * abs(i - 40) / 20);
                }
                b = 100.0 * (UINT16_MAX - w) / UINT16_MAX;
                //pwm_set_duty(w);
                UDPLUO("Light_Identify: i = %2d b = %3.0f w = %5d\n", i, b, UINT16_MAX);
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    //pwm_set_duty(0);
    //lightSET();
    vTaskDelete(NULL);
}

void light_identify(homekit_value_t _value)
{
    UDPLUO("Light Identify\n");
    xTaskCreate(light_identify_task, "Light identify", 256, NULL, 2, NULL);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Sonoff Dimmer");

homekit_characteristic_t lightbulb_on = HOMEKIT_CHARACTERISTIC_(ON, false, .getter = light_on_get, .setter = light_on_set);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(
            .id = 1,
            .category = homekit_accessory_category_switch,
            .services = (homekit_service_t *[]){
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
                                .characteristics = (homekit_characteristic_t *[]){
                                    &name,
                                    HOMEKIT_CHARACTERISTIC(MANUFACTURER, "iTEAD"),
                                    HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "PWM Dimmer"),
                                    HOMEKIT_CHARACTERISTIC(MODEL, "Sonoff D1"),
                                    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1.6"),
                                    HOMEKIT_CHARACTERISTIC(IDENTIFY, light_identify),
                                    NULL}),
                HOMEKIT_SERVICE(LIGHTBULB, .primary = true, .characteristics = (homekit_characteristic_t *[]){HOMEKIT_CHARACTERISTIC(NAME, "Sonoff Dimmer"), &lightbulb_on, HOMEKIT_CHARACTERISTIC(BRIGHTNESS, 100, .getter = light_bri_get, .setter = light_bri_set), NULL}), NULL}),
    NULL};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = HOMEKIT_CODE
};

void create_accessory_name()
{
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);

    int name_len = snprintf(NULL, 0, "Sonoff D1 %02X:%02X:%02X",
                            macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len + 1);
    snprintf(name_value, name_len + 1, "Sonoff D1 %02X:%02X:%02X",
             macaddr[3], macaddr[4], macaddr[5]);

    name.value = HOMEKIT_STRING(name_value);
}

void user_init(void)
{
    udplog_init(2);
    setup_uart();

    create_accessory_name();

    wifi_init();
    homekit_server_init(&config);
}
