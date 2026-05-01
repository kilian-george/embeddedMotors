#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

static void spinDelayUs(uint32_t us) {
    while(us--) {
        for (int i = 0; i < 12; i++) {
            __asm("    nop\n");
        }
        __asm(  "    nop\n"
                "    nop\n"
                "    nop\n"
                "    nop\n"
                "    nop\n"
                "    nop\n"
                "    nop\n"
                "    nop\n");
    }
}

static void spinDelayMs(uint32_t ms) {
    while(ms--) {
        spinDelayUs(1000);
    }
}

// Candidate for my assert Module
void _assert_failed (const char *assertion, const char *file, unsigned int line) {
    const uint8_t LED_PIN = 25;
    const uint32_t assertLoopDelayMs = 50;
    
    // Not alot that we can do at the current time so simply blink the LED rapidly
    // Normally an IO would display: 'Assertion failed: expression, file filename, line number'
    printf("Assertion Failed: %s at %s::%d\n", assertion, file, line);
    vTaskDelay(10);
    // vTaskSuspendAll();
    taskENTER_CRITICAL();

    // Denergize any outputs if task mode disable context switching
    // Do I disable Interrupts?
    // Enable the GPIO port that is used for the on-board LED.
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // turn off the LED
    gpio_put(LED_PIN, 1);
    
    // Loop forever.
    while (true) {
        printf("Failed Assert\n");
        gpio_put(LED_PIN, 0);
        spinDelayMs(assertLoopDelayMs);
        gpio_put(LED_PIN, 1);
        spinDelayMs(assertLoopDelayMs);
    }
}


#pragma weak _close
#pragma weak _lseek

int _close(int file) { return -1; }
int _lseek(int file, int ptr, int dir) { return -1; }