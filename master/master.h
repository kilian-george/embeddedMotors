#ifndef __MASTER_H_INCLUDED_
#define __MASTER_H_INCLUDED_

#include <stdlib.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "motor.h"
#include "pwm.h"

// PINS
#define RED          0
#define GREEN        1
#define SW2          7
#define SW1          8
// #define PULSE        9   // in motor.h
// #define PWM          10  // in pwm.h
// #define CT1          12  // in motor.h
// #define CT2          13  // in motor.h
#define PHA_PIN      14
#define PHB_PIN      15
#define MISO         16
#define CS           17
#define CLK          18
#define MOSI         19
#define LED          25

// Giving Names to Register Addresses and Commands
#define MOTOR_POS_0  ((uint8_t)0x00)    // ADDR
#define MOTOR_POS_8  ((uint8_t)0x01)    // ADDR
#define MOTOR_POS_16 ((uint8_t)0x02)    // ADDR
#define MOTOR_POS_24 ((uint8_t)0x03)    // ADDR
#define MOTOR_CMD_0  ((uint8_t)0x04)    // ADDR
#define MOTOR_CMD_8  ((uint8_t)0x05)    // ADDR
#define MOTOR_CMD_16 ((uint8_t)0x06)    // ADDR
#define MOTOR_CMD_24 ((uint8_t)0x07)    // ADDR
#define CMD_REG      ((uint8_t)0x08)    // ADDR
#define STAT_REG     ((uint8_t)0x09)    // ADDR

#define SET_MOTOR    ((uint8_t)0x01)    // CMD
#define GET_MOTOR    ((uint8_t)0x00)    // CMD
#define READ         ((uint8_t)0x0A)    // CMD
#define WRITE        ((uint8_t)0x0B)    // CMD


// Macros to set the ChipSelect, Clock, MasterOut and read the MasterIn signals..
// Needed a delay so my tools could see it.
#define setCS(x)   do { gpio_put(CS,   ((x) != 0));     sleep_us(500); } while(0)
#define setCLK(x)  do { gpio_put(CLK,  ((x) != 0));     sleep_us(500); } while(0)
#define setMOSI(x) do { gpio_put(MOSI, ((x) != 0));     sleep_us(500); } while(0)
#define getMISO()  gpio_get(MISO)


// FUNCTION DECLERATIONS
void    spi_start();
void    spi_end();
uint8_t spi_transfer(uint8_t);

void    write(uint8_t, uint8_t);
uint8_t read(uint8_t);

#endif