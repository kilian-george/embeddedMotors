#ifndef __LAB07_PWM_H_INCLUDED_
#define __LAB07_PWM_H_INCLUDED_

#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM 10

void ConfigurePWM(uint32_t baseFrequency);
void setPWMDuty(uint32_t percent);

#endif