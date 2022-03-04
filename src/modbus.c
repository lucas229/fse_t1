#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "modbus.h"
#include "uart.h"
#include "crc16.h"

int createPackage(unsigned char functionCode, unsigned char dataCode, unsigned char *enrollment, unsigned char *data, unsigned char dataLength, unsigned char *tx_buffer) {
    int p = 0;
    tx_buffer[p++] = 0x01;
    tx_buffer[p++] = functionCode;
    tx_buffer[p++] = dataCode;
    memcpy(&tx_buffer[p], enrollment, ENROLLMENT_LENGTH);
    p += ENROLLMENT_LENGTH;
    memcpy(&tx_buffer[p], data, dataLength);
    p += dataLength;
    short crc = calcula_CRC(tx_buffer, p);
    memcpy(&tx_buffer[p], &crc, sizeof(short));
    p += sizeof(short);
    return p;
}

void writeModbus(unsigned char dataCode, unsigned char *enrollment, void *data) {
    unsigned char tx_buffer[261];
    int size;
    if(dataCode == CTRL_SIGNAL) {
        size = createPackage(0x16, dataCode, enrollment, data, sizeof(int), tx_buffer);
    } else if(dataCode == REF_SIGNAL) {
        size = createPackage(0x16, dataCode, enrollment, data, sizeof(float), tx_buffer);
    } else if(dataCode == SYS_STATUS || dataCode == CTRL_MODE) {
        size = createPackage(0x16, dataCode, enrollment, data, sizeof(unsigned char), tx_buffer);
    }
    initializeUart("/dev/serial0");
    writeUart(tx_buffer, size);
    closeUart();
}

int readModbus(unsigned char dataCode, unsigned char *enrollment, void *data) {
    unsigned char tx_buffer[3 + ENROLLMENT_LENGTH + sizeof(short)];
    int size = createPackage(0x23, dataCode, enrollment, NULL, 0, tx_buffer), rx_length, errors = 0;
    while(errors < 3) {
        unsigned char rx_buffer[261];
        rx_length = receive(tx_buffer, rx_buffer, size);
        short crc;
        memcpy(&crc, &rx_buffer[rx_length - 2], sizeof(short));
        if(rx_length != -1 && calcula_CRC(rx_buffer, rx_length - 2) == crc) {
            if(dataCode == INT_TEMP || dataCode == REF_TEMP) {
                memcpy(data, &rx_buffer[3], sizeof(float));
            } else if(dataCode == USER_CMD) {
                memcpy(data, &rx_buffer[3], sizeof(int));
            }
            break;
        } else {
            errors++;
            rx_length = -1;
        }
    }
    return rx_length;
}

int receive(unsigned char *tx_buffer, unsigned char *rx_buffer, int size) {
    initializeUart("/dev/serial0");
    writeUart(tx_buffer, size);
    sleep(1);
    int rx_length = readUart(rx_buffer);
    closeUart();
    return rx_length;
}
