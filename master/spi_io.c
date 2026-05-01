#include "master.h"

void spi_start() {
    setCS(LOW);
}

void spi_end() {
    setCS(HIGH);
}

uint8_t spi_transfer(uint8_t out) { // transfers 1 byte at a time
    uint8_t count = 0, in = 0;
    setCLK(LOW);                    // makes sure CLK starts at 0 which is baseline for SPI Mode 1

    for (; count < 8; count++) {    // 8 bits in a byte
        in <<= 1;                   // shift over by one bit to make room for reading in new bit
        setMOSI(out & 0x80);        // (0x80 == 10000000b) masks the out variable which is a byte
        setCLK(HIGH);               // set high so that MCP23S17 can read the MISO line (MCP23S17 reads on rising edge)
        in += getMISO();            // read in MISO line and adds to in. is reading MSB -> LSB
        setCLK(LOW);                // sets CLK LOW (one cycle finished)
        out <<= 1;                  // shift bits by one so next bit is MSB and can be sent in the setMOSI line by masking w/ 0x80
    }
    setMOSI(0);                     // sets to safe LOW state

    return in;                      // returns data recieved
}


void write(uint8_t addr, uint8_t data) {
    uint8_t addr_cmd = addr | WRITE;
    spi_start();                  
    spi_transfer(addr_cmd);           // send the 4 bit preamble and 4 bit address
    spi_transfer(data);               // The data to go at dest address
    spi_end();   
}

uint8_t read(uint8_t addr) {
    uint8_t addr_cmd = addr | READ;
    // printf("addr_cmd: 0x%02x\n", addr_cmd);
    uint8_t value;
    
    spi_start();    
    spi_transfer(addr_cmd);
    value = spi_transfer(1);
    spi_end(); 
    
    return value;
}