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
#include "bme280Controller.h"

static unsigned char enrollment[ENROLLMENT_LENGTH], mode = POTENTIOMETER_MODE;
static float referenceTemperature = -1;
static double kp = 20, ki = 0.1, kd = 100;

void initMenu() {
    system("clear");
    initDevices();
    enrollmentMenu();
    mainMenu();
}

void initDevices() {
    initGpio();
    lcd_connect();
    init_sensor();
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
        printf("1 - Aguardar acionamento do forno\n");
        printf("2 - Alterar temperatura de referência\n");
        printf("3 - Alterar parâmetros do PID\n");
        printf("4 - Mostrar valores atuais\n");
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
            closeConnections();
            break;
        }
    }
}

void waitOven() {
    unsigned char reply = 0;
    writeModbus(SYS_STATUS, enrollment, &reply);
    displayOff();

    int flag = 0;
    while(1) {
        int status = 0x02;
        if((readModbus(USER_CMD, enrollment, &status) != -1 && status == 0x01) || flag) {
            reply = 1;
            writeModbus(SYS_STATUS, enrollment, &reply);

            int status;

            if(mode == POTENTIOMETER_MODE) {
                writeModbus(CTRL_MODE, enrollment, &mode);
                status = potentiometerMode();
                referenceTemperature = -1;
            } else if(mode == CURVE_MODE) {
                writeModbus(CTRL_MODE, enrollment, &mode);
                status = curveMode();
                referenceTemperature = -1;
            } else if(mode == TERMINAL_MODE) {
                reply = CURVE_MODE;
                writeModbus(CTRL_MODE, enrollment, &reply);
                status = terminalMode();
            }

            system("clear");
            if(status == 0x02) {
                printf("1 - Aguardar forno\n2 - Menu inicial\n\n");
                printf("Forno desligado. Deseja aguardar o acionamento do forno ou retornar ao menu inicial?\n");
                int choice;
                scanf("%d", &choice);
                system("clear");
                if(choice == 1) {
                    displayOff();
                    flag = 0;
                } else if(choice == 2) {
                    ClrLcd();
                    break;
                }
            } else {
                flag = 1;
            }

        }

        printf("Aguardando acionamento...\n\n");
        sleep(1);
    }
}

int potentiometerMode() {
    int log = getLogFile(), status;
    while(1) {
        float internalTemperature;
        if(readModbus(INT_TEMP, enrollment, &internalTemperature) == -1) {
            showError();
            break;
        }

        if(readModbus(REF_TEMP, enrollment, &referenceTemperature) == -1) {
            showError();
            break;
        }

        int signal = calculateSignal(internalTemperature);

        struct bme280_data data = get_sensor_data();
        printf("Temperatura interna: %.2f\n", internalTemperature);
        printf("Temperatura externa: %.2lf\n", data.temperature);
        printf("Temperatura de referência: %.2f\n", referenceTemperature);
        printf("Sinal de controle: %d\n\n", signal);

        updateDisplay(internalTemperature, data.temperature);
        logData(log, internalTemperature, data.temperature, signal);

        status = readCommand();
        if(status == 0x02 || status == 0x04) {
            break;
        }
        usleep(500000);
    }
    close(log);
    return status;
}

int curveMode() {
    int time = 0, target, log = getLogFile(), status;
    FILE *file = fopen("Data/curva_reflow.csv", "r");
    fscanf(file, "%*[^\n]");
    fscanf(file, "%d,", &target);
    while(1) {
        if(time == target) {
            fscanf(file, "%f", &referenceTemperature);
            fscanf(file, "%d,", &target);
        }

        float internalTemperature;
        if(readModbus(INT_TEMP, enrollment, &internalTemperature) == -1) {
            showError();
            break;
        }

        int signal = calculateSignal(internalTemperature);
        writeModbus(REF_SIGNAL, enrollment, &referenceTemperature);

        struct bme280_data data = get_sensor_data();
        printf("Temperatura interna: %.2f\n", internalTemperature);
        printf("Temperatura externa: %.2lf\n", data.temperature);
        printf("Temperatura de referência: %.2f\n", referenceTemperature);
        printf("Sinal de controle: %d\n", signal);
        printf("Tempo: %d\n\n", time);

        updateDisplay(internalTemperature, data.temperature);
        logData(log, internalTemperature, data.temperature, signal);

        status = readCommand();
        if(status == 0x02 || status == 0x03) {
            break;
        }
        
        time++;
        usleep(500000);
    }
    fclose(file);
    close(log);
    return status;
}

