#ifndef __LAB07_MOTOR_H_INCLUDED_
#define __LAB07_MOTOR_H_INCLUDED_

#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h> 
#include <task.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pwm.h"

// PINS
#define CT1   12
#define CT2   13
#define PULSE 9 // DEBUG TOOL

// MACROS
#define MIN_PRIORITY 1 
#define LOW          0 
#define HIGH         1
#define VELOCITY_SAMPLES 16
#define DEADBAND 50

#define min(x, y) (((x) > (y)) ? (y) : (x))
#define max(x, y) (((x) < (y)) ? (x) : (y))
#define abs(x)    ((x) < 0) ? ((x) * -1) : (x)

typedef enum {M_SLOW, M_MEDIUM, M_FAST} motor_speed_t;
typedef enum {M_BUSY, M_IDLE} motor_status_t;

typedef enum {
        IRQ_LEVEL_LOW  = 0x1,
        IRQ_LEVEL_HIGH = 0x2,
        IRQ_EDGE_FALL  = 0x4,
        IRQ_EDGE_RISE  = 0x8
} irq_event_t;

// GLOBAL VARIABLES
extern int32_t  _encoder;
extern uint32_t _velocitySamples[VELOCITY_SAMPLES];
extern uint32_t _velocityNext;
extern motor_status_t _motor_status;
extern int32_t _motorSetpointPosition;

// FUNCTION DECLARATIONS
void motor_init(void);
void motorDrive(int32_t drive);
void motor_set_position(int32_t position);
int32_t motor_get_position(void);
int32_t motor_get_velocity(void);
motor_status_t motor_status(void);
void motor_speed_limit(motor_speed_t speed);
void motor_move(uint32_t pos_in_tics);
uint32_t _readTimer(void);

#endif