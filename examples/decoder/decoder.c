/* Respond to a button press.
 *
 * This code combines two ways of checking for a button press -
 * busy polling (the bad way) and button interrupt (the good way).
 *
 * This sample code is in the public domain.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

/* pin config */
const int gpio = 5;   /* gpio 0 usually has "PROGRAM" button attached */
const gpio_inttype_t int_type = GPIO_INTTYPE_EDGE_ANY;
#define GPIO_HANDLER gpio05_interrupt_handler
#define MAX_PULSES 1024

// 0 = Ready
// 1 = Receiving
// 2 = Sending
volatile uint8_t irState = 0;
uint32_t pulses[MAX_PULSES];
uint32_t pulseCount = 0;
uint32_t lastPulseTime = 0;
uint32_t commandEndThresholdUs = 400000;

void buttonIntTask(void *pvParameters)
{
    printf("Waiting for button press interrupt on gpio %d...\r\n", gpio);
    gpio_set_interrupt(gpio, int_type);

    while (1) {
        uint32_t now = sdk_system_get_time();

        if (irState == 1 && now - lastPulseTime > commandEndThresholdUs) {
            printf("START!\n");
            int i;
            for (i = 0; i < pulseCount; i++) {
                printf("%u\n", pulses[i]);
            }
            printf("END!\n");

            irState = 0;
            pulseCount = 0;
        } else {
            printf(".");
        }

        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void GPIO_HANDLER(void)
{
    uint32_t now = sdk_system_get_time();

    if (irState == 0) {
        pulseCount = 0;
        irState = 1;
    } else if (irState == 1) {
        uint32_t pulseDuration = now - lastPulseTime;
        if (pulseCount < MAX_PULSES) {
            pulses[pulseCount] = pulseDuration;
            pulseCount++;
        } else {
            printf("Max pulses exceeded\n");
            irState = 0;
            pulseCount = 0;
        }
    } else if (irState == 2) {

    }

    lastPulseTime = now;
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    gpio_enable(gpio, GPIO_INPUT);

    xTaskCreate(buttonIntTask, (signed char *)"buttonIntTask", 256, NULL, 2, NULL);
}