int terminalMode() {
    int log = getLogFile(), status;
    writeModbus(REF_SIGNAL, enrollment, &referenceTemperature);
    while(1) {
        float internalTemperature;
        if(readModbus(INT_TEMP, enrollment, &internalTemperature) == -1) {
            showError();
            break;
        }

        int signal = calculateSignal(internalTemperature);

        struct bme280_data data = get_sensor_data();
        printf("Temperatura interna: %.2f\n", internalTemperature);
        printf("Temperatura externa: %.2lf\n", data.temperature);
        printf("Temperatura de referência: %.2f\n", referenceTemperature);
        printf("Sinal de controle: %d\n\n", signal);

        updateDisplay(internalTemperature, data.temperature);
        logData(log, internalTemperature, data.temperature, signal);

        status = readCommand();
        if(status == 0x02 || status == 0x03 || status == 0x04) {
            break;
        }

        usleep(500000);
    }
    close(log);
    return status;
}

void showError() {
    printf("Desligando o forno (Erro na conexão Modbus)...\n\n");
    sleep(3);
    softPwmWrite(RESISTOR_PIN, 0);
    unsigned char reply = 0;
    writeModbus(SYS_STATUS, enrollment, &reply);
}

void initGpio() {
    wiringPiSetup();
    pinMode(RESISTOR_PIN, OUTPUT);
    softPwmCreate(RESISTOR_PIN, 0, 100);
    pinMode(FAN_PIN, OUTPUT);
    softPwmCreate(FAN_PIN, 0, 100);
}

int calculateSignal(float internalTemperature) {
    pid_atualiza_referencia(referenceTemperature);
    pid_configura_constantes(kp, ki, kd);
    int signal = pid_controle(internalTemperature);

    if(signal > -40 && signal < 0) {
        signal = -40;
    }

    writeModbus(CTRL_SIGNAL, enrollment, &signal);

    if(signal >= 0) {
        softPwmWrite(RESISTOR_PIN, signal);
        softPwmWrite(FAN_PIN, 0);
    } else {
        softPwmWrite(FAN_PIN, -signal);
        softPwmWrite(RESISTOR_PIN, 0);
    }

    return signal;
}

int getLogFile() {
    struct stat info;
    if(stat("Data", &info) == -1) {
        mkdir("Data", S_IRWXU);
    }
    int file = open("Data/log.csv", O_CREAT|O_WRONLY, S_IRWXU);
    char header[] = "Date(YYYY-MM-DD),Time(HH:MM:SS),Internal temperature,External temperature,Reference temperature,Signal\n";
    write(file, header, strlen(header));
    return file;
}

void logData(int file, float internalTemperature, double externalTemperature, int signal) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char data[256];
    strftime(data, 256, "%Y-%m-%d,", local);
    write(file, data, strlen(data));
    strftime(data, 256, "%H:%M:%S,", local);
    write(file, data, strlen(data));
    sprintf(data, "%f,%lf,%f,%d\n", internalTemperature, externalTemperature, referenceTemperature, signal);
    write(file, data, strlen(data));
}

int readCommand() {
    int status = 0x01;
    if(readModbus(USER_CMD, enrollment, &status) == -1) {
        showError();
        return 0x02;
    }
    if(status == 0x02) {
        softPwmWrite(RESISTOR_PIN, 0);
        unsigned char reply = 0;
        writeModbus(SYS_STATUS, enrollment, &reply);
    } else if(status == 0x03) {
        mode = POTENTIOMETER_MODE;
    } else if(status == 0x04) {
        mode = CURVE_MODE;
    }
    return status;
}

void referenceTemperatureMenu() {
    printf("Insira -1 para desativar o modo de controle por terminal.\n\n");
    printf("Temperatura de referência: ");
    float temperature;
    scanf("%f", &temperature);
    system("clear");
    struct bme280_data data = get_sensor_data();
    if(temperature < data.temperature && temperature != -1) {
        printf("A temperatura de referência não pode ser menor do que a temperatura ambiente (%.2lf).\n\n", data.temperature);
        return;
    }
    referenceTemperature = temperature;
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

void displayOff() {
    ClrLcd();
    lcdLoc(LINE1);
    char data[] = "Desligado";
    typeln(data);
}

void updateDisplay(float internalTemperature, double externalTemperature) {
    ClrLcd();
    lcdLoc(LINE1);
    char data[256];
    if(mode == TERMINAL_MODE) {
        sprintf(data, "TERM TI:%.1f", internalTemperature);
    } else {
        sprintf(data, "UART TI:%.1f", internalTemperature);
    }
    typeln(data);
    lcdLoc(LINE2);
    sprintf(data, "TE:%.1lf TR:%.1f", externalTemperature, referenceTemperature);
    typeln(data);
}

void closeConnections() {
    softPwmStop(RESISTOR_PIN);
    softPwmStop(FAN_PIN);
    lcd_close();
    close_sensor();
    closeModbus();
}

void closeAll() {
    closeConnections();
    softPwmWrite(RESISTOR_PIN, 0);
    unsigned char status = 0;
    writeModbus(SYS_STATUS, enrollment, &status);
    ClrLcd();
}
