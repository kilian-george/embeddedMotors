/**
 * @file    slave.c
 * @brief   
 * @author  Venessa Kuchenik
 * @date    2026-04-05
 */
#include "slave.h"

// registers
uint8_t registers[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};      // credits to Killian for his idea on how to setup the registers

void set_motor() {
    int32_t motor_cmd = (int32_t)((*(int64_t*)(registers)) & 0x00000000FFFFFFFF);
    motor_set_position(motor_cmd);
}

void gpio_callback(uint gpio, uint32_t unused) {
    queueData qd;
    uint8_t cs, clk, mosi;
    static uint8_t priorCS = 0, priorCLK = 0;  // initialized once, updated values persist btwn function calls
    gpio_put(PICO_DEFAULT_LED_PIN, HIGH);      // set LED pin 25 HIGH

    if (gpio == CS || gpio == CLK){
        // Get current states
        cs = getCS();
        clk = getCLK();
        mosi = getMOSI();

        if (cs != priorCS) {            // did CS change?
            qd.nibbles.cs_clk = (cs == 0) ? CS_LOW : CS_HIGH;
            qd.nibbles.mosi = 0;        // Clear MOSI
            xQueueSendFromISR(Queue, &qd, NULL);
            priorCS = cs;
        } else if (clk != priorCLK) {   // did CLK change?
            qd.nibbles.cs_clk = (clk == 0) ? CLK_LOW : CLK_HIGH;
            qd.nibbles.mosi = mosi;     // Include MOSI value when clock changes
            xQueueSendFromISR(Queue, &qd, NULL);
            priorCLK = clk;
        }
    } else if (gpio == PHA_PIN){
        if (gpio_get(PHB_PIN)) _encoder++;
        else _encoder--;
    }
    gpio_put(PICO_DEFAULT_LED_PIN, LOW);      // set LED pin 25 LOW
}


void hardware_init(void) {
    gpio_init(MOSI);
    gpio_init(CLK);
    gpio_init(CS);
    gpio_init(MISO);
    gpio_init(LED);
    gpio_init(PHA_PIN);
    gpio_init(PHB_PIN);
    
    gpio_set_dir(PHA_PIN, GPIO_IN);
    gpio_set_dir(PHB_PIN, GPIO_IN);
    gpio_set_dir(MOSI,  GPIO_IN);
    gpio_set_dir(CLK,   GPIO_IN);
    gpio_set_dir(CS,    GPIO_IN);
    gpio_set_dir(MISO,  GPIO_OUT);
    gpio_set_dir(LED,   GPIO_OUT);

    gpio_pull_up(CS);       // so CS pin doesn't float
    gpio_pull_up(CLK);

    // Queue Creation
    Queue = xQueueCreate(100, sizeof(queueData)); // one spi transfer has 34 events to store in the queue

    // ENABLE INTERRUPTS
    gpio_set_irq_enabled_with_callback(PHA_PIN, GPIO_IRQ_EDGE_RISE , true, &gpio_callback);    
    gpio_set_irq_enabled(CS,    GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(CLK,   GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}


void state_machine(void *none) {              // where the state machine will exist
    State      state = IDLE;
    Register   reg_select;
    queueData  q_data;
    uint8_t    bit_count = 0;
    uint8_t    buffer = 0;
    uint8_t    address = 0;
    uint8_t    command = 0;

    while(1) {
        printf("TOP OF STATE MACHINE\n");
        xQueueReceive(Queue, &q_data, portMAX_DELAY);           // wont fall down until something is in queue

        switch (state) {
            case IDLE:
                if (q_data.nibbles.cs_clk == CS_LOW) {
                    state = CMD;
                    bit_count = 0;
                    buffer = 0;
                }
                break;
            
            case CMD:   // parent sending first byte
                if (q_data.nibbles.cs_clk == CLK_HIGH) {
                    buffer <<= 1;                                    // collect addr and cmd
                    buffer |= (q_data.nibbles.mosi & 0x01);
                    bit_count++;
                }
                if (bit_count >= 8) {
                    printf("ADDR_CMD read in: buffer = 0x%02x\n", buffer);
                    address = (0xF0 & buffer) >> 4;         // parse out address
                    command = 0x0F & buffer;                // parse out command

                    if      (address == 0x00) reg_select = MOTOR_POS_0;
                    else if (address == 0x01) reg_select = MOTOR_POS_8;
                    else if (address == 0x02) reg_select = MOTOR_POS_16;
                    else if (address == 0x03) reg_select = MOTOR_POS_24;
                    else if (address == 0x04) reg_select = MOTOR_CMD_0;
                    else if (address == 0x05) reg_select = MOTOR_CMD_8;
                    else if (address == 0x06) reg_select = MOTOR_CMD_16;
                    else if (address == 0x07) reg_select = MOTOR_CMD_24;
                    else if (address == 0x08) reg_select = CMD_REG;
                    else if (address == 0x09) reg_select = STAT_REG;

                    if      (command == 0x0A) state = READ;             // determine state from command
                    else if (command == 0x0B) state = WRITE;

                    bit_count = 0;                                  // reset bit count
                    buffer = 0;                                     // reset buffer
                }
                break;
            
            case READ:  // parent wants to read a reg
                if (q_data.nibbles.cs_clk == CLK_LOW) {     // setup data on falling edge, parent reads data on rising
                    setMISO((registers[reg_select] << bit_count) & 0x80);  // move over by number of bits seen and mask with 10000000b
                    bit_count++;
                }
                if (bit_count >= 8) {
                    printf("Register data read to parent.\n");
                    printf("next state: DONE\n");
                    bit_count = 0;
                    state = DONE;
                }
                break;
            
            case WRITE: // parent wants to write
                if (q_data.nibbles.cs_clk == CLK_HIGH) {
                    buffer <<= 1;
                    buffer |= (q_data.nibbles.mosi & 0x01);  // maybe need `q_data.nibbles.mosi & 0x01` here
                    bit_count++;
                }
                if (bit_count >= 8) {
                    registers[reg_select] = buffer;
                    if (reg_select == CMD_REG && (CMD_REG == SET_MOTOR)) set_motor();
                    bit_count = 0;
                    buffer = 0;
                    state = DONE;
                }
                break;
            
            case DONE:  // or ERROR
                if (q_data.nibbles.cs_clk == CS_HIGH) state = IDLE;
                if (state == IDLE) {
                    printf("communication ended with parent\n");
                    printf("next state: IDLE\n");
                }
                bit_count = 0;
                buffer = 0;
                break;
        }
    }
}

/**
 * @brief   Main entry point. Keep this function small.
 *          Perform things such as:
 *            - setting up hardware
 *            - declaring tasks
 *            - starting RTOS Scheduler  
 */
int main(void) {
    #ifdef ACTUAL_SERIAL
        stdio_uart_init();
    #else
        stdio_init_all();
    #endif

    sleep_ms(20);           // give time for serial bus to init
    hardware_init();
    motor_init();

    // create tasks
    xTaskCreate(state_machine, "state_machine", 2048, NULL, MIN_PRIORITY, NULL);
    
    vTaskStartScheduler();
    
    while(1){};
}