#include <stdio.h>
#include <unistd.h>         //Used for UART
#include <fcntl.h>          //Used for UART
#include <termios.h>        //Used for UART

#include "uart.h"

static int uart0_filestream = -1;

void initializeUart(char *name) {
    uart0_filestream = open(name, O_RDWR | O_NOCTTY | O_NDELAY);      //Open in non blocking read/write mode
    // if(uart0_filestream == -1) {
    //     printf("Erro - Não foi possível iniciar a UART.\n\n");
    // } else {
    //     printf("UART inicializada!\n\n");
    // }
    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;     //Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
}

void writeUart(unsigned char *tx_buffer, int size) {
    if(uart0_filestream != -1) {
        // printf("Escrevendo caracteres na UART ...");
        write(uart0_filestream, tx_buffer, size);
        // if(count < 0) {
        //     printf("UART TX error\n\n");
        // } else {
        //     printf("escrito.\n\n");
        // }
    }
}

int readUart(unsigned char *rx_buffer) {
    //----- CHECK FOR ANY RX BYTES -----
    int rx_length = -1;
    if(uart0_filestream != -1) {
        // Read up to 261 characters from the port if they are there
        rx_length = read(uart0_filestream, rx_buffer, 261);      //Filestream, buffer to store in, number of bytes to read (max)
        // if(rx_length < 0) {
        //     printf("Erro na leitura.\n\n"); //An error occured (will occur if there are no bytes)
        // } else if(rx_length == 0) {
        //     printf("Nenhum dado disponível.\n\n"); //No data waiting
        // }
    }
    return rx_length;
}

void closeUart() {
    if(uart0_filestream != -1) {
        close(uart0_filestream);
        uart0_filestream = -1;
    }
}
