#ifndef MODBUS_H
#define MODBUS_H

#define INT_TEMP 0xC1
#define REF_TEMP 0xC2
#define USER_CMD 0xC3
#define SYS_STATUS 0xD3
#define CTRL_MODE 0xD4
#define STRING_SND 0xB3
#define ENROLLMENT_LENGTH 4

int createPackage(unsigned char functionCode, unsigned char dataCode, unsigned char *enrollment, unsigned char *data, unsigned char dataLength, unsigned char *tx_buffer);
void writeModbus(unsigned char dataCode, unsigned char *enrollment, void *data);
int readModbus(unsigned char dataCode, unsigned char *enrollment, void *data);
int receive(unsigned char *tx_buffer, unsigned char *rx_buffer, int size);

#endif
