#include <stdlib.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "motor.h"

#define MIN_PRIORITY 1
#define LOW          0
#define HIGH         1

#define LED          25             
#define MOSI         19             
#define CLK          18             
#define CS           17            
#define MISO         16
#define PHA_PIN      14
#define PHB_PIN      15


#define SET_MOTOR    ((uint8_t)0x01)    // CMD
#define GET_MOTOR    ((uint8_t)0x00)    // CMD


#define getCS()    gpio_get(CS)
#define getCLK()   gpio_get(CLK)
#define getMOSI()  gpio_get(MOSI)
#define getMISO()  gpio_get(MISO)
#define setMISO(x) do { gpio_put(MISO, ((x) != 0));     sleep_us(500); } while(0)

// ENUMS
typedef enum {  // registers
    MOTOR_POS_0,        // Address 0
    MOTOR_POS_8,        // Address 1
    MOTOR_POS_16,       // Address 2
    MOTOR_POS_24,       // Address 3

    MOTOR_CMD_0,        // Address 4
    MOTOR_CMD_8,        // Address 5
    MOTOR_CMD_16,       // Address 6
    MOTOR_CMD_24,       // Address 7

    CMD_REG,            // Address 8
    STAT_REG            // Address 9
} Register;

typedef enum {  // state machine
    IDLE,
    CMD,
    READ,
    WRITE,
    DONE
} State;

enum {          // states
    CS_HIGH,
    CS_LOW,
    CLK_HIGH,
    CLK_LOW
};

// QUEUE DATA STRUCTURE
typedef struct {
    union {         // union tells compiler to read the following byte in two ways
        struct {
            uint8_t cs_clk  : 4;  // Bits 0-3
            uint8_t mosi    : 4;  // Bits 4-7
        } nibbles;
        uint8_t encodedByte;
    };
} queueData;

// Name handlers
static QueueHandle_t Queue = NULL;