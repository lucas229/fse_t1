#ifndef UART_H
#define UART_H

void initializeUart(char *name);
void writeUart(unsigned char *tx_buffer, int size);
int readUart(unsigned char *rx_buffer);
void closeUart();

#endif
