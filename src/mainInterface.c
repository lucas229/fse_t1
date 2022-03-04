#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <softPwm.h>

#include "mainInterface.h"
#include "pid.h"

static unsigned char enrollment[ENROLLMENT_LENGTH];
static float referenceTemperature = -1;
static double kp = 20, ki = 0.1, kd = 100;

void initMenu() {
    system("clear");
    enrollmentMenu();
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
        int status = 2;
        if(readModbus(USER_CMD, enrollment, &status) != -1 && status == 1) {
            reply = 1;
            writeModbus(SYS_STATUS, enrollment, &reply);

            if(referenceTemperature == -1) {
                unsigned char mode = 0;
                writeModbus(CTRL_MODE, enrollment, &mode);
            } else {
                unsigned char mode = 1;
                writeModbus(CTRL_MODE, enrollment, &mode);
            }

            startOven();
        }
        printf("Aguardando acionamento...\n\n");
        sleep(1);
    }
}

void startOven() {
    int resistorPin = 4, fanPin = 5;
    wiringPiSetup();
    pinMode(resistorPin, OUTPUT);
    softPwmCreate(resistorPin, 1, 100);
    pinMode(fanPin, OUTPUT);
    softPwmCreate(fanPin, 40, 100);

    while(1) {
        float internalTemperature;
        readModbus(INT_TEMP, enrollment, &internalTemperature);
        readModbus(REF_TEMP, enrollment, &referenceTemperature);

        pid_atualiza_referencia(referenceTemperature);
        pid_configura_constantes(kp, ki, kd);
        int signal = pid_controle(internalTemperature);

        printf("Temperatura interna: %.2f\n", internalTemperature);
        printf("Temperatura de referência: %.2f\n", referenceTemperature);
        printf("Sinal de controle: %d\n\n", signal);

        if(signal > -40 && signal < 0) {
            signal = -40;
        }
        writeModbus(CTRL_SIGNAL, enrollment, &signal);
        writeModbus(REF_SIGNAL, enrollment, &referenceTemperature);

        if(signal >= 0) {
            softPwmWrite(resistorPin, signal);
        } else {
            signal = -signal;
            softPwmWrite(fanPin, signal);
        }

        int status = 1;
        if(readModbus(USER_CMD, enrollment, &status) != -1 && status == 2) {
            softPwmStop(resistorPin);
            softPwmStop(fanPin);
            unsigned char reply = 0;
            writeModbus(SYS_STATUS, enrollment, &reply);
            break;
        }

        sleep(1);
    }
}

void referenceTemperatureMenu() {
    printf("Insira -1 para desativar o modo de controle por terminal.\n\n");
    printf("Temperatura de referência: ");
    scanf("%f", &referenceTemperature);
    system("clear");
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
