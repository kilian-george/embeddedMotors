/**
 * @file    master.c
 * @brief   Starting point for WSU (Vancouver) cs466 Embedded Systems Lab5
 * @author  Venessa Kuchenik
 * @date    2026-04-05
 */
#include "master.h"

// SEMAPHORE HANDLERS
static SemaphoreHandle_t _sw1 = NULL;
static SemaphoreHandle_t _sw2 = NULL;

uint8_t  sw1_pressed     = 0;
int32_t  positions[9]    = {0, 435, 870, 1305, 1740, 3480, 5220, -5220, -3480};
uint8_t  position_index  = 0;
int32_t  target_position = 0;

void gpio_callback(uint gpio, uint32_t event) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    gpio_put(LED, HIGH);

    if (gpio == SW1) {
        sw1_pressed = 1;
        xSemaphoreGiveFromISR(_sw1, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    } else if (gpio == SW2 && sw1_pressed) {
        xSemaphoreGiveFromISR(_sw2, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    } else if (gpio == PHA_PIN){
        if (gpio_get(PHB_PIN)) _encoder++;
        else _encoder--;
    }
}


void hardware_init(void) {
    gpio_init(LED);
    gpio_init(RED);
    gpio_init(GREEN);
    gpio_init(MOSI);
    gpio_init(CLK);
    gpio_init(CS);
    gpio_init(MISO);
    gpio_init(PHA_PIN);
    gpio_init(PHB_PIN);
    gpio_init(SW1);
    gpio_init(SW2);
    
    gpio_set_dir(LED,     GPIO_OUT);
    gpio_set_dir(RED,     GPIO_OUT);
    gpio_set_dir(GREEN,   GPIO_OUT);
    gpio_set_dir(MOSI,    GPIO_OUT);
    gpio_set_dir(CLK,     GPIO_OUT);
    gpio_set_dir(CS,      GPIO_OUT);
    gpio_set_dir(MISO,    GPIO_IN);
    gpio_set_dir(PHA_PIN, GPIO_IN);
    gpio_set_dir(PHB_PIN, GPIO_IN);
    gpio_set_dir(SW1,     GPIO_IN);
    gpio_set_dir(SW2,     GPIO_IN);

    setCLK(LOW);    // set serial clock LOW
    setCS(HIGH);    // Chip Select set to HIGH so child device knows were not trying to talk to it yet

    gpio_pull_up(SW1);
    gpio_pull_up(SW2);

    gpio_put(RED, HIGH);
    gpio_put(GREEN, HIGH);

    // Create Semaphores
    _sw1 = xSemaphoreCreateBinary();
    _sw2 = xSemaphoreCreateBinary();

    gpio_set_irq_enabled_with_callback(PHA_PIN, GPIO_IRQ_EDGE_RISE , true, &gpio_callback);    
    // gpio_set_irq_enabled(PHB_PIN, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SW1, GPIO_IRQ_EDGE_FALL, true);    
    gpio_set_irq_enabled(SW2, GPIO_IRQ_EDGE_FALL, true);    
}


void heartbeat(void *none) {
    static int value = -100;
    static int count = 0;

    while(1) {
        gpio_put(LED, count++ & 1);
        // setPWMDuty(value);
        // motorDrive(value);
        // value += 20;
        // if (value >= 100) value = -100;
        // printf("value: %d\n", value);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sw1_click(void *none) {
    while(1) {
        if (xSemaphoreTake(_sw1, portMAX_DELAY) == pdPASS) {
            gpio_put(LED, LOW);
            printf("SW1 pressed\n");
            position_index = (position_index + 1) % 9;

            target_position = positions[position_index];
            write(MOTOR_CMD_0,  (uint8_t)(target_position & 0x000000FF));
            write(MOTOR_CMD_8,  (uint8_t)((target_position >> 8) & 0x000000FF));
            write(MOTOR_CMD_16, (uint8_t)((target_position >> 16) & 0x000000FF));
            write(MOTOR_CMD_24, (uint8_t)((target_position >> 24) & 0x000000FF));
            write(CMD_REG, SET_MOTOR);
            // perhaps a delay here while child copies the 4 MOTOR_CMD registers to the "PID motor setpoint"
            motor_set_position(target_position);
            
            // motorDrive(90);

        }
    }
}

void sw2_click(void *none) {
    while(1) {
        if (xSemaphoreTake(_sw2, portMAX_DELAY) == pdPASS) {
            gpio_put(LED, LOW);
            printf("SW2 pressed\n");
            if (position_index == 0) position_index = 9;
            else position_index -= 1;
            target_position = positions[position_index];

            write(MOTOR_CMD_0,  (uint8_t)(target_position & 0x000000FF));
            write(MOTOR_CMD_8,  (uint8_t)((target_position >> 8) & 0x000000FF));
            write(MOTOR_CMD_16, (uint8_t)((target_position >> 16) & 0x000000FF));
            write(MOTOR_CMD_24, (uint8_t)((target_position >> 24) & 0x000000FF));
            write(CMD_REG, SET_MOTOR);
            // perhaps a delay here while child copies the 4 MOTOR_CMD registers to the "PID motor setpoint"
            motor_set_position(target_position);
        }
    }
}

int main(void) {
    #ifdef ACTUAL_SERIAL
        stdio_uart_init();
    #else
        stdio_init_all();
    #endif

    sleep_ms(100);           // give time for serial bus to init
    hardware_init();
    motor_init();

    motorDrive(90);
    // create tasks
    xTaskCreate(sw1_click, "sw1_click", 1024, NULL, MIN_PRIORITY+3, NULL);
    xTaskCreate(sw2_click, "sw2_click", 1024, NULL, MIN_PRIORITY+3, NULL);
    xTaskCreate(heartbeat, "hearbeat", 256, NULL, MIN_PRIORITY, NULL); 

    vTaskStartScheduler();
    
    while(1){};
}