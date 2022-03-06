#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "mainInterface.h"
#include "pid.h"
#include "modbus.h"
#include "i2clcd.h"

static unsigned char enrollment[ENROLLMENT_LENGTH], mode = POTENTIOMETER_MODE;
static float referenceTemperature = -1;
static double kp = 20, ki = 0.1, kd = 100;

void initMenu() {
    system("clear");
    enrollmentMenu();
    lcd_connect();
    lcdOff();
    mainMenu();
}

void enrollmentMenu() {
    printf("Matrícula: ");
    int num;
    scanf("%d", &num);
    system("clear");
    for(int i = 0; i < ENROLLMENT_LENGTH; i++) {
        enrollment[ENROLLMENT_LENGTH - i - 1] = num % 10;
        num /= 10;
    }
}

void mainMenu() {
    while(1) {
        printf("1 - Aguardar forno\n");
        printf("2 - Alterar temperatura de referência\n");
        printf("3 - Alterar parâmetros\n");
        printf("4 - Visualizar valores\n");
        printf("0 - Finalizar\n");
        int choice;
        scanf("%d", &choice);
        system("clear");
        if(choice == 1) {
            waitOven();
        } else if(choice == 2) {
            referenceTemperatureMenu();
        } else if(choice == 3) {
            parametersMenu();
        } else if(choice == 4) {
            showValues();
        } else {
            break;
        }
    }
}

void waitOven() {
    unsigned char reply = 0;
    writeModbus(SYS_STATUS, enrollment, &reply);

    while(1) {
        int status = 0x02;
        if(readModbus(USER_CMD, enrollment, &status) != -1 && status == 0x01) {
            reply = 1;
            writeModbus(SYS_STATUS, enrollment, &reply);

            if(mode == TERMINAL_MODE) {
                reply = CURVE_MODE;
                writeModbus(CTRL_MODE, enrollment, &reply);
            } else {
                writeModbus(CTRL_MODE, enrollment, &mode);
            }

            initOven();
            lcdOff();
            system("clear");
            mode = POTENTIOMETER_MODE;
            referenceTemperature = -1;

            printf("1 - Aguardar forno\n2 - Menu inicial\n\n");
            printf("Forno desligado. Deseja aguardar o acionamento do forno ou retornar ao menu inicial?\n");
            int choice;
            scanf("%d", &choice);
            system("clear");
            if(choice == 2) {
                break;
            }
        }

        printf("Aguardando acionamento...\n\n");
        sleep(1);
    }
}

void initOven() {
    initGpio();
    int time = 0, target = 0, log = getLogFile(), resistorSignal = 0, fanSignal = 0;
    FILE *file = NULL;
    while(1) {
        float internalTemperature;
        readModbus(INT_TEMP, enrollment, &internalTemperature);

        if(mode == POTENTIOMETER_MODE) {
            readModbus(REF_TEMP, enrollment, &referenceTemperature);
        } else if(mode == CURVE_MODE) {
            if(file == NULL) {
                file = fopen("Data/curva_reflow.csv", "r");
                fscanf(file, "%*[^\n]");
                fscanf(file, "%d,", &target);
            }
            if(time == target) {
                fscanf(file, "%f", &referenceTemperature);
                fscanf(file, "%d,", &target);
            }
            time++;
        }

        int signal = calculateSignal(internalTemperature);

        if(signal > -40 && signal < 0) {
            signal = -40;
        }

        printf("Temperatura interna: %.2f\n", internalTemperature);
        printf("Temperatura de referência: %.2f\n", referenceTemperature);
        printf("Sinal de controle: %d\n", signal);
        printf("Time: %d\n\n", time);

        writeModbus(CTRL_SIGNAL, enrollment, &signal);
        writeModbus(REF_SIGNAL, enrollment, &referenceTemperature);

        if(signal >= 0) {
            resistorSignal = signal;
            softPwmWrite(RESISTOR_PIN, signal);
        } else {
            signal = -signal;
            fanSignal = signal;
            resistorSignal = 0;
            softPwmWrite(FAN_PIN, signal);
            softPwmWrite(RESISTOR_PIN, 0);
        }

        updateDisplay(internalTemperature);

        logData(log, internalTemperature, resistorSignal, fanSignal);

        int status = readCommand();
        if(status == 0x02) {
            break;
        } else if(status == 0x03) {
            time = 0;
            if(file != NULL) {
                fclose(file);
                file = NULL;
            }
        }
        sleep(1);
    }
    if(file != NULL) {
        fclose(file);
    }
    close(log);
}

