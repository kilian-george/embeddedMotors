/**
 * @file    lab7.c
 * @brief   Starting point for WSU (Vancouver) cs466 Embedded Systems Lab2
 * @author  Sage F
 * @date    2026-04-14
 */
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "motor.c"
#include "pwm.c"

const uint8_t LED_PIN = 25;

const uint8_t SW1_PIN = 7;
const uint8_t SW2_PIN = 8;

volatile int32_t encoderTicks = 0;
volatile uint64_t RPMarr[50] = { 0 };
volatile uint32_t arrCount = 0;

const uint32_t minPriority = 1;

const uint16_t position[9] = { 435, 870, 1305, 1740, 3480, 5220, -5330, -3480, 0 };
volatile int8_t cyclePos = 0;

uint32_t rpmCalc();

static SemaphoreHandle_t xmotorMoveSem = NULL;

// ISR
void gpio_int_callback(uint gpio, uint32_t events_unused) 
{
    if (gpio == PHA_PIN) {
        //RPMarr[(arrCount % 50)] = to_ms_since_boot(get_absolute_time());
        //arrCount++;
        if (gpio_get(PHB_PIN))
            encoderTicks++;
        else
            encoderTicks--;
    }
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (gpio == SW1_PIN) {
        cyclePos = (cyclePos + 1) % 9;
        xSemaphoreGiveFromISR(xmotorMoveSem, &xHigherPriorityTaskWoken);
    }

    if (gpio == SW2_PIN) {
        cyclePos = (cyclePos == 0) ? 8 : cyclePos - 1;
        xSemaphoreGiveFromISR(xmotorMoveSem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void hardware_init(void)
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    /*
    gpio_init(PHA_PIN);
    //gpio_pull_up(PHA_PIN);
    gpio_set_dir(PHA_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PHA_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);

    gpio_init(PHB_PIN);
    //gpio_pull_up(PHB_PIN);
    gpio_set_dir(PHB_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PHB_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);
*/
    gpio_init(SW1_PIN);
    gpio_pull_up(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);

}

// Idle task
void heartbeat(void * notUsed)
{   
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED_PIN, 0);

        // uint32_t rpm = rpmCalc();

        // printf("RPM %d, ", rpm);
        printf("Pos: %d\n", (encoderTicks % 300));
    }
}

uint32_t rpmCalc(){

    if (arrCount < 50)
            return 0;

    uint32_t start = (arrCount - 50) % 50;        
    uint64_t total = 0;
    uint32_t rpm = 0;
    
    taskENTER_CRITICAL();
    for (uint8_t i = 1; i < 50; i++) {
        uint32_t idx1 = (start + i) % 50;
        uint32_t idx0 = (start + i - 1) % 50;
        total += RPMarr[idx1] - RPMarr[idx0];
    }
    taskEXIT_CRITICAL();

    uint64_t avg = total / (50-1);
    return (uint32_t)(60000 / (avg * 300));
}

void MotorMover(void *notUsed)
{
    for (;;)
    {
        xSemaphoreTake(xmotorMoveSem, portMAX_DELAY);
        printf("Changing Position.\n");

        taskENTER_CRITICAL();
        uint8_t pos = cyclePos;
        taskEXIT_CRITICAL();

        motor_set_position(position[pos]);
    }
}

int main(void)
{
    stdio_init_all();

    xmotorMoveSem = xSemaphoreCreateBinary(); 

    hardware_init();
    motor_init();
    
    xTaskCreate(heartbeat, "heartbeatTask", 256, NULL, minPriority, NULL);
    xTaskCreate(MotorMover, "MotorMover", 256, NULL, minPriority, NULL);
    //xTaskCreate(_pidPositionServo, "pid", 256, NULL, minPriority, NULL);

    vTaskStartScheduler();
    while(1){};
}
