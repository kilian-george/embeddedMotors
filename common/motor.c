#include "motor.h"

int32_t        _encoder = 0;
uint32_t       _velocitySamples[VELOCITY_SAMPLES];
uint32_t       _velocityNext = 0;
motor_status_t _motor_status = M_IDLE;
int32_t        _motorSetpointPosition = 0;

void motorDrive(int32_t drive) {
    // printf("I'm driving here!\n");
    assert(drive >= -100);
    assert(drive <= 100);

    if (drive > 0) {
        gpio_put(CT1, 1);
        gpio_put(CT2, 0);
    } else {
        gpio_put(CT1, 0);
        gpio_put(CT2, 1);
    }
    
    int32_t duty = abs(drive);
    // duty = (duty * (100 - DEADBAND) / 100);
    setPWMDuty(duty);
}

// TASK
static void _pidPositionServo( void *notUsed ) {
    int previous_error = 0;
    double integral = 0;
    int dt=1;

    double Kp = 4; // TODO You will need to discover these three parameters
    double Ki = 0;
    double Kd = 3.5;

    while(1) {
        int error = _motorSetpointPosition - _encoder;  // where we want to be minus where were at
        integral = (error + previous_error)*0.5*dt;
        //integral = integral + error*dt;
        double derivative = (error - previous_error)/dt;
        double output = Kp*error + Ki*integral + Kd*derivative;
        int drive = output;
        if (drive > 99) drive = 99;
        if (drive < -99) drive = -99;
        printf("TP: %d AP: %d Error: %d Drive: %d \n", _motorSetpointPosition, _encoder, error, drive);
        motorDrive(drive);
        previous_error = error;

        // printf("  !! sp:%d, mp:%d, d:%d, e:%d, t:%u, v:%d\n", 
        //         _motorSetpointPosition, 
        //         _encoder, 
        //         drive, 
        //         (uint32_t)error,
        //         _readTimer(),
        //         motor_get_velocity());

        vTaskDelay(pdMS_TO_TICKS(dt));
        //vTaskDelayUntil( &xLastWakeTime, dt);        
    }
}

void motor_init(void) {
    printf("%s: \n", __func__);

    // For watching the ISR timing
    gpio_init(PULSE);
    gpio_init(CT1);
    gpio_init(CT2);
    
    gpio_set_dir(PULSE, GPIO_OUT);
    gpio_set_dir(CT1,   GPIO_OUT);
    gpio_set_dir(CT2,   GPIO_OUT);

    ConfigurePWM(30000);
    setPWMDuty(10);
    setPWMDuty(90);
    setPWMDuty(33);
    //_setupTimer();

    xTaskCreate(_pidPositionServo, "bid", 1024, NULL, MIN_PRIORITY+2, NULL);
}


// ————————TOOLS——————————
void motor_set_position(int32_t position) {
    _motorSetpointPosition = position;
}


int32_t motor_get_position(void) {
    return _encoder;
}


int32_t motor_get_velocity(void) {
    int i = 0;
    int32_t v = 0;

    for (i = 0; i < VELOCITY_SAMPLES; i++) {
        v += _velocitySamples[i];
    }

    return (v / VELOCITY_SAMPLES);
}


motor_status_t motor_status(void) {
    return _motor_status;
}


void motor_speed_limit(motor_speed_t speed) {

}


void motor_move(uint32_t pos_in_tics) {

}


uint32_t _readTimer(void) { // placeholder until I get a high-res timer working.
    return time_us_32();
}