void initGpio() {
    wiringPiSetup();
    pinMode(RESISTOR_PIN, OUTPUT);
    softPwmCreate(RESISTOR_PIN, 0, 100);
    pinMode(FAN_PIN, OUTPUT);
    softPwmCreate(FAN_PIN, 40, 100);
}

int calculateSignal(int internalTemperature) {
    pid_atualiza_referencia(referenceTemperature);
    pid_configura_constantes(kp, ki, kd);
    return pid_controle(internalTemperature);
}

int getLogFile() {
    struct stat info;
    if(stat("Data", &info) == -1) {
        mkdir("Data", S_IRWXU);
    }
    int file = open("Data/log.csv", O_CREAT|O_WRONLY, S_IRWXU);
    char header[] = "Date (YYYY-MM-DD),Time (HH:MM:SS),Internal temperature,Reference temperature,Resistor,Fan\n";
    write(file, header, strlen(header));
    return file;
}

void logData(int file, float internalTemperature, int resistorSignal, int fanSignal) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char data[256];
    strftime(data, 256, "%Y-%m-%d,", local);
    write(file, data, strlen(data));
    strftime(data, 256, "%H:%M:%S,", local);
    write(file, data, strlen(data));
    sprintf(data, "%f,%lf,%d,%d\n", internalTemperature, referenceTemperature, resistorSignal, fanSignal);
    write(file, data, strlen(data));
}

int readCommand() {
    int status = 0x01;
    readModbus(USER_CMD, enrollment, &status);
    if(status == 0x02) {
        softPwmStop(RESISTOR_PIN);
        softPwmStop(FAN_PIN);
        unsigned char reply = 0;
        writeModbus(SYS_STATUS, enrollment, &reply);
    } else if(status == 0x03) {
        mode = POTENTIOMETER_MODE;
        writeModbus(CTRL_MODE, enrollment, &mode);
    } else if(status == 0x04) {
        mode = CURVE_MODE;
        writeModbus(CTRL_MODE, enrollment, &mode);
    }
    return status;
}

void referenceTemperatureMenu() {
    printf("Insira -1 para desativar o modo de controle por terminal.\n\n");
    printf("Temperatura de referência: ");
    scanf("%f", &referenceTemperature);
    system("clear");
    if(referenceTemperature == -1) {
        mode = POTENTIOMETER_MODE;
    } else {
        mode = TERMINAL_MODE;
    }
}

void parametersMenu() {
    printf("Kp: ");
    scanf("%lf", &kp);
    printf("Ki: ");
    scanf("%lf", &ki);
    printf("Kd: ");
    scanf("%lf", &kd);
    system("clear");
}

void showValues() {
    printf("Matrícula: ");
    for(int i = 0; i < ENROLLMENT_LENGTH; i++) {
        printf("%u", enrollment[i]);
    }
    printf("\nTemperatura de referência: %.2f\n", referenceTemperature);
    printf("Kp: %.2lf\n", kp);
    printf("Ki: %.2lf\n", ki);
    printf("Kd: %.2lf\n\n", kd);
}

void lcdOff() {
    ClrLcd();
    lcdLoc(LINE1);
    char data[256] = "Desligado";
    typeln(data);
}

void updateDisplay(float internalTemperature) {
    ClrLcd();
    lcdLoc(LINE1);
    char data[256];
    if(mode == TERMINAL_MODE) {
        sprintf(data, "Modo: Terminal");
    } else {
        sprintf(data, "Modo: UART");
    }
    typeln(data);
    lcdLoc(LINE2);
    sprintf(data, "TI:%.1f TR:%.1lf", internalTemperature, referenceTemperature);
    typeln(data);
}

void closeConnections() {
    softPwmStop(RESISTOR_PIN);
    softPwmStop(FAN_PIN);
    unsigned char status = 0;
    writeModbus(SYS_STATUS, enrollment, &status);
    ClrLcd();
}
