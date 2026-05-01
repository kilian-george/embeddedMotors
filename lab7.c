/**
 * @file    lab7.c
 * @brief   Starting point for WSU (Vancouver) cs466 Embedded Systems Lab2
 * @author  Sage F
 * @date    2026-04-14
 */
#include "master.h"

const uint32_t minPriority = 1;


// SEMAPHORE HANDLERS
static SemaphoreHandle_t _sw1 = NULL;
static SemaphoreHandle_t _sw2 = NULL;

uint8_t  sw1_pressed     = 0;
int32_t  positions[9]    = {435, 870, 1305, 1740, 3480, 5220, -5220, -3480, 0};
uint8_t  position_index  = 0;
int32_t  target_position = 0;

uint32_t encoderTicks = 0;

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

   if (gpio == SW1) {
        sw1_pressed = 1;
        xSemaphoreGiveFromISR(_sw1, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    } 
    else if (gpio == SW2 && sw1_pressed) {
        xSemaphoreGiveFromISR(_sw2, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void hardware_init(void)
{
    gpio_init(LED);
    gpio_init(RED);
    gpio_init(GREEN);
    gpio_init(MOSI);
    gpio_init(CLK);
    gpio_init(CS);
    gpio_init(MISO);
    // gpio_init(PHA_PIN);
    // gpio_init(PHB_PIN);
    gpio_init(SW1);
    gpio_init(SW2);

    gpio_set_dir(LED,     GPIO_OUT);
    gpio_set_dir(RED,     GPIO_OUT);
    gpio_set_dir(GREEN,   GPIO_OUT);
    gpio_set_dir(MOSI,    GPIO_OUT);
    gpio_set_dir(CLK,     GPIO_OUT);
    gpio_set_dir(CS,      GPIO_OUT);
    gpio_set_dir(MISO,    GPIO_IN);
    // gpio_set_dir(PHA_PIN, GPIO_IN);
    // gpio_set_dir(PHB_PIN, GPIO_IN);
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

    gpio_set_irq_enabled(SW1, GPIO_IRQ_EDGE_FALL, true);    
    gpio_set_irq_enabled(SW2, GPIO_IRQ_EDGE_FALL, true);   

}

// Idle task
void heartbeat(void * notUsed)
{   
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED, 0);

        // uint32_t rpm = rpmCalc();

        // printf("RPM %d, ", rpm);
        printf("Pos: %d, target: %d\n", (encoderTicks % 300), target_position);
    }
}

#if 0
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
#endif

void sw1_click(void *none) {
    while(1) {
        if (xSemaphoreTake(_sw1, portMAX_DELAY) == pdPASS) {
            gpio_put(LED, LOW);
            printf("SW1 pressed\n");
            position_index = (position_index + 1) % 9;

            target_position = positions[position_index];
            //motorDrive(90);
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
        }
    }
}

#if 0
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
#endif

int main(void)
{
    stdio_init_all();

    xmotorMoveSem = xSemaphoreCreateBinary(); 
    _sw1 = xSemaphoreCreateBinary(); 
    _sw2 = xSemaphoreCreateBinary(); 

    hardware_init();
    motor_init();
    
    xTaskCreate(heartbeat, "heartbeatTask", 256, NULL, minPriority, NULL);
    //xTaskCreate(MotorMover, "MotorMover", 256, NULL, minPriority, NULL);
    xTaskCreate(sw1_click, "sw1_click", 1024, NULL, minPriority+3, NULL);
    xTaskCreate(sw2_click, "sw2_click", 1024, NULL, minPriority+3, NULL);
    //xTaskCreate(_pidPositionServo, "pid", 256, NULL, minPriority, NULL);

    vTaskStartScheduler();
    while(1){};
}